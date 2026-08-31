import os

import dashscope

class Settings:
    protocol_version = 2

    # ==== LLM 接入配置 (OpenAI 兼容协议) ====
    # 任何提供 OpenAI 兼容接口的服务 (百炼兼容地址 / Token Plan 专属地址 / 第三方中转站)
    # 都可以通过这 4 个环境变量接入, 无需改代码:
    #   LLM_BASE_URL     接口地址 (以 /v1 结尾的 base url)
    #   LLM_API_KEY      密钥 (sk- / sk-ws- / 中转站密钥均可)
    #   LLM_CHAT_MODEL   对话模型名
    #   LLM_INTENT_MODEL 意图识别模型名
    LLM_BASE_URL = os.getenv("LLM_BASE_URL", "https://dashscope.aliyuncs.com/compatible-mode/v1")
    LLM_API_KEY = os.getenv("LLM_API_KEY", os.getenv("DASHSCOPE_API_KEY", ""))
    INTENT_MODEL = os.getenv("LLM_INTENT_MODEL", "qwen-turbo")  # 专门用于意图识别
    CHAT_MODEL = os.getenv("LLM_CHAT_MODEL", "qwen-turbo")      # 用于常规对话

    # 兼容旧代码: cosyvoice TTS 仍走 dashscope SDK, 它读取 dashscope.api_key
    dashscope.api_key = LLM_API_KEY

    # device
    ASR_DEVICE = "cpu"            # ASR 模型使用的设备
    # ASR_DEVICE = "cuda"         # ASR 模型使用的设备
    VAD_DEVICE = "cpu"            # VAD 模型使用的设备

    # 超时设置
    API_TIMEOUT = int(os.getenv("LLM_API_TIMEOUT", "30"))  # 秒

    def Set_API_Key(self, aliyun_api_key):
        """板端 hello 消息会携带 aliyun_api_key。
        规则: dashscope(TTS) 始终跟随板端; 但 LLM 密钥只有在服务器端
        未通过环境变量配置时才允许被板端覆盖, 防止板端 conf 里的旧 key
        把服务器端配好的正确 key 覆盖掉。"""
        if aliyun_api_key and aliyun_api_key not in ("your_aliyun_key", "null", "None"):
            dashscope.api_key = aliyun_api_key
            if not Settings.LLM_API_KEY:
                Settings.LLM_API_KEY = aliyun_api_key

global_settings = Settings()
