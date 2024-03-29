#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <NTPClient.h>
#include <FS.h>
#include <ArduinoJson.h>

#include "Font_11x15.h"
#include "Font_5x7.h"
#include "Font_8x8_Icons.h"
#include "SSD1306_SWI2C.h"
#include "SSD1306_Utils.h"
#include "WebServer.h"

#define P_SDA 5
#define P_SCL 4
#define P_LED 16
#define P_BTN 13

#define P_SWI 15
#define BP_SWI 0x00008000

#define GPIO_ON_ADDR 0x60000304
#define GPIO_OFF_ADDR 0x60000308

#define ALARM_FILE_NAME "sys_alarms.json"
#define ALARM_COUNT 7
#define NUM_LEDS 72
#define NUM_LED_COLORS (NUM_LEDS * 4)
#define DISPLAY_AUTOOFFDELAY 30000
#define SUNRISE_DURATION (60 * 60 * 1000 / INTERVAL_ALARMVISUALS)
#define SUNRISE_TICKSPERCOLOUR (SUNRISE_DURATION / 30)
#define FLASH_ONTICKS 1
#define FLASH_OFFTICKS 2
#define FLASH_FLASHES 2
#define FLASH_PERIODTICKS 10

#define INTERVAL_OTA 1000
#define INTERVAL_CONNCHECK 5000
#define INTERVAL_DISPLAYREFRESH 20
#define INTERVAL_PIXELBLINK 250
#define INTERVAL_TIMEUPDATE 20000
#define INTERVAL_TIMEDRAW 500
#define INTERVAL_ALARMCHECK 10000
#define INTERVAL_ALARMVISUALS 100
#define INTERVAL_WEBSERVER 25
#define INTERVAL_LEDUPDATE 32
#define INTERVAL_COLOURCYCLE 1
#define INTERVAL_BUTTONCHECK 100

#define CURRENT_LIMIT_500
//define CURRENT_LIMIT_2500

// create a WifiPassword.h file that defines WIFI_PASSWORD & declares ssid & pass
#if __has_include("WifiPassword.h")
#include "WifiPassword.h"
#endif
#ifndef WIFI_PASSWORD
const char *ssid = "";
const char *pass = "";
#endif

const char *ntpServer = "192.168.1.1";
const long utcOffsetInSeconds = 7200L;

extern "C" void ICACHE_RAM_ATTR swi_write_ext(uint8_t *data, uint16_t len, uint8_t repeat);

SSD1306 *screen;
Font_5x7 *small_font = new Font_5x7();
Font_11x15 *medium_font = new Font_11x15();
Font_8x8_Icons *icon_font = new Font_8x8_Icons();
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, utcOffsetInSeconds);
WiFiServer server(80);
WebServer webserver(&server);

//G, R, B, W
uint8_t led_colours[NUM_LED_COLORS];

bool alarming = false;
bool activityPixelState = false;
volatile bool buttonPressed = false;
bool colorCycleEnabled = false;
bool displayAutoOff = false;
bool displayOn = true;
bool displayRefreshNeeded = false;
bool sunriseComplete = false;
bool timeUpdateSuccess = false;
uint32_t alarming_remainder = 0;
uint8_t alarming_alarm = ALARM_COUNT;
uint8_t buttonPressedCount = 0;
uint16_t colourCycle_currentIndex = 0;
int32_t connectionRetryCount = 1;
uint32_t displayLastActivity = 0;
uint8_t displayResyncCounter = 0;
uint8_t flashCounter = 0;
uint32_t sunriseRemaining = 0;
uint8_t torching = 0;

uint32_t last_OTA = 0;
uint32_t last_ConnCheck = 0;
uint32_t last_displayRefresh = 0;
uint32_t last_pixelBlink = 0;
uint32_t last_timeUpdate = 0;
uint32_t last_timeDraw = 0;
uint32_t last_alarmCheck = 0;
uint32_t last_alarmVisuals = 0;
uint32_t last_webServerUpdate = 0;
uint32_t last_ledUpdate = 0;
uint32_t last_colorCycle = 0;
uint32_t last_buttonCheck = 0;

ApiMethod *GetMethods;
ApiMethod *PostMethods;

class Alarm
{
public:
  bool Enabled;
  uint8_t Hour;
  uint8_t Minute;
  uint8_t Duration;
  uint8_t RepeatDays;
  bool EnabledForDay(uint8_t day) { return 1 & (RepeatDays >> (day > 6 ? 6 : day)); }
  bool SingleShot() { return RepeatDays == 0; }
};

Alarm *alarms = new Alarm[ALARM_COUNT];

