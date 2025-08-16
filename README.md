## Twitch Streamer Tally light for TTGO T-display with 1.14" LCD

Auto update Twitch streamer live status use Twitch API. The live status will showed on LCD as full screen color.

|Color|Status|Note|
|:--:|:--:|:--|
|🟥|On Air|Streamer is live|
|🟩|Off Line|Streamer off line|
|🟨|Error|Some thing went wrong on http client, Check live log for httpcode via comport (115200 8 n 1)|
|🟦|Power On|Power on and start connect to Wifi, If it stay in this status means Wifi connect fail|

---
### How to use
1. Enter your own Wifi and Twitch developer informations, then the channel you would like to monitor.

        const char* ssid = "YOUR_WIFI_SSID";
        const char* password = "YOUR_WIFI_PASSWORD";
        const char* twitch_client_id = "YOUR_TWITCH_CLIENT_ID";
        const char* twitch_client_secret = "YOUR_TWITCH_CLIENT_SECRET";
        const char* streamer_login = "STREAMER_NAME"; // lowercase Twitch username

2. Delete this local include. Or you can just put the informations above into this file
  
        #include "user_data.h"

3. Last update time of the streamer status will show on LCD screen to indicate the device is still working.

---

*20250815 version 1.0*

|Hardware|version
|:--|:--|
|TTGO T-display|[Lilygo](https://lilygo.cc/products/lilygo%C2%AE-ttgo-t-display-1-14-inch-lcd-esp32-control-board)|

|Auduino Platform|version|
|:--|:--|
|esp32:esp32|2.0.0|

|Auduino Libs|version|
|:--|:--|
|WiFi             |2.0.0|
|HTTPClient       |2.0.0|
|WiFiClientSecure |2.0.0|
|ArduinoJson      |7.4.2|
|TFT_eSPI         |2.3.4|
|SPI              |2.0.0|
|FS               |2.0.0|
|SPIFFS           |2.0.0|
