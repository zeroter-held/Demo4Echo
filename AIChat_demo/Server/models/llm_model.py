import json
import re
import time

import requests

from config.settings import global_settings
from tools.logger import logger

_THINK_OPEN = re.compile(r"<(think|thinking)>")
_THINK_CLOSE = re.compile(r"</(think|thinking)>")
_THINK_BLOCK = re.compile(r"<(think|thinking)>.*?</(think|thinking)>", re.S)


class LLMModel:
    """OpenAI 兼容协议的 LLM 客户端。

    与具体厂商解耦: 只要服务端提供 /chat/completions 接口
    (百炼 compatible-mode / Token Plan 专属地址 / 任意中转站) 都能用。
    连接三要素 (base_url / api_key / model) 全部来自 settings 的环境变量。
    """

    def __init__(self, model_name: str = "qwen-turbo"):
        self.model_name = model_name
        # 对话初始历史记录
        self.messages = [
            {"role": "system", "content": "你是一个桌面机器人, 名为Echo, 快速地回复我."}
        ]

    # ---------------- 内部工具 ----------------
    def _url(self) -> str:
        return global_settings.LLM_BASE_URL.rstrip("/") + "/chat/completions"

    def _headers(self) -> dict:
        return {
            "Authorization": "Bearer %s" % global_settings.LLM_API_KEY,
            "Content-Type": "application/json",
        }

    def _extra_body(self) -> dict:
        extra = {}
        # 百炼兼容地址支持 enable_search (联网搜索); 中转站可能不认这个字段,
        # 所以只在地址是阿里云域名时才附加, 避免 400。
        if "aliyuncs.com" in global_settings.LLM_BASE_URL:
            extra["enable_search"] = True
        # 语音助手不需要模型输出思考过程; 不支持该字段的厂商会在 400 后自动重试去掉它
        extra["enable_thinking"] = False
        return extra

    @staticmethod
    def _post(body: dict, stream: bool):
        """带重试的 POST:
        - 400 且带 enable_thinking 字段 -> 去掉该字段立即重试 (厂商不支持);
        - 5xx / 429 / 网络异常 -> 退避重试 (中转站偶发 SERVICE_BUSY);
        全部失败时返回最后一次的响应 (可能为 None)。"""
        url = global_settings.LLM_BASE_URL.rstrip("/") + "/chat/completions"
        headers = {
            "Authorization": "Bearer %s" % global_settings.LLM_API_KEY,
            "Content-Type": "application/json",
        }
        timeout = (5, 60) if stream else global_settings.API_TIMEOUT
        resp = None
        for attempt in range(3):
            try:
                resp = requests.post(url, headers=headers, json=body,
                                     timeout=timeout, stream=stream)
            except requests.RequestException as e:
                logger.warning(f"LLM 请求异常 attempt={attempt}: {e}")
                time.sleep(1 + attempt)
                continue
            if resp.status_code == 400 and body.get("enable_thinking") is not None:
                body = {k: v for k, v in body.items() if k != "enable_thinking"}
                continue  # 去掉不支持的字段, 立即重试
            if resp.status_code in (429, 500, 502, 503, 529):
                logger.warning(f"LLM 服务繁忙 {resp.status_code} attempt={attempt}, 退避重试...")
                time.sleep(1 + attempt)
                continue
            return resp
        return resp

    @staticmethod
    def _chunk_text(chunk: dict) -> str:
        """从流式 chunk 里提取增量文本, 兼容 delta / message 两种写法。"""
        try:
            choice = chunk.get("choices", [{}])[0]
        except (IndexError, AttributeError):
            return ""
        delta = choice.get("delta") or choice.get("message") or {}
        return delta.get("content") or ""

    @staticmethod
    def _filter_think(piece: str, state: dict) -> str:
        """流式 think 块过滤器: 吞掉 <think>...</think> 内容, 只放行正常回复。
        state = {"in_think": bool, "pending": str}"""
        buf = state["pending"] + piece
        out = []
        while True:
            if state["in_think"]:
                m = _THINK_CLOSE.search(buf)
                if m:
                    buf = buf[m.end():]
                    state["in_think"] = False
                    continue
                buf = ""  # 思考中, 全部静默吞掉
                break
            m = _THINK_OPEN.search(buf)
            if m:
                out.append(buf[:m.start()])
                buf = buf[m.end():]
                state["in_think"] = True
                continue
            # 没有完整开标签: 把结尾可能是 "<think" 前缀的部分先扣住等下一个片
            hold = ""
            idx = buf.rfind("<")
            if idx >= 0:
                tail = buf[idx:]
                if ("<think>".startswith(tail) or "<thinking>".startswith(tail)) and len(tail) >= 1:
                    hold = tail
                    buf = buf[:idx]
            out.append(buf)
            buf = hold
            break
        state["pending"] = buf
        return "".join(out)

    # ---------------- 对外接口 (与旧版保持一致) ----------------
    def set_model_sys_content(self, content: str):
        """设置模型的系统描述内容"""
        self.messages[0]["content"] = content

    def add_message(self, role: str, content: str):
        """添加对话历史记录"""
        self.messages.append({"role": role, "content": content})

    def clear_messages(self):
        """清空对话历史记录"""
        self.messages = [
            {"role": "system", "content": "你是一个桌面机器人, 名为Echo, 全程请快速地回复我. 同时你还有函数执行的功能, 可以根据函数来回复我. "}
        ]

    def get_LLM_response(self, question: str) -> str:
        """非流式生成回答 (意图识别在用)"""
        if question:
            self.add_message("user", question)

        body = {
            "model": self.model_name,
            "messages": self.messages,
            "stream": False,
        }
        body.update(self._extra_body())

        try:
            r = self._post(body, stream=False)
            if r is not None and r.status_code == 200:
                content = r.json()["choices"][0]["message"]["content"]
                # 兜底: 去掉可能存在的思考块
                content = _THINK_BLOCK.sub("", content).strip()
                self.add_message("assistant", content)
                return content
            status = r.status_code if r is not None else "None"
            logger.error(f"非流式生成失败，状态码: {status}, 响应: {r.text[:300] if r is not None else 'no response'}")
            return "抱歉，我暂时无法处理您的请求。"
        except Exception as e:
            logger.error(f"非流式生成异常: {str(e)}")
            return "抱歉，我暂时无法处理您的请求。"

    def get_LLM_response_stream(self, question):
        """流式生成回答 (yield 生成器, 需要在外部循环中调用)。
        出错时 yield -1, 与旧版行为一致。"""
        if question:
            self.add_message("user", question)

        body = {
            "model": self.model_name,
            "messages": self.messages,
            "stream": True,
        }
        body.update(self._extra_body())

        full_text = ""
        state = {"in_think": False, "pending": ""}
        try:
            r = self._post(body, stream=True)
            if r is None or r.status_code != 200:
                status = r.status_code if r is not None else "None"
                logger.error(f"流式生成失败，状态码: {status}, 响应: {r.text[:300] if r is not None else 'no response'}")
                # 不沉默: 读一句兜底话, 让用户知道是网络问题
                fallback = "抱歉，现在网络有点忙，请稍后再问我一次。"
                self.add_message("assistant", fallback)
                yield fallback
                return

            # 解析 SSE: 每行形如 "data: {...}", 结束标志 "data: [DONE]"
            for line in r.iter_lines(decode_unicode=True):
                if not line:
                    continue
                line = line.strip()
                if not line.startswith("data:"):
                    continue
                payload = line[len("data:"):].strip()
                if payload == "[DONE]":
                    break
                try:
                    chunk = json.loads(payload)
                except json.JSONDecodeError:
                    continue
                piece = self._chunk_text(chunk)
                if not piece:
                    continue
                safe = self._filter_think(piece, state)
                if safe:
                    full_text += safe
                    yield safe
            # 流结束: 放出被扣住的尾巴 (非 think 状态时)
            if not state["in_think"] and state["pending"]:
                full_text += state["pending"]
                yield state["pending"]
                state["pending"] = ""
            # 最后记录回复的信息
            self.add_message("assistant", full_text)
        except Exception as e:
            logger.error(f"An exception occurred: {str(e)}")
            if not full_text:
                fallback = "抱歉，现在网络有点忙，请稍后再问我一次。"
                self.add_message("assistant", fallback)
                yield fallback
            else:
                yield -1