// G,R,B,W
#if defined(CURRENT_LIMIT_500)
const uint8_t sunrise_colours[31][4] = {
    {0, 0, 0, 0},
    {0, 0, 3, 0},
    {0, 0, 8, 0},
    {0, 0, 13, 0},
    {0, 1, 19, 0},
    {0, 2, 24, 0},
    {0, 4, 33, 0},
    {0, 8, 44, 0},
    {0, 14, 52, 0},
    {0, 23, 59, 0},
    {0, 34, 65, 0},
    {0, 44, 69, 2},
    {0, 55, 70, 3},
    {0, 57, 71, 4},
    {0, 56, 59, 7},
    {0, 67, 38, 11},
    {0, 77, 17, 14},
    {25, 89, 0, 9},
    {45, 96, 0, 2},
    {55, 94, 0, 4},
    {47, 75, 0, 19},
    {43, 51, 0, 33},
    {30, 26, 0, 46},
    {15, 4, 0, 57},
    {14, 1, 12, 58},
    {14, 1, 26, 58},
    {18, 1, 32, 69},
    {22, 0, 37, 83},
    {31, 1, 46, 98},
    {39, 0, 54, 117},
    {52, 2, 67, 138},
};
#else
const uint8_t sunrise_colours[31][4] = {
    {0, 0, 0, 0},
    {0, 0, 9, 0},
    {0, 0, 21, 0},
    {0, 0, 35, 0},
    {0, 2, 49, 0},
    {0, 5, 63, 0},
    {0, 10, 88, 0},
    {0, 20, 116, 0},
    {0, 37, 137, 0},
    {0, 61, 156, 0},
    {0, 89, 170, 0},
    {0, 117, 180, 3},
    {0, 146, 184, 6},
    {0, 150, 186, 9},
    {1, 147, 154, 15},
    {0, 175, 100, 23},
    {1, 202, 41, 30},
    {67, 234, 0, 18},
    {120, 254, 0, 2},
    {147, 249, 0, 7},
    {133, 202, 0, 37},
    {127, 146, 0, 64},
    {92, 79, 0, 93},
    {59, 25, 1, 114},
    {42, 1, 22, 123},
    {43, 1, 58, 123},
    {57, 2, 70, 146},
    {72, 1, 83, 174},
    {93, 0, 99, 207},
    {121, 0, 118, 246},
    {255, 127, 218, 255},
};
#endif

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting");

  pinMode(P_LED, OUTPUT);
  digitalWrite(P_LED, HIGH);
  pinMode(P_SWI, OUTPUT);
  WRITE_PERI_REG(GPIO_OFF_ADDR, BP_SWI);

  pinMode(P_BTN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(P_BTN), isr_buttonStateChange, CHANGE);

  screen = new SSD1306(P_SDA, P_SCL);
  screen->clear_buffer();
  SSD1306_Utils::write_string(screen, 0, 0, small_font, "Booting");
  screen->refresh();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  Serial.println("Connecting Wifi");
  screen->clear_buffer();
  SSD1306_Utils::write_string(screen, 0, 0, small_font, "Connecting");
  screen->refresh();

  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    screen->clear_buffer();
    SSD1306_Utils::write_string(screen, 0, 0, small_font, "Connection Failed!");
    SSD1306_Utils::write_string(screen, 0, 12, small_font, "Rebooting...");
    screen->refresh();

    delay(5000);
    ESP.restart();
  }

  Serial.print("Connected, IP Address is: ");
  Serial.println(WiFi.localIP());

  Serial.print("Provisioning OTA...");
  setup_ota();
  Serial.println(" done.");

  Serial.print("Setting up Webserver Callbacks...");
  setup_webserver();
  Serial.println(" done.");

  timeClient.begin();
  ntpUDP.setTimeout(75);
  server.begin();

  Serial.print("FS init...");
  if (SPIFFS.begin())
  {
    Serial.println(" OK");
  }
  else
  {
    Serial.print(" failed, formatting...");
    SPIFFS.format();
    SPIFFS.begin() ? Serial.println(" OK") : Serial.println(" failed.");
  }

  setup_alarms();

  Serial.println("Booted.\r\n");
}
void setup_alarms()
{
  if (SPIFFS.exists(ALARM_FILE_NAME))
  {
    auto f = SPIFFS.open(ALARM_FILE_NAME, "r");
    String json = f.readString();
    f.close();
    deserializeAlarms(json);
  }
}
void setup_ota()
{
  static ulong last_OTA_ScreenRefresh = 0;
  ArduinoOTA.onStart([]() {
    Serial.println("OTA Start");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA End");
    screen->clear_buffer();
    SSD1306_Utils::write_string(screen, 0, 0, small_font, "Applying Update...");
    screen->refresh();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));

    if (millis() > last_OTA_ScreenRefresh + 200)
    {
      screen->clear_buffer();
      SSD1306_Utils::write_string(screen, 0, 0, small_font, "Updating: " + String(progress / (total / 100)));
      screen->refresh();
      last_OTA_ScreenRefresh = millis();
    }
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);

    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
      Serial.println("End Failed");
  });

  ArduinoOTA.begin();
}
void setup_webserver()
{
  GetMethods = new ApiMethod[4];

  GetMethods[0].Path = "admin/cycle";
  GetMethods[0].Callback = api_getColourCycle;

  GetMethods[1].Path = "admin/leds";
  GetMethods[1].Callback = api_getLeds;

  GetMethods[2].Path = "alarms";
  GetMethods[2].Callback = api_getAlarms;

  GetMethods[3].Path = "debug/state";
  GetMethods[3].Callback = api_getState;

  webserver.SetGetHandlers(GetMethods, 4);

  PostMethods = new ApiMethod[7];

  PostMethods[0].Path = "alarms";
  PostMethods[0].Callback = api_setAlarms;

  PostMethods[1].Path = "admin/cycle";
  PostMethods[1].Callback = api_toggleColourCycle;

  PostMethods[2].Path = "admin/resetleds";
  PostMethods[2].Callback = api_resetLeds;

  PostMethods[3].Path = "admin/leds";
  PostMethods[3].Callback = api_setLeds;

  PostMethods[4].Path = "admin/format";
  PostMethods[4].Callback = api_format;

  PostMethods[5].Path = "admin/testAlarmOn";
  PostMethods[5].Callback = api_testAlarmOn;

  PostMethods[6].Path = "admin/testAlarmOff";
  PostMethods[6].Callback = api_testAlarmOff;

  webserver.SetPostHandlers(PostMethods, 7);
}
ICACHE_RAM_ATTR void isr_buttonStateChange()
{
  buttonPressed = !digitalRead(P_BTN);
}

