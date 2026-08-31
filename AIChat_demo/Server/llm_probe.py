# -*- coding: utf-8 -*-
"""连通性探针: 拿到新 key 后一次性验证 模型列表 / 对话 / TTS 三条链路。

用法 (在 Server 目录下):
    LLM_BASE_URL=https://ws-afun9ws4hgqrqcly.cn-beijing.maas.aliyuncs.com/compatible-mode/v1 \
    LLM_API_KEY=sk-ws-完整key \
    .venv/Scripts/python.exe llm_probe.py
"""
import os

import requests

BASE = os.getenv("LLM_BASE_URL",
                 "https://ws-afun9ws4hgqrqcly.cn-beijing.maas.aliyuncs.com/compatible-mode/v1")
KEY = os.getenv("LLM_API_KEY", "")

print("BASE =", BASE)
print("KEY len =", len(KEY), "| head =", KEY[:10], "... tail =", KEY[-10:])

h = {"Authorization": "Bearer " + KEY}

# ---------- 1) 模型列表 ----------
try:
    r = requests.get(BASE.rstrip("/") + "/models", headers=h, timeout=15)
    print("\n[1] MODELS status:", r.status_code)
    if r.status_code == 200:
        ids = [m.get("id") for m in r.json().get("data", [])]
        print("    MODEL IDS:", ids)
    else:
        print("    ", r.text[:300])
except Exception as e:
    print("[1] MODELS FAIL:", e)

# ---------- 2) 对话测试 ----------
candidates = ["qwen-turbo", "qwen-plus", "qwen-max", "qwen3-coder-plus", "qwen3-max"]
for m in candidates:
    try:
        r = requests.post(
            BASE.rstrip("/") + "/chat/completions", headers=h,
            json={"model": m, "messages": [{"role": "user", "content": "你好"}],
                  "stream": False},
            timeout=20)
        if r.status_code == 200:
            print("\n[2] CHAT OK  model =", m, "->",
                  r.json()["choices"][0]["message"]["content"][:60])
            break
        print("[2] CHAT", m, r.status_code, r.text[:150])
    except Exception as e:
        print("[2] CHAT", m, "FAIL", e)

# ---------- 3) TTS 测试 (cosyvoice-v1) ----------
try:
    import dashscope
    from dashscope.audio.tts_v2 import SpeechSynthesizer, AudioFormat, ResultCallback
    dashscope.api_key = KEY
    got = {"n": 0}

    class CB(ResultCallback):
        def on_open(self): print("\n[3] TTS ws open")
        def on_complete(self): print("[3] TTS complete")
        def on_error(self, message): print("[3] TTS error:", message)
        def on_close(self): print("[3] TTS close")
        def on_data(self, data): got["n"] += len(data)

    s = SpeechSynthesizer(model="cosyvoice-v1", voice="longxiaochun",
                          format=AudioFormat.PCM_16000HZ_MONO_16BIT, callback=CB())
    s.call("你好，这是一个语音合成测试。")
    print("[3] TTS pcm bytes:", got["n"])
except Exception as e:
    print("[3] TTS FAIL:", repr(e))
