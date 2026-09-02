#include "app_WeatherPage.h"

#include <stdio.h>
#include <string.h>

int get_weather_info_by_adcode(const char *adcode, WeatherInfo_t *weather_info)
{
    (void)adcode;
    if(!weather_info) return -1;

    snprintf(weather_info->weather, sizeof(weather_info->weather), "%s", "多云");
    snprintf(weather_info->temperature, sizeof(weather_info->temperature), "%s", "24");
    snprintf(weather_info->humidity, sizeof(weather_info->humidity), "%s", "55");
    snprintf(weather_info->windpower, sizeof(weather_info->windpower), "%s", "≤3");
    return 0;
}