void loop()
{
  uint32_t now = millis();

  if (now - last_ConnCheck >= INTERVAL_CONNCHECK)
    check_connectivity();

  if (now - last_pixelBlink >= INTERVAL_PIXELBLINK)
    pixel_blink();

  if (now - last_OTA >= INTERVAL_OTA)
    check_ota();

  if (now - last_colorCycle >= INTERVAL_COLOURCYCLE)
    colour_cycle();

  if (now - last_timeUpdate >= INTERVAL_TIMEUPDATE)
    time_update();

  if (now - last_timeDraw >= INTERVAL_TIMEDRAW)
    time_draw();

  if (now - last_alarmCheck >= INTERVAL_ALARMCHECK)
    check_alarms();

  if (now - last_alarmVisuals >= INTERVAL_ALARMVISUALS)
    alarm_visuals();

  if (now - last_webServerUpdate >= INTERVAL_WEBSERVER)
    check_webserver();

  if (now - last_ledUpdate >= INTERVAL_LEDUPDATE)
    led_update();

  if (now - last_buttonCheck >= INTERVAL_BUTTONCHECK)
    button_check();

  if (now - last_displayRefresh >= INTERVAL_DISPLAYREFRESH)
    display_refresh();
}
void check_connectivity()
{
  if (!WiFi.isConnected() || WiFi.localIP().toString().startsWith("169.254"))
  {
    connectionRetryCount++;
    SSD1306_Utils::write_string(screen, 12, 0, small_font, "Retry " + String(connectionRetryCount));
    screen->refresh();

    WiFi.reconnect();
    if (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
      SSD1306_Utils::write_string(screen, 12, 0, small_font, "Failed " + String(connectionRetryCount));
      screen->refresh();

      last_ConnCheck = millis();
    }
  }
  else
  {
    if (connectionRetryCount != 0)
    {
      connectionRetryCount = 0;
      screen->clear_buffer();
      SSD1306_Utils::write_char(screen, 0, 0, icon_font, (char)Icons::Wifi); // Wifi Logo
      SSD1306_Utils::write_string(screen, 12, 0, small_font, WiFi.localIP().toString());
      screen->refresh();
    }

    last_ConnCheck = millis();
  }
}
void pixel_blink()
{
  activityPixelState = !activityPixelState;
  screen->set_pixel(127, 0, activityPixelState);
  displayRefreshNeeded = true;

  last_pixelBlink = millis();
}
void check_ota()
{
  ArduinoOTA.handle();

  last_OTA = millis();
}

