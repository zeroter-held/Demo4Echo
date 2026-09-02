#include <stdint.h>
#include <stdio.h>

#include "app_ChatBotPage.h"

uint8_t chat_bot_move_dir = 0;

static int ai_chat_running = 0;

int start_ai_chat(const char *address, int port, const char *token, const char *deviceId,
                  const char *aliyun_api_key, int protocolVersion, int sample_rate,
                  int channels, int frame_duration)
{
    (void)token;
    (void)deviceId;
    (void)aliyun_api_key;
    (void)protocolVersion;
    (void)sample_rate;
    (void)channels;
    (void)frame_duration;
    ai_chat_running = 1;
    printf("[SIM] AIChat connected to %s:%d\n", address ? address : "localhost", port);
    return 0;
}

int stop_ai_chat(void)
{
    ai_chat_running = 0;
    chat_bot_move_dir = 0;
    return 0;
}

int get_ai_chat_state(void)
{
    return ai_chat_running ? 3 : -1;
}

void chat_bot_get_intent_process(void)
{
}
