// #define ENABLE_DEBUG // Uncomment the following line to enable serial debug output

#ifdef ENABLE_DEBUG
  #define DEBUG_ESP_PORT Serial
  #define NODEBUG_WEBSOCKETS
#endif

#include <Arduino.h>

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ESP32) || defined(ARDUINO_ARCH_RP2040)
  #include <WiFi.h>
#endif

#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>

#include "SinricPro.h"
#include "SinricProTV.h"

#define APP_KEY "c7d69b27-e871-406e-90e1-d6dc946e2528"
#define APP_SECRET "49aa3c96-6188-4578-9ef5-078e5854bff7-d0255251-6440-4420-b7ad-f86b61d3788d"
#define TV_ID "6574a75e04cd98f87191aded"

#define WIFI_SSID "CloseToFreshCo"
#define WIFI_PASS "Junction1990"
#define BAUD_RATE 115200

// remote button values
unsigned long btn_power = 0x2FD48B7;
unsigned long long btn_repeat = 0xFFFFFFFFFFFFFFFF;
unsigned long last_code_rec = 0x0;

bool tvPowerState;
SinricProTV &myTV = SinricPro[TV_ID]; // add device to SinricPro

//// IR_setup

// IR reciver
const uint16_t kRecvPin = 34;
IRrecv irrecv(kRecvPin);

//IR Sender LED
const uint16_t kSendIrLed = 14; // 4;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
IRsend irsend(kSendIrLed);      // Set the GPIO to be used to sending the message.

decode_results ir_results;
//// IR_setup end

// TV device callbacks, made empty call backs for unused functions

bool onEmptyFunction_0(const String &deviceId, bool &nName)
{
  Serial.printf("Do nothing Function0");
  return true;
}
bool onEmptyFunction_1(const String &deviceId, String &nName)
{
  Serial.printf("Do nothing Function1");
  return true;
}
bool onEmptyFunction_2(const String &deviceId, const int cCount, String &nName)
{
  Serial.printf("Do nothing Function2");
  return true;
}
bool onEmptyFunction_3(const String &deviceId, bool &nName)
{
  Serial.printf("Do nothing Function3");
  return true;
}
bool onEmptyFunction_4(const String &deviceId, int &volumeDelta, bool volumeDefault)
{
  Serial.printf("Do nothing Function4");
  return true;
}
bool onEmptyFunction_5(const String &deviceId, const int cCount)
{
  Serial.printf("Do nothing Function5");
  return true;
}

bool onPowerState(const String &deviceId, bool &state)
{
  Serial.printf("TV request state: %s, current TV state: %s\r\n", state ? "on" : "off" ,tvPowerState ? "on" : "off");

  if (tvPowerState != state)
  {
    tvPowerState = state; // set powerState
    Serial.printf("TV turned %s\r\n", state ? "on" : "off");

    irrecv.pause(); // this helps feedback from the transmitter

    for (int x = 0; x <= 5; x++)
    {
      irsend.sendNEC(btn_power, 32);
    }

    for (int x = 0; x <= 5; x++)
    {
      irsend.sendNEC(btn_repeat, 32);
    }

    irrecv.resume();
  }else{
    Serial.printf("TV same state %s\r\n", tvPowerState ? "on" : "off");
  }

  return true;
}

// setup function for WiFi connection
void setupWiFi()
{
  Serial.printf("\r\n[Wifi]: Connecting");

#if defined(ESP8266)
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setAutoReconnect(true);
#elif defined(ESP32)
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
#endif

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.printf(".");
    delay(250);
  }
  IPAddress localIP = WiFi.localIP();
  Serial.printf("connected!\r\n[WiFi]: IP-Address is %d.%d.%d.%d\r\n", localIP[0], localIP[1], localIP[2], localIP[3]);
}

// setup function for SinricPro
void setupSinricPro()
{

  // set callback functions to device
  myTV.onPowerState(onPowerState);
  myTV.onAdjustVolume(onEmptyFunction_4);
  myTV.onChangeChannel(onEmptyFunction_1);
  myTV.onChangeChannelNumber(onEmptyFunction_2);
  myTV.onMediaControl(onEmptyFunction_1);
  myTV.onMute(onEmptyFunction_3);
  myTV.onSelectInput(onEmptyFunction_1);
  myTV.onSetVolume(onEmptyFunction_5);
  myTV.onSkipChannels(onEmptyFunction_2);

  // setup SinricPro
  SinricPro.onConnected([]()
                        { Serial.printf("Connected to SinricPro\r\n"); });
  SinricPro.onDisconnected([]()
                           { Serial.printf("Disconnected from SinricPro\r\n"); });
  SinricPro.begin(APP_KEY, APP_SECRET);
}

// main setup function
void setup()
{
  Serial.begin(BAUD_RATE);
  Serial.printf("\r\n\r\n");

  Serial.println("Starting...");

  setupWiFi();
  setupSinricPro();

  irrecv.enableIRIn();
  irsend.begin();

  Serial.println("Ready");
}

void loop()
{

  SinricPro.handle();

  if (irrecv.decode(&ir_results))
  {

    if (ir_results.value == btn_power) // power button
    {
      tvPowerState = !tvPowerState;
      Serial.printf("Got new IR power: %s\r\n", tvPowerState ? "on" : "off");
      myTV.sendPowerStateEvent(tvPowerState);
    }
    else if (ir_results.value == btn_repeat) // repeate last code
    {
      Serial.print("Repeating: ");
      Serial.println(last_code_rec, HEX);
    }
    else
    {
      Serial.print("Unprogrammed code: ");
      Serial.println(ir_results.value, HEX);
    }

    if (ir_results.value != btn_repeat)
    {
      last_code_rec = ir_results.value;
    }

    // dump(&ir_results);
    irrecv.resume();
  }
}