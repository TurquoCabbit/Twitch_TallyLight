#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include <esp_task_wdt.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include <TFT_eSPI.h>


const char versionControl[] = "1.2";

#define LOG_MAX_LENGHT    128

#define FEATURE_SERIAL_ON
#define FEATURE_LCD_ON


#include "./user_data.h"
/*  Enter your own infomation here or put them into ./user_data.h
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* twitch_client_id = "YOUR_TWITCH_CLIENT_ID";
const char* twitch_client_secret = "YOUR_TWITCH_CLIENT_SECRET";
const char* streamer_login = "STREAMER_NAME"; // lowercase Twitch username
*/

#define POLLING_DELAY_TIME_ms   (10000)     // Update status every 10s
#define TOKEN_RE_GET_TIME_s     (14400)     // Get token every 4 hours
#define TOKEN_RE_GET_TIME_CNT   (TOKEN_RE_GET_TIME_s / (POLLING_DELAY_TIME_ms / 1000))

static String accessToken;
static int tokenReGetCnt;
static int getAccessToken();
static int isStreamerLive();

static TFT_eSPI tft = TFT_eSPI();


static int dbgPrintf(const char * format,...) {
#ifdef FEATURE_SERIAL_ON
    char InputBuff[LOG_MAX_LENGHT];

    va_list    args;
    va_start(args, format);
    vsprintf(InputBuff, format, args);
    va_end(args);

    Serial.write(InputBuff);
    
#endif
    return 0;
}

static void tft_backLight_pwm_init(uint8_t chan, double freq) {
#ifdef FEATURE_LCD_ON

    ledcSetup(chan, freq, 8);
    ledcAttachPin(TFT_BL, chan);
    ledcWrite(0, 127);

#endif
}

static void tftBackLight_pwmDuty_set(uint8_t percentage) {
#ifdef FEATURE_LCD_ON
    uint32_t duty;

    duty = percentage * 255 / 100;

    ledcWrite(0, percentage);

#endif
}

static void tft_fillScreen_color_set(uint32_t color) {
#ifdef FEATURE_LCD_ON
    tft.fillScreen(color);
#endif
}


static void tft_time_print(struct tm *timeinfo) {
#ifdef FEATURE_LCD_ON
    tft.setCursor(35, 40);
    tft.setTextColor(TFT_BLACK);
    tft.setTextSize(6);
    tft.printf("%02d", timeinfo->tm_hour);
    
    tft.setCursor(35, 100);
    tft.printf("%02d", timeinfo->tm_min);

    tft.setCursor(35, 160);
    tft.printf("%02d", timeinfo->tm_sec);
#endif
}

#define TALLY_LIGHT_STA_ERROR           -1
#define TALLY_LIGHT_STA_NO_CONNECTED    0
#define TALLY_LIGHT_STA_OFFLINE         1
#define TALLY_LIGHT_STA_ONAIR           2
static void tallyLight_stat_set(int sta) {

    switch (sta) {
        case TALLY_LIGHT_STA_OFFLINE:
            tftBackLight_pwmDuty_set(30);
            tft_fillScreen_color_set(TFT_GREEN);
            dbgPrintf("tallyLight:OFFLINE\n");
            break;
        
        case TALLY_LIGHT_STA_ONAIR:
            tftBackLight_pwmDuty_set(100);
            tft_fillScreen_color_set(TFT_RED);
            dbgPrintf("tallyLight:ON AIR\n");
            break;
            
        case TALLY_LIGHT_STA_ERROR:
            tftBackLight_pwmDuty_set(100);
            tft_fillScreen_color_set(TFT_YELLOW);
            dbgPrintf("tallyLight:ERROR\n");
            break;
        
        case TALLY_LIGHT_STA_NO_CONNECTED:
        default:
            tftBackLight_pwmDuty_set(30);
            tft_fillScreen_color_set(TFT_BLUE);
            dbgPrintf("tallyLight:NO CONNECTED\n");
            break;
    }
}