void colour_cycle()
{
  if (!colorCycleEnabled)
    return;

  if (led_colours[colourCycle_currentIndex] == 255)
  {
    led_colours[colourCycle_currentIndex] = 0;
    ++colourCycle_currentIndex %= NUM_LED_COLORS;
  }
  led_colours[colourCycle_currentIndex] += 1;

  SSD1306_Utils::write_string(screen, 109, 16, small_font, "   ");
  SSD1306_Utils::write_string(screen, 109, 16, small_font, String(colourCycle_currentIndex));
  SSD1306_Utils::write_string(screen, 109, 24, small_font, "   ");
  SSD1306_Utils::write_string(screen, 109, 24, small_font, String(led_colours[colourCycle_currentIndex]));
  displayRefreshNeeded = true;

  last_colorCycle = millis();
}
void time_update()
{
  timeUpdateSuccess = timeClient.update();

  if (timeClient.getHours() <= 6 || timeClient.getHours() >= 21) // 9pm - 6am
    displayAutoOff = true;
  else
    displayAutoOff = false;

  last_timeUpdate = millis();
}
void time_draw()
{
  char timeString[8];
  sprintf(timeString, "%02d:%02d:%02d", timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds());
  bool anyAlarmEnabled = false;
  for (uint8_t i = 0; i < ALARM_COUNT; i++)
    if (alarms[i].Enabled)
      anyAlarmEnabled = true;

  SSD1306_Utils::write_string(screen, 12, 12, medium_font, timeString);
  SSD1306_Utils::write_char(screen, 0, 10, icon_font, (char)(anyAlarmEnabled || alarming ? alarming ? Icons::BellRinging : Icons::Bell : Icons::Empty));
  SSD1306_Utils::write_char(screen, 0, 19, icon_font, (char)(timeUpdateSuccess ? Icons::Empty : Icons::Network));

  displayRefreshNeeded = true;
  last_timeDraw = millis();
}
void check_webserver()
{
  bool serving = webserver.handle();

  SSD1306_Utils::write_char(screen, 109, 0, icon_font, (char)(Icons::Computer));
  if (serving)
  {
    SSD1306_Utils::write_char(screen, 100, 0, icon_font, (char)(Icons::ActivityLeft));
    SSD1306_Utils::write_char(screen, 118, 0, icon_font, (char)(Icons::ActivityRight));
    displayRefreshNeeded = true;
  }
  else
  {
    SSD1306_Utils::write_char(screen, 100, 0, icon_font, (char)(Icons::Empty));
    SSD1306_Utils::write_char(screen, 118, 0, icon_font, (char)(Icons::Empty));
  }

  last_webServerUpdate = millis();
}
void led_update()
{
  delay(0);

  os_intr_lock();

  swi_write_ext(led_colours, NUM_LED_COLORS, 1);

  os_intr_unlock();

  last_ledUpdate = millis();
}
void check_alarms()
{
  if (!alarming)
  {
    // loop through alarms, check and trigger if needed
    for (uint8_t i = 0; i < ALARM_COUNT && !alarming; i++)
    {
      auto alarm = &alarms[i];

      if (alarm->Enabled && alarm->Hour == timeClient.getHours() && alarm->Minute == timeClient.getMinutes())
      {
        if (alarm->SingleShot())
        {
          Serial.printf("Single shot alarm triggered:\r\n  i: %d %d:%d %dm\r\n", i, timeClient.getHours(), timeClient.getMinutes(), alarm->Duration == 0 ? 5 : alarm->Duration * 15);
          alarming = true;
          alarm->Enabled = false;
          saveAlarms();
        }
        else if (alarm->EnabledForDay(timeClient.getDay()))
        {
          Serial.printf("Repeating alarm triggered:\r\n i: %d %d:%d %dm %d%d%d%d%d%d%d\r\n", i, timeClient.getHours(), timeClient.getMinutes(), alarm->Duration == 0 ? 5 : alarm->Duration * 15, alarm->EnabledForDay(0), alarm->EnabledForDay(1), alarm->EnabledForDay(2), alarm->EnabledForDay(3), alarm->EnabledForDay(4), alarm->EnabledForDay(5), alarm->EnabledForDay(6));
          alarming = true;
        }

        if (alarming)
        {
          alarming_alarm = i;
          alarming_remainder = alarm->Duration * 15;
          if (alarming_remainder == 0)
            alarming_remainder = 5;
          alarming_remainder = (alarming_remainder * 60 * 1000) / INTERVAL_ALARMCHECK;
        }
      }
    }
  }
  else
  {
    if (--alarming_remainder == 0)
    {
      alarming = false;
      Serial.println("Alarm ended");
    }
  }

  last_alarmCheck = millis();
}
void alarm_visuals()
{
  if (alarming)
  {
    if (alarms[alarming_alarm].Duration < 4) // minimum sunrise is 1 hour, flash if less
      alarm_flash();
    else
      alarm_sunrise();
  }
  else if (alarming_alarm != ALARM_COUNT) // end of the alarm (no longer alarming but not reset)
  {
    resetLeds();
    sunriseComplete = false;
    sunriseRemaining = 0;

    alarming_alarm = ALARM_COUNT;
  }

  last_alarmVisuals = millis();
}
void button_check()
{
  if (buttonPressed)
    buttonPressedCount++;
  else if (buttonPressedCount)
  {
    buttonPressedCount = 0;
  }

  if (buttonPressedCount)
  {
    displayLastActivity = millis();
    SSD1306_Utils::write_char(screen, 109, 12, icon_font, (char)Icons::Button);
    SSD1306_Utils::write_string(screen, 109, 24, small_font, String(buttonPressedCount));
  }
  else
  {
    SSD1306_Utils::write_char(screen, 109, 12, icon_font, (char)Icons::Empty);
    SSD1306_Utils::write_string(screen, 109, 24, small_font, "   ");
  }

  if (alarming)
  {
    if (buttonPressedCount >= (2500 / INTERVAL_BUTTONCHECK)) // 2.5 seconds to cancel alarm
    {
      alarming_remainder = 0;
      alarming = false;
      buttonPressedCount = 0;
    }
  }
  else
  {
    if (buttonPressedCount == 3 && torching) // 300ms, reset torch
    {
      resetLeds();
      torching = 0;
    }
    if (buttonPressedCount && buttonPressedCount % 5 == 0) // every 500ms, increment torch
    {
      torching++;
      setTorch();
    }
  }

  last_buttonCheck = millis();
}
void display_refresh()
{
  if (displayAutoOff)
  {
    if (displayOn)
    {
      if (millis() > displayLastActivity + DISPLAY_AUTOOFFDELAY)
      {
        displayOn = false;
        screen->display_off();
      }
    }
    else
    {
      if (millis() < displayLastActivity + DISPLAY_AUTOOFFDELAY)
      {
        displayOn = true;
        screen->reinitialise();
      }
    }
  }
  else if (!displayOn)
  {
    displayOn = true;
    screen->reinitialise();
  }

  if (displayResyncCounter == 0 && displayOn)
    screen->resynchronize();
  if (displayRefreshNeeded && displayOn)
    screen->refresh();

  displayRefreshNeeded = false;
  displayResyncCounter++;
  last_displayRefresh = millis();
}

