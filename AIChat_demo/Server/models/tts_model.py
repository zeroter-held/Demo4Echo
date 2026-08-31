import asyncio
import io
import os
import queue
import re
import threading
import time

import dashscope
from dashscope.audio.tts_v2 import ResultCallback

from tools.logger import logger

# TTS 后端选择: "edge" (免费, 无需密钥) 或 "cosyvoice" (需要百炼 sk- 密钥)
TTS_BACKEND = os.getenv("TTS_BACKEND", "edge")
EDGE_VOICE = os.getenv("TTS_VOICE", "zh-CN-XiaoxiaoNeural")

# 朗读前文本清洗: 去掉 emoji / markdown 符号 / 列表符, 这些读出来是噪音
_EMOJI_RE = re.compile(
    "[\U0001F000-\U0001FAFF\U00002600-\U000027BF\U0001F1E6-\U0001F1FF"
    "\u2B00-\u2BFF\uFE0F\u200D]")
_MARKDOWN_RE = re.compile(r"[#*`>|_~]")
_BULLET_RE = re.compile(r"(^| )[-•] ")


def clean_tts_text(text: str) -> str:
    text = _EMOJI_RE.sub("", text)
    text = _MARKDOWN_RE.sub("", text)
    text = _BULLET_RE.sub(r"\1", text)
    text = re.sub(r"\s+", " ", text)
    return text.strip()


class EdgeTTSWorker:
    """edge-tts 后端: 把 LLM 流式文本按句切分, 逐句合成 mp3,
    解码重采样成 16kHz PCM 后通过 on_data 回调送出。
    对外行为与 cosyvoice 流式合成保持一致 (on_data / on_complete / on_close)。"""

    def __init__(self, voice: str = EDGE_VOICE):
        self.voice = voice
        self._queue = queue.Queue()
        self._thread = None
        self._buf = ""
        # 默认空回调, 防止未 set 就调用时报错
        self.on_open = lambda: None
        self.on_complete = lambda: None
        self.on_error = lambda m: None
        self.on_close = lambda: None
        self.on_data = lambda d: None

    # ---------- 外部接口 ----------
    def start(self, on_open=None, on_complete=None, on_error=None, on_close=None, on_data=None):
        self.on_open = on_open or self.on_open
        self.on_complete = on_complete or self.on_complete
        self.on_error = on_error or self.on_error
        self.on_close = on_close or self.on_close
        self.on_data = on_data or self.on_data
        self._buf = ""
        if self._thread is None or not self._thread.is_alive():
            self._thread = threading.Thread(target=self._run, daemon=True, name="edge-tts-worker")
            self._thread.start()
        self.on_open()

    def feed(self, text_chunk):
        if not isinstance(text_chunk, str) or not text_chunk:
            return
        self._buf += clean_tts_text(text_chunk)
        for sentence in self._extract_sentences():
            self._queue.put(sentence)

    def finish(self):
        tail = self._buf.strip()
        self._buf = ""
        if tail:
            self._queue.put(tail)
        self._queue.put(None)  # 会话结束哨兵

    # ---------- 内部实现 ----------
    def _extract_sentences(self):
        """从缓冲区里切出完整句子, 剩下半句留在缓冲区。"""
        parts = re.split(r"(?<=[。！？!?；\n])", self._buf)
        if len(parts) == 1:
            # 没有句末标点: 太长就按逗号强切, 否则继续攒着
            if len(self._buf) > 80:
                parts = re.split(r"(?<=[，,、])", self._buf)
            else:
                return []
        self._buf = parts[-1]
        return [p for p in parts[:-1] if p.strip()]

    def _run(self):
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        while True:
            task = self._queue.get()
            if task is None:
                # 本轮对话的文本全部合成完毕
                self.on_complete()
                self.on_close()
                continue
            try:
                loop.run_until_complete(self._synth_one(task))
            except Exception as e:
                logger.error(f"edge-tts synth error: {e}")
                self.on_error(str(e))
        loop.close()

    async def _synth_one(self, text: str):
        import edge_tts
        import torch
        import torchaudio

        t0 = time.time()
        mp3 = b""
        comm = edge_tts.Communicate(text, self.voice)
        async for chunk in comm.stream():
            if chunk["type"] == "audio":
                mp3 += chunk["data"]
        if not mp3:
            logger.warning(f"edge-tts empty audio for: {text[:20]}")
            return
        wave, sr = torchaudio.load(io.BytesIO(mp3), format="mp3")
        if sr != 16000:
            wave = torchaudio.transforms.Resample(orig_freq=sr, new_freq=16000)(wave)
        pcm = (wave.squeeze(0).clamp(-1, 1) * 32767).to(torch.int16).numpy().tobytes()
        # 按 200ms (6400 字节) 一块送出, 让发送线程节奏平滑
        for i in range(0, len(pcm), 6400):
            self.on_data(pcm[i:i + 6400])
        logger.info(f"edge-tts synth '{text[:12]}...' {len(pcm)} bytes in {time.time()-t0:.2f}s")