void setup() {

#ifdef FEATURE_SERIAL_ON
    Serial.begin(115200);
#endif
    dbgPrintf("-------------------------------------------------\n");
    dbgPrintf("Start up! version:%s\n", versionControl);
    dbgPrintf("-------------------------------------------------\n");

    delay(100);
    
#ifdef FEATURE_LCD_ON
    tft.begin();
    tft.fillScreen(TFT_BLACK);

    tft_backLight_pwm_init(0, 5000);
#endif

    tallyLight_stat_set(TALLY_LIGHT_STA_NO_CONNECTED);

    WiFi.begin(ssid, password);
    dbgPrintf("Connecting to WiFi [%s] ...\n", ssid);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);    // 1s
        dbgPrintf(".\n");
    }
    dbgPrintf("Connected to [%s] successfully!\n", ssid);
    
    configTime(8 * 3600, 0, "pool.ntp.org");
}

void loop() {
    static int isTokenValid = 0;
    static int isOnAir = 0;

    struct tm timeinfo;

    if (tokenReGetCnt <= 0) {
        // for hours
        isTokenValid = getAccessToken();
        
        tokenReGetCnt = TOKEN_RE_GET_TIME_CNT;

        if (isTokenValid == -1) {
            tallyLight_stat_set(TALLY_LIGHT_STA_ERROR);
            tokenReGetCnt = 5;
        }

        configTime(8 * 3600, 0, "pool.ntp.org");
    }
    
    if (isTokenValid == 1) {
        isOnAir = isStreamerLive();

        if (isOnAir == 1) {
            tallyLight_stat_set(TALLY_LIGHT_STA_ONAIR);
        } else if (isOnAir == 0) {
            tallyLight_stat_set(TALLY_LIGHT_STA_OFFLINE);
        } else if (isOnAir == -1) {
            tallyLight_stat_set(TALLY_LIGHT_STA_ERROR);
            isTokenValid = 0;
        }
    }
    
    if (getLocalTime(&timeinfo)) {
        dbgPrintf("%02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        tft_time_print(&timeinfo);
    }

    tokenReGetCnt --;
    dbgPrintf("tokenReGetCnt:%d\n", tokenReGetCnt);

    delay(POLLING_DELAY_TIME_ms); // Check every 10 seconds
}

static int getAccessToken() {
    int ret;

    HTTPClient http;
    String url = "https://id.twitch.tv/oauth2/token?client_id=" + String(twitch_client_id) +
                "&client_secret=" + String(twitch_client_secret) +
                "&grant_type=client_credentials";

    ret = -1;

    http.begin(url);
    int httpCode = http.POST("");
    dbgPrintf("getAccessToken httpCode:%d\n", httpCode);

    if (httpCode == 200) {
        String payload = http.getString();

        DynamicJsonDocument doc(512);
        deserializeJson(doc, payload);
        accessToken = doc["access_token"].as<String>();

        ret = 1;
    } else {
        tokenReGetCnt = 0;
        ret = -1;
    }

    http.end();

    return ret;
}

static int isStreamerLive() {
    int ret;
    
    HTTPClient http;
    String url = "https://api.twitch.tv/helix/streams?user_login=" + String(streamer_login);

    ret = -1;

    http.begin(url);
    http.addHeader("Client-ID", twitch_client_id);
    http.addHeader("Authorization", "Bearer " + accessToken);

    int httpCode = http.GET();
    dbgPrintf("[%s] isStreamerLive httpCode:%d\n",streamer_login, httpCode);
    
    if (httpCode == 200) {
        String payload = http.getString();

        DynamicJsonDocument doc(1024);
        deserializeJson(doc, payload);
        JsonArray data = doc["data"].as<JsonArray>();
        
        if (data.size() == 0) {
            ret = 0;
        } else {
            ret = 1;
        }

    } else {
        tokenReGetCnt = 0;
        ret -1;
    }

    http.end();
    return ret;
}