ApiMethodResponse api_getColourCycle(String &requestBody)
{
  ApiMethodResponse response;

  const size_t capacity = JSON_OBJECT_SIZE(1);
  DynamicJsonDocument doc(capacity);

  doc["colorCycle"] = colorCycleEnabled;

  serializeJson(doc, response.Body);
  response.Type = ResponseType::Json;
  return response;
}
ApiMethodResponse api_toggleColourCycle(String &requestBody)
{
  colorCycleEnabled = !colorCycleEnabled;

  return ApiMethodResponse();
}
ApiMethodResponse api_resetLeds(String &requestBody)
{
  resetLeds();

  return ApiMethodResponse();
}
ApiMethodResponse api_testAlarmOn(String &requestBody)
{
  alarming = 1;
  alarming_alarm = 0;
  alarming_remainder = 90;
  alarming_remainder = (alarming_remainder * 60 * 1000) / INTERVAL_ALARMCHECK;

  return ApiMethodResponse();
}
ApiMethodResponse api_testAlarmOff(String &requestBody)
{
  alarming_remainder = 1;
  return ApiMethodResponse();
}
ApiMethodResponse api_format(String &requestBody)
{
  ApiMethodResponse response;

  SPIFFS.end();
  response.Error = SPIFFS.format() ? ErrorState::None : ErrorState::InternalServerError;
  SPIFFS.begin();

  return response;
}
ApiMethodResponse api_getLeds(String &requestBody)
{
  ApiMethodResponse response;

  const size_t capacity = JSON_ARRAY_SIZE(3) + JSON_OBJECT_SIZE(1) + 3 * JSON_OBJECT_SIZE(4);
  DynamicJsonDocument doc(capacity);

  JsonArray leds = doc.createNestedArray("leds");
  JsonObject leds_0 = leds.createNestedObject();
  JsonObject leds_1 = leds.createNestedObject();
  JsonObject leds_2 = leds.createNestedObject();

  leds_0["g"] = led_colours[0];
  leds_0["r"] = led_colours[1];
  leds_0["b"] = led_colours[2];
  leds_0["w"] = led_colours[3];

  leds_1["g"] = led_colours[4];
  leds_1["r"] = led_colours[5];
  leds_1["b"] = led_colours[6];
  leds_1["w"] = led_colours[7];

  leds_2["g"] = led_colours[8];
  leds_2["r"] = led_colours[9];
  leds_2["b"] = led_colours[10];
  leds_2["w"] = led_colours[11];

  serializeJson(doc, response.Body);
  response.Type = ResponseType::Json;
  return response;
}
ApiMethodResponse api_setLeds(String &requestBody)
{
  ApiMethodResponse response;

  const size_t capacity = JSON_ARRAY_SIZE(3) + JSON_OBJECT_SIZE(1) + 3 * JSON_OBJECT_SIZE(4) + 40;
  DynamicJsonDocument doc(capacity);

  auto err = deserializeJson(doc, requestBody);
  if (err)
  {
    response.Error = ErrorState::BadRequest;
    return response;
  }

  JsonArray leds = doc["leds"];
  for (uint8_t i = 0; i < NUM_LEDS; i++)
  {
    auto led = leds[i % 3];

    int leds_r = led["r"];
    int leds_g = led["g"];
    int leds_b = led["b"];
    int leds_w = led["w"];

    //G, R, B, W
    led_colours[(i * 4) + 0] = leds_g;
    led_colours[(i * 4) + 1] = leds_r;
    led_colours[(i * 4) + 2] = leds_b;
    led_colours[(i * 4) + 3] = leds_w;
    Serial.printf("%d, %d, %d, %d\r\n", leds_r, leds_g, leds_b, leds_w);

    torching = 1;
  }

  return response;
}
ApiMethodResponse api_getAlarms(String &requestBody)
{
  ApiMethodResponse response;
  response.Body = serializeAlarms();
  response.Type = ResponseType::Json;
  return response;
}
ApiMethodResponse api_setAlarms(String &requestBody)
{
  ApiMethodResponse response;

  deserializeAlarms(requestBody);
  saveAlarms();

  return response;
}
ApiMethodResponse api_getState(String &requestBody)
{
  ApiMethodResponse response;
  response.Body = serializeState();
  response.Type = ResponseType::Json;
  return response;
}