class TTSModel:

    def __init__(self):
        self.backend = TTS_BACKEND
        self.callback = None
        self._edge = None
        self.synthesizer = None
        if self.backend == "cosyvoice":
            from dashscope.audio.tts_v2 import SpeechSynthesizer, AudioFormat
            self.synthesizer = SpeechSynthesizer(
                model="cosyvoice-v1",
                voice="longxiaochun",
                format=AudioFormat.PCM_16000HZ_MONO_16BIT,
                callback=self.callback
            )
        else:
            self._edge = EdgeTTSWorker()

    class __tts_callback(ResultCallback):
        """cosyvoice 回调适配器 (继承 SDK 的 ResultCallback, 行为与原版一致)"""

        def __init__(self, on_open=None, on_complete=None, on_error=None, on_close=None, on_data=None):
            self._on_open = on_open or (lambda: logger.info("tts server-WS is open."))
            self._on_complete = on_complete or (lambda: logger.info("Speech synthesis task completed successfully."))
            self._on_error = on_error or (lambda message: logger.info(f"Speech synthesis task failed, {message}"))
            self._on_close = on_close or (lambda: logger.info("tts server-WS is closed."))
            self._on_data = on_data or (lambda data: logger.info(f"Audio result length: {len(data)}"))

        def on_open(self):
            self._on_open()

        def on_complete(self):
            self._on_complete()

        def on_error(self, message: str):
            self._on_error(message)

        def on_close(self):
            self._on_close()

        def on_data(self, data: bytes) -> None:
            self._on_data(data)

    def tts_stream_set(self, on_open=None, on_complete=None, on_error=None, on_close=None, on_data=None):
        '''设置TTS回调函数, 提前打开连接'''
        if self.backend == "edge":
            self._edge.start(on_open=on_open, on_complete=on_complete, on_error=on_error,
                             on_close=on_close, on_data=on_data)
            return True

        from dashscope.audio.tts_v2 import SpeechSynthesizer, AudioFormat
        self.callback = self.__tts_callback(on_open=on_open, on_complete=on_complete,
                                            on_error=on_error, on_close=on_close, on_data=on_data)
        self.synthesizer = SpeechSynthesizer(
            model="cosyvoice-v1",
            voice="longxiaochun",
            format=AudioFormat.PCM_16000HZ_MONO_16BIT,
            callback=self.callback
        )
        try:
            self.synthesizer.streaming_call('')  # 先提前打开ws连接
            time.sleep(0.1)
        except Exception as e:
            return False

    def tts_stream_close(self):
        if self.backend == "edge":
            self._edge.finish()
            return
        self.synthesizer.streaming_complete()
        logger.info(f"Request ID: {self.synthesizer.get_last_request_id()}")

    def tts_stream_speech_synthesis(self, text_chunk):
        '''流式语音合成

        :param: text_chunk: 文本片
        :return: 合成是否成功'''
        if self.backend == "edge":
            self._edge.feed(text_chunk)
            return True

        if text_chunk:
            try:
                self.synthesizer.streaming_call(text_chunk)
            except Exception as e:
                return False
