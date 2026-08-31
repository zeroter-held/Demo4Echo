import asyncio
import argparse
import os
from ws_server import WebSocketServer
from threads.tts_thread import TTSGenerateThread
from threads.audio_send_thread import AudioSendThread
from tools.logger import logger
from service_manager import ServiceManager
from config.settings import global_settings
import sys
sys.path.append("..")

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--access_token",
        default=os.getenv("AICHAT_ACCESS_TOKEN", "123456"),
        help="WebSocket client access token",
    )
    parser.add_argument(
        "--aliyun_api_key",
        default=os.getenv("DASHSCOPE_API_KEY"),
        help="Alibaba Cloud Model Studio API key",
    )
    return parser.parse_args()


async def main(access_token, aliyun_api_key):
    if not aliyun_api_key:
        raise ValueError("Missing Aliyun API key. Set DASHSCOPE_API_KEY or pass --aliyun_api_key.")

    global_settings.Set_API_Key(aliyun_api_key)

    # 初始化vad asr chat intent tts服务
    service_manager = ServiceManager()

    # 启动 TTS 生成线程
    tts_generate_thread = TTSGenerateThread(service_manager)
    # tts_generate_thread.start()

    # 启动 audio 数据发送线程
    tts_send_thread = AudioSendThread(service_manager)
    tts_send_thread.start()

    # 启动 WebSocket 服务器
    server = WebSocketServer(host="0.0.0.0", port=8000, access_token=access_token, service_manager=service_manager)
    try:
        await server.start_server()
    except KeyboardInterrupt:
        logger.info("\n服务器正在关闭...")
    finally:
        # 停止线程
        service_manager.stop_event.set()  # 设置停止事件
        # tts_generate_thread.join()
        tts_send_thread.join()
        logger.info("服务器已关闭。")

if __name__ == "__main__":
    args = parse_args()
    try:
        asyncio.run(main(args.access_token, args.aliyun_api_key))
    except KeyboardInterrupt:
        logger.info("程序已被用户中断")
    finally:
        # 确保事件循环关闭
        try:
            loop = asyncio.get_event_loop()
            if loop.is_running():
                loop.stop()
        except RuntimeError:
            pass
        logger.info("事件循环已关闭")