void deserializeAlarms(String &json)
{
  const size_t capacity = JSON_ARRAY_SIZE(ALARM_COUNT) + ALARM_COUNT * JSON_OBJECT_SIZE(5) + (ALARM_COUNT + 1) * 40;
  DynamicJsonDocument doc(capacity);

  deserializeJson(doc, json);

  for (uint8_t i = 0; i < ALARM_COUNT; i++)
  {
    JsonObject node = doc[i];

    alarms[i].Enabled = node["Enabled"];
    alarms[i].Hour = node["Hour"];
    alarms[i].Minute = node["Minute"];
    alarms[i].Duration = node["Duration"];
    alarms[i].RepeatDays = node["RepeatDays"];
  }
}
String serializeAlarms()
{
  const size_t capacity = JSON_ARRAY_SIZE(ALARM_COUNT) + ALARM_COUNT * JSON_OBJECT_SIZE(5);
  DynamicJsonDocument doc(capacity);

  for (uint8_t i = 0; i < ALARM_COUNT; i++)
  {
    JsonObject node = doc.createNestedObject();

    node["Enabled"] = alarms[i].Enabled;
    node["Hour"] = alarms[i].Hour;
    node["Minute"] = alarms[i].Minute;
    node["Duration"] = alarms[i].Duration;
    node["RepeatDays"] = alarms[i].RepeatDays;
  }

  String json;
  serializeJson(doc, json);

  return json;
}
void saveAlarms()
{
  if (SPIFFS.exists(ALARM_FILE_NAME))
  {
    SPIFFS.remove(ALARM_FILE_NAME);
  }

  String json = serializeAlarms();

  auto f = SPIFFS.open(ALARM_FILE_NAME, "w");
  f.print(json);
  f.close();
}

void resetLeds()
{
  for (uint16_t i = 0; i < NUM_LED_COLORS; i++)
    led_colours[i] = 0;
}

//LED Pattern
//   1-18 inside
//  19-36 outside
//  37-54 inside
//  55-72 outside

