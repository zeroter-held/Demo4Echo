#include "sys_manager.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char * sys_config_path = "./system_para.conf";
const char * city_adcode_path = "./gaode_adcode.json";

static void set_string(char *destination, size_t destination_size, const char *source)
{
    if(!destination || destination_size == 0) return;
    strncpy(destination, source ? source : "", destination_size - 1);
    destination[destination_size - 1] = '\0';
}

int sys_set_lcd_brightness(int brightness)
{
    return brightness >= 0 && brightness <= 100 ? 0 : -1;
}

int sys_set_volume(int level)
{
    return level >= 0 && level <= 100 ? 0 : -1;
}

int sys_set_time(int year, int month, int day, int hour, int minute, int second)
{
    (void)year;
    (void)month;
    (void)day;
    (void)hour;
    (void)minute;
    (void)second;
    return 0;
}

void sys_get_time(int *year, int *month, int *day, int *hour, int *minute, int *second)
{
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    if(!local) return;

    if(year) *year = local->tm_year + 1900;
    if(month) *month = local->tm_mon + 1;
    if(day) *day = local->tm_mday;
    if(hour) *hour = local->tm_hour;
    if(minute) *minute = local->tm_min;
    if(second) *second = local->tm_sec;
}

int sys_get_day_of_week(int year, int month, int day)
{
    struct tm date = {0};
    date.tm_year = year - 1900;
    date.tm_mon = month - 1;
    date.tm_mday = day;
    if(mktime(&date) == (time_t)-1) return 0;
    return date.tm_wday;
}

bool sys_get_wifi_status(void)
{
    return true;
}

const char * sys_get_city_name_by_adcode(const char *filepath, const char *target_adcode)
{
    (void)filepath;
    if(target_adcode && strcmp(target_adcode, "440300") == 0) return "深圳市";
    return "东城区";
}

int sys_get_auto_location_by_ip(LocationInfo_t *location, const char *api_key)
{
    (void)api_key;
    if(!location) return -1;
    set_string(location->city, sizeof(location->city), "东城区");
    set_string(location->adcode, sizeof(location->adcode), "110101");
    return 0;
}

int sys_get_time_from_ntp(const char *ntp_server, int *year, int *month, int *day,
                          int *hour, int *minute, int *second)
{
    (void)ntp_server;
    sys_get_time(year, month, day, hour, minute, second);
    return 0;
}

int sys_save_system_parameters(const char *filepath, const system_para_t *params)
{
    FILE *file;
    if(!filepath || !params) return -1;

    file = fopen(filepath, "w");
    if(!file) return -1;

    fprintf(file, "year=%d\n", params->year);
    fprintf(file, "month=%d\n", params->month);
    fprintf(file, "day=%d\n", params->day);
    fprintf(file, "hour=%d\n", params->hour);
    fprintf(file, "minute=%d\n", params->minute);
    fprintf(file, "brightness=%d\n", params->brightness);
    fprintf(file, "sound=%d\n", params->sound);
    fprintf(file, "wifi_connected=%s\n", params->wifi_connected ? "true" : "false");
    fprintf(file, "auto_time=%s\n", params->auto_time ? "true" : "false");
    fprintf(file, "auto_location=%s\n", params->auto_location ? "true" : "false");
    fprintf(file, "city=%s\n", params->location.city);
    fprintf(file, "adcode=%s\n", params->location.adcode);
    fprintf(file, "gaode_api_key=%s\n", params->gaode_api_key);
    fprintf(file, "AIChat_server_url=%s\n", params->aichat_app_info.addr);
    fprintf(file, "AIChat_server_port=%d\n", params->aichat_app_info.port);
    fprintf(file, "AIChat_server_token=%s\n", params->aichat_app_info.token);
    fprintf(file, "AIChat_Client_ID=%s\n", params->aichat_app_info.device_id);
    fprintf(file, "aliyun_api_key=%s\n", params->aichat_app_info.aliyun_api_key);
    fprintf(file, "AIChat_protocol_version=%d\n", params->aichat_app_info.protocol_version);
    fprintf(file, "AIChat_sample_rate=%d\n", params->aichat_app_info.sample_rate);
    fprintf(file, "AIChat_channels=%d\n", params->aichat_app_info.channels);
    fprintf(file, "AIChat_frame_duration=%d\n", params->aichat_app_info.frame_duration);

    fclose(file);
    return 0;
}

static void apply_parameter(system_para_t *params, const char *key, const char *value)
{
    if(strcmp(key, "year") == 0) params->year = atoi(value);
    else if(strcmp(key, "month") == 0) params->month = atoi(value);
    else if(strcmp(key, "day") == 0) params->day = atoi(value);
    else if(strcmp(key, "hour") == 0) params->hour = atoi(value);
    else if(strcmp(key, "minute") == 0) params->minute = atoi(value);
    else if(strcmp(key, "brightness") == 0) params->brightness = (uint16_t)atoi(value);
    else if(strcmp(key, "sound") == 0) params->sound = (uint16_t)atoi(value);
    else if(strcmp(key, "wifi_connected") == 0) params->wifi_connected = strcmp(value, "true") == 0;
    else if(strcmp(key, "auto_time") == 0) params->auto_time = strcmp(value, "true") == 0;
    else if(strcmp(key, "auto_location") == 0) params->auto_location = strcmp(value, "true") == 0;
    else if(strcmp(key, "city") == 0) set_string(params->location.city, sizeof(params->location.city), value);
    else if(strcmp(key, "adcode") == 0) set_string(params->location.adcode, sizeof(params->location.adcode), value);
    else if(strcmp(key, "gaode_api_key") == 0) set_string(params->gaode_api_key, sizeof(params->gaode_api_key), value);
    else if(strcmp(key, "AIChat_server_url") == 0) set_string(params->aichat_app_info.addr, sizeof(params->aichat_app_info.addr), value);
    else if(strcmp(key, "AIChat_server_port") == 0) params->aichat_app_info.port = atoi(value);
    else if(strcmp(key, "AIChat_server_token") == 0) set_string(params->aichat_app_info.token, sizeof(params->aichat_app_info.token), value);
    else if(strcmp(key, "AIChat_Client_ID") == 0) set_string(params->aichat_app_info.device_id, sizeof(params->aichat_app_info.device_id), value);
    else if(strcmp(key, "aliyun_api_key") == 0) set_string(params->aichat_app_info.aliyun_api_key, sizeof(params->aichat_app_info.aliyun_api_key), value);
    else if(strcmp(key, "AIChat_protocol_version") == 0) params->aichat_app_info.protocol_version = atoi(value);
    else if(strcmp(key, "AIChat_sample_rate") == 0) params->aichat_app_info.sample_rate = atoi(value);
    else if(strcmp(key, "AIChat_channels") == 0) params->aichat_app_info.channels = atoi(value);
    else if(strcmp(key, "AIChat_frame_duration") == 0) params->aichat_app_info.frame_duration = atoi(value);
}

int sys_load_system_parameters(const char *filepath, system_para_t *params)
{
    FILE *file;
    char line[256];

    if(!filepath || !params) return -1;
    file = fopen(filepath, "r");
    if(!file) return -1;

    while(fgets(line, sizeof(line), file)) {
        char key[128];
        char value[128];
        if(sscanf(line, "%127[^=]=%127s", key, value) == 2) {
            apply_parameter(params, key, value);
        }
    }

    fclose(file);
    return 0;
}