void setTorch()
{
  switch (torching)
  {
  case 1:
    for (int i = 0; i < NUM_LED_COLORS; i++)
    {
      if (
        (i / 72 == 0) // inside a
        && (i % 4 == 3) // white only
        && ((i / 4) % 3 == 0) // every third
      ) 
      {
        led_colours[i] = 1;
      }
      else if (
        (i / 72 == 2) // inside b
        && (i % 4 == 3) // white only
        && ((i / 4) % 3 == 1) // every third shifted by 1
      ) 
      {
        led_colours[i] = 1;
      }
      else
        led_colours[i] = 0;
    }
    break;

  case 2:
    for (int i = 0; i < NUM_LED_COLORS; i++)
    {
      if (
        (i / 72 == 0 || i / 72 == 2) // inside only
        && (i % 4 == 3) // white only
        && ((i / 4) % 2 == 0) // every second
      ) 
      {
        led_colours[i] = 1;
      }
      else
        led_colours[i] = 0;
    }
    break;
  case 3:
    for (int i = 0; i < NUM_LED_COLORS; i++)
    {
      if (
        (i / 72 == 0 || i / 72 == 2) // inside only
        && (i % 4 == 3) // white only
      ) 
      {
        led_colours[i] = 1;
      }
      else
        led_colours[i] = 0;
    }
    break;
  case 4:
    for (int i = 0; i < NUM_LED_COLORS; i++)
    {
      if (
        (
          (i / 72 == 0 || i / 72 == 2) // inside only
          && (i % 4 == 3) // white only
        ) 
        ||
        (
          (i % 4 == 3) // white only
          && ((i / 4) % 2 == 0) // every second
        )  
      ) 
      {
        led_colours[i] = 1;
      }
      else
        led_colours[i] = 0;
    }
    break;
  case 5:
    for (int i = 0; i < NUM_LED_COLORS; i++)
    {
      if (i % 4 == 3) // white only
        led_colours[i] = 1;
      else
        led_colours[i] = 0;
    }
    break;
  case 6:
    for (int i = 0; i < NUM_LED_COLORS; i++)
    {
      if (
        (i / 72 == 0 || i / 72 == 2) // inside only
        && (i % 4 == 3) // white only
      ) 
      {
        led_colours[i] = 2;
      }
      else if (i % 4 == 3) // white only)
      {
        led_colours[i] = 1;
      }
      else
        led_colours[i] = 0;
    }
    break;
  case 7:
    for (int i = 0; i < NUM_LED_COLORS; i++)
    {
      if (i % 4 == 3) // white only
        led_colours[i] = 3;
      else
        led_colours[i] = 0;
    }
    break;
  case 8:
    for (int i = 0; i < NUM_LED_COLORS; i++)
    {
      if (i % 4 == 3) // white only
        led_colours[i] = 5;
      else
        led_colours[i] = 0;
    }
    break;
  case 9:
    for (int i = 0; i < NUM_LED_COLORS; i++)
    {
      if (i % 4 == 3) // white only
        led_colours[i] = 10;
      else
        led_colours[i] = 0;
    }
    break;
  case 10:
    for (int i = 0; i < NUM_LED_COLORS; i++)
    {
      if (i % 4 == 3) // white only
        led_colours[i] = 20;
      else
        led_colours[i] = 0;
    }
    break;
  case 11:
    for (int i = 0; i < NUM_LED_COLORS; i++)
    {
      if (i % 4 == 3) // white only
        led_colours[i] = 64;
      else
        led_colours[i] = 0;
    }
    break;
  case 12:
    for (int i = 0; i < NUM_LED_COLORS; i++)
    {
      if (i % 4 == 3) // white only
        led_colours[i] = 128;
      else
        led_colours[i] = 0;
    }
    break;
  case 13:
    for (int i = 0; i < NUM_LED_COLORS; i++)
    {
      if (i % 4 == 3) // white only
#if defined(CURRENT_LIMIT_500)
        led_colours[i] = 192;
#else
        led_colours[i] = 255;
#endif
      else
        led_colours[i] = 0;
    }
    break;

#if !defined(CURRENT_LIMIT_500)
  case 17:
    for (int i = 0; i < NUM_LED_COLORS; i++)
    {
      if (i % 4 == 3) // white only
        led_colours[i] = 255;
      else
        led_colours[i] = 32;
    }
    break;
  case 19:
    for (int i = 0; i < NUM_LED_COLORS; i++)
    {
      if (i % 4 == 3) // white only
        led_colours[i] = 255;
      else
        led_colours[i] = 64;
    }
    break;
  case 21:
    for (int i = 0; i < NUM_LED_COLORS; i++)
    {
      if (i % 4 == 3) // white only
        led_colours[i] = 255;
      else
        led_colours[i] = 128;
    }
    break;
  case 23:
    for (int i = 0; i < NUM_LED_COLORS; i++)
    {
      if (i % 4 == 0) // g
        led_colours[i] = 255;
      if (i % 4 == 1) // r
        led_colours[i] = 127;
      if (i % 4 == 2) // b
        led_colours[i] = 218;
      if (i % 4 == 3) // w
        led_colours[i] = 255;
    }
    break;
#endif
#if !defined(CURRENT_LIMIT_500) && !defined(CURRENT_LIMIT_2500)
  case 24:
    for (int i = 0; i < NUM_LED_COLORS; i++)
    {
        led_colours[i] = 255;
    }
    break;
#endif

  default:
    break;
  }
}

void alarm_flash()
{
  uint8_t flash = 0;

  for (uint8_t i = 0; i < FLASH_FLASHES; i++)
  {
    uint8_t start = i * (FLASH_ONTICKS + FLASH_OFFTICKS);

    if (flashCounter == start)
      flash = 1;
    if (flashCounter == start + FLASH_ONTICKS)
      flash = 2;
  }

  if (flash)
    for (uint16_t i = 0; i < NUM_LED_COLORS; i++)
    {
      if (i % 4 == 3) // w
        led_colours[i] = flash == 1 ? 
#if defined(CURRENT_LIMIT_500)
        192
#else
        255 
#endif
        : 0;
      else
        led_colours[i] = 0;
    }

  ++flashCounter %= FLASH_PERIODTICKS;
}

void alarm_sunrise()
{
  if (!sunriseRemaining && !sunriseComplete)
  {
    sunriseRemaining = SUNRISE_DURATION;
    resetLeds();
    //Serial.printf("Sunrse triggered, duration %d\r\n", sunriseRemaining);
  }

  if (!sunriseComplete)
  {
    uint32_t currentTime = SUNRISE_DURATION - sunriseRemaining;
    uint8_t currentColourIndex = currentTime / SUNRISE_TICKSPERCOLOUR;

    uint32_t currentColourStartTime = currentColourIndex * SUNRISE_TICKSPERCOLOUR;
    uint32_t nextColourStartTime = currentColourStartTime + SUNRISE_TICKSPERCOLOUR;

    //Serial.printf("sr: %d, ct: %d, cci:  %d, ccst: %d, ncst: %d\r\n", sunriseRemaining, currentTime, currentColourIndex, currentColourStartTime, nextColourStartTime);

    for (uint8_t i = 0; i < 4; i++)
    {
      uint32_t startColour = sunrise_colours[currentColourIndex][i];
      startColour <<= 2;
      uint32_t endColour = sunrise_colours[currentColourIndex < 30 ? currentColourIndex + 1 : 30][i];
      endColour = (endColour << 2) + 3;

      uint32_t targetColour = 0;
      targetColour += startColour * (nextColourStartTime - currentTime);
      targetColour += endColour * (currentTime - currentColourStartTime);
      targetColour /= nextColourStartTime - currentColourStartTime;

      uint8_t targetLeds = targetColour & 3;
      targetColour >>= 2;
      //Serial.printf("i: %d, sc: %d, ec:  %d, tc: %d, tl: %d\r\n", i, startColour, endColour, targetColour, targetLeds);

      for (uint16_t ci = i; ci < NUM_LED_COLORS; ci += 4)
      {
        if ((ci / 4) % 4 <= targetLeds)
          led_colours[ci] = targetColour;
      }
    }

    if (!--sunriseRemaining)
    {
      sunriseRemaining = 1;
      sunriseComplete = true;
    }
  }

  if (alarming_remainder * INTERVAL_ALARMCHECK < (15 * 60 * 1000))
  {
    alarm_flash();
  }
}

String serializeState()
{
  const size_t capacity = JSON_OBJECT_SIZE(31);
  DynamicJsonDocument doc(capacity);

  doc["millis"] = millis();
  doc["last_OTA"] = last_OTA;
  doc["last_webServerUpdate"] = last_webServerUpdate;
  doc["last_ledUpdate"] = last_ledUpdate;
  doc["last_ConnCheck"] = last_ConnCheck;
  doc["connectionRetryCount"] = connectionRetryCount;
  doc["last_displayRefresh"] = last_displayRefresh;
  doc["displayRefreshNeeded"] = displayRefreshNeeded;
  doc["displayAutoOff"] = displayAutoOff;
  doc["displayLastActivity"] = displayLastActivity;
  doc["displayOn"] = displayOn;
  doc["last_pixelBlink"] = last_pixelBlink;
  doc["activityPixelState"] = activityPixelState;
  doc["last_timeUpdate"] = last_timeUpdate;
  doc["timeUpdateSuccess"] = timeUpdateSuccess;
  doc["last_timeDraw"] = last_timeDraw;
  doc["last_alarmCheck"] = last_alarmCheck;
  doc["alarming"] = alarming;
  doc["alarming_remainder"] = alarming_remainder;
  doc["alarming_alarm"] = alarming_alarm;
  doc["last_alarmVisuals"] = last_alarmVisuals;
  doc["sunriseRemaining"] = sunriseRemaining;
  doc["sunriseComplete"] = sunriseComplete;
  doc["flashCounter"] = flashCounter;
  doc["last_colorCycle"] = last_colorCycle;
  doc["colorCycleEnabled"] = colorCycleEnabled;
  doc["colourCycle_currentIndex"] = colourCycle_currentIndex;
  doc["last_buttonCheck"] = last_buttonCheck;
  doc["buttonPressed"] = buttonPressed;
  doc["buttonPressedCount"] = buttonPressedCount;
  doc["torching"] = torching;

  String json;
  serializeJson(doc, json);

  return json;
}
