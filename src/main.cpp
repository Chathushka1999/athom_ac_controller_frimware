#include <cstdlib>
#include <stdbool.h>
#include <string.h>
// #include <vector>
#include <sstream>
#include <cstring>
// #include <iostream>
#include <cctype>
#include <time.h>
#include <ArduinoJson.h>
// #include <RapidJSON.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>

// Libraries for MQTT client, WiFi connection and SAS-token generation.
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <base64.h>
// #include <bearssl/bearssl.h>
// #include <bearssl/bearssl_hmac.h>
#include <libb64/cdecode.h>

// Azure IoT SDK for C includes
#include <az_core.h>
#include <az_iot.h>
#include <azure_ca.h>

// Additional sample headers
#include "iot_configs.h"

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPClient.h>
#include <LittleFS.h>
#include "ir.h"

#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>
#include <assert.h>
#include <IRac.h>
#include <IRtext.h>

const uint16_t ir_led = 4;
IRsend irsend(ir_led);
const int kRecvPin = 5;
const uint16_t kCaptureBufferSize = 1024;
#if DECODE_AC
// Some A/C units have gaps in their protocols of ~40ms. e.g. Kelvinator
// A value this large may swallow repeats of some protocols
const uint8_t kTimeout = 50;
#else  // DECODE_AC
// Suits most messages, while not swallowing many repeats.
const uint8_t kTimeout = 15;
#endif // DECODE_AC
const uint16_t kMinUnknownSize = 12;
const uint8_t kTolerancePercentage = kTolerance; // kTolerance is normally 25%
#define LEGACY_TIMING_INFO false
// const uint16_t led = 13;

// const char* ssid = "chathushka";
// const char* password = "87654321";
// const char *ssid = "AC_Controller";
String hostnameBase = "ATH-IR-CUS";
String hostname = "";
String hipen = "-";
String ssid = "";
String wifiSsid = "";
String wifiPw = "";
String host = "";
String device_id = "";
String device_key = "";
bool apStarted = false;
bool isWifiConnected = false;
String updateStarted = "false";
String firmwareURL = "";
ESP8266WebServer server(80);
ESP8266WebServer serverOTA(80);
unsigned long lastSerialReadTime = 0;
IRrecv irrecv(kRecvPin, kCaptureBufferSize, kTimeout, true);
decode_results results; // Somewhere to store the results
bool is_protocol_set = false;


int prot_address = 0;
int temp_address = 20;
int fanspeed_address = 22;
int PROTOCOL = -1;
int POWER = 2;
int FAN_SPEED = 3;
int MODE = 1;
int TEMPERATURE = 22;

// When developing for your own Arduino-based platform,
// please follow the format '(ard;<platform>)'.
#define AZURE_SDK_CLIENT_USER_AGENT "c%2F" AZ_SDK_VERSION_STRING "(ard;esp8266)"

// Utility macros and defines
#define LED_PIN 13
#define sizeofarray(a) (sizeof(a) / sizeof(a[0]))
#define ONE_HOUR_IN_SECS 3600
#define NTP_SERVERS "pool.ntp.org", "time.nist.gov"
#define MQTT_PACKET_SIZE 1024

#define INCOMING_DATA_BUFFER_SIZE 256
static char incoming_data[INCOMING_DATA_BUFFER_SIZE];

// Translate iot_configs.h defines into variables used by the sample
// static const char* ssid = IOT_CONFIG_WIFI_SSID;
// static const char* password = IOT_CONFIG_WIFI_PASSWORD;
// static const char* host = IOT_CONFIG_IOTHUB_FQDN;
// static const char* device_id = IOT_CONFIG_DEVICE_ID;
// static const char* device_key = IOT_CONFIG_DEVICE_KEY;
static const int port = 8883;

// Memory allocated for the sample's variables and structures.
static WiFiClientSecure wifi_client;
static X509List cert((const char *)ca_pem);
static PubSubClient mqtt_client(wifi_client);
static az_iot_hub_client client;
static char sas_token[200];
static uint8_t signature[512];
static unsigned char encrypted_signature[32];
static char base64_decoded_device_key[32];
// static unsigned long next_telemetry_send_time_ms = 0;
//  static char telemetry_topic[128];
//  static uint8_t telemetry_payload[100];
//  static uint32_t telemetry_send_count = 0;

// function definitions
static void initializeClients();
void readStringFromEEPROM(int startAddress, char *buffer);
void writeStringToEEPROM(int startAddress, const char *string);
void saveDataToEEPROM();
void saveProtocol();
void loadDataFromEEPROM();
void handleRoot();
void handleSubmit();
void handleCm();
void handleStatus();
void handleTestIR();
void handleIP();
void loadCredentials();
void loadMqqtParams();
bool connectToWiFi();
void startAPServer();
void startServerOTA();
void saveCredentials(String ssid, String password);
void saveMqttParams(String MqttHost, String MqttClient, String MqttPassword);
void saveURL(String URL, String updateStarted);
String NetworkUniqueId(void);
uint32_t ESP_getChipId(void);
void handleGetFirmwareURL();
void updateFirmware(String url);
void setOTA();
void recieveProtocol();

// Auxiliary functions
void recieveProtocol()
{
  // variables to create timer for recieving protocol
  const unsigned long interval = 60000; // 1 minute in milliseconds
  unsigned long previousMillis = millis();
  Serial.println("Waiting for IR code...");
  while (millis() - previousMillis <= interval)
  {
    if (irrecv.decode(&results))
    {
      // Serial.println("Received IR Code:");
      // Serial.println(static_cast<unsigned long>(results.value), HEX);
      // // unsigned long decoded_protocol = static_cast<unsigned long>(results.decode_type);
      // PROTOCOL = static_cast<int>(results.decode_type);
      // // String decoded_protocol = String(decoded_protocol);
      // irrecv.disableIRIn();
      // Display the tolerance percentage if it has been change from the default.
      if (kTolerancePercentage != kTolerance)
        Serial.printf(D_STR_TOLERANCE " : %d%%\n", kTolerancePercentage);
      // Display the basic output of what we found.
      Serial.print(resultToHumanReadableBasic(&results));
      Serial.println();
      Serial.print("Decoded PROTOCOL in int: ");
      PROTOCOL = static_cast<int>(results.decode_type);
      Serial.println(PROTOCOL);

      yield(); // Feed the WDT as the text output can take a while to print.
#if LEGACY_TIMING_INFO
      // Output legacy RAW timing info of the result.
      Serial.println(resultToTimingInfo(&results));
      yield(); // Feed the WDT (again)
#endif         // LEGACY_TIMING_INFO
      // Output the results as source code
      yield(); // Feed the WDT (again)
      return;
    }
    yield(); // Feed the WDT (again)
  }
  // irrecv.resume();
  Serial.println("Timeout: No IR command received within 1 minute.");
}

void updateFirmware(String url)
{
  HTTPClient http;

  Serial.print("[HTTP] begin...\n");
  if (http.begin(url))
  {
    Serial.print("[HTTP] GET...\n");
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK)
    {
      Serial.println("[HTTP] Firmware update available.");
      WiFiUDP::stopAll(); // Ensure no other services are using port 8266

      Serial.print("Updating firmware from: ");
      Serial.println(url);

      t_httpUpdate_return ret = ESPhttpUpdate.update(url);
      switch (ret)
      {
      case HTTP_UPDATE_FAILED:
        Serial.printf("[OTA] Update failed (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        break;
      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("[OTA] No updates available.");
        break;
      case HTTP_UPDATE_OK:
        Serial.println("[OTA] Update successful.");
        delay(2000);
        ESP.restart();
        break;
      }
      http.end();
    }
    else
    {
      Serial.printf("[HTTP] Unable to connect\n");
    }
  }
}

void handleGetFirmwareURL()
{
  // String firmwareUrl = "http://your-firmware-server.com/firmware.bin";
  String URL = serverOTA.arg("OtaUrl");
  int semicolonPos = URL.indexOf(";");

  // Extract the URL substring
  String firmwareURL = URL.substring(8, semicolonPos);
  // const char*  = firmwareURL.c_str();
  //   char charMinimalURL[firmwareURL.length() + 1]; // +1 for null terminator
  //   firmwareURL.toCharArray(charMinimalURL, firmwareURL.length() + 1);

  //   Serial.print("Recieved firmware URL: ");
  //   Serial.println(firmwareURL);
  updateStarted = "true";
  //  // char charMinimalURL[] = "http://ota.tasmota.com/tasmota/tasmota.bin.gz";
  //   const char searchString[] = ".bin.gz";
  //   const char replaceString[] = "-minimal.bin.gz";

  //   // Find the position of the substring to replace
  //   char* pos = strstr(charMinimalURL, searchString);
  //   if (pos != NULL) {
  //       // Calculate the length of the replacement string
  //       size_t replaceLen = strlen(replaceString);
  //       size_t searchLen = strlen(searchString);
  //       size_t originalLen = strlen(charMinimalURL);

  //       // Calculate the length difference
  //       int lengthDiff = replaceLen - searchLen;

  //       // Shift the remaining characters to accommodate the new string
  //       memmove(pos + replaceLen, pos + searchLen, originalLen - (pos - charMinimalURL) - searchLen + 1);

  //       // Copy the replacement string into the position
  //       memcpy(pos, replaceString, replaceLen);

  //         // memmove(pos + replaceLen, pos + searchLen, originalLen - (pos - charMinimalURL) - searchLen + 1);

  //         // // Copy the replacement string into the position
  //         // memcpy(pos, replaceString, replaceLen);

  // Your original string
  // String firmwareURL = "example_file.bin.gz";
  // Substring to be replaced
  String searchString = ".bin.gz";
  // Replacement substring
  String replaceString = "-minimal.bin.gz";

  // Find the position of the substring to be replaced
  int pos = firmwareURL.indexOf(searchString);

  // If the substring is found
  if (pos != -1)
  {
    // Maximum length of the modified string
    const int bufferSize = firmwareURL.length() + (replaceString.length() - searchString.length()) + 1;
    // Buffer to hold the modified string
    char charMinimalURL[bufferSize + 10];

    // Copy characters before the substring
    firmwareURL.substring(0, pos).toCharArray(charMinimalURL, bufferSize);
    // Concatenate the replacement substring
    strcat(charMinimalURL, replaceString.c_str());
    // Concatenate characters after the substring
    strcat(charMinimalURL, firmwareURL.substring(pos + searchString.length()).c_str());

    // String minimalURL = String(charMinimalURL);
    Serial.println(charMinimalURL);
    String minimalURL(charMinimalURL);
    Serial.println("Minimal firmware url = " + minimalURL);
    saveURL(firmwareURL, updateStarted);
    serverOTA.send(200, "text/plain", "OTA firmware recieved");
    updateFirmware(minimalURL);
  }
  Serial.println("invalid URL");
}

String NetworkUniqueId(void)
{
  String unique_id = WiFi.macAddress();
  unique_id.replace(":", ""); // Full 12 chars MAC address as ID
  String firstSix = unique_id.substring(0, 6);
  return firstSix;
}

uint32_t ESP_getChipId(void)
{
  return ESP.getChipId();
}

void generateWifiHost()
{
  String networkId = NetworkUniqueId();
  char chipId[30];
  // std::string chipId = std::to_string(ESP_getChipId() & 0x1FFF);
  sprintf(chipId, "%u", ESP_getChipId() & 0x1FFF);
  hostname = hostnameBase + hipen + networkId + hipen + chipId;
  ssid = hostname;
}


String homePage = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP8266 Configuration</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      margin: 0;
      padding: 0;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
      background-color: #f4f4f9;
    }
    .container {
      width: 90%;
      max-width: 400px;
      padding: 20px;
      border-radius: 8px;
      background-color: #ffffff;
      box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
    }
    h2 {
      text-align: center;
      color: #333;
    }
    label {
      font-size: 0.9rem;
      color: #555;
    }
    input[type="text"], input[type="password"], select {
      width: 100%;
      padding: 8px;
      margin: 8px 0;
      border: 1px solid #ddd;
      border-radius: 4px;
    }
    button {
      width: 100%;
      padding: 10px;
      border: none;
      border-radius: 4px;
      color: white;
      background-color: #4CAF50;
      cursor: pointer;
    }
    button:hover {
      background-color: #45a049;
    }
    .section {
      margin-top: 20px;
    }
    .status {
      text-align: center;
      margin-top: 20px;
      font-size: 0.9rem;
      color: #666;
    }
  </style>
</head>
<body>

<div class="container">
  <h2>ESP8266 Configuration</h2>

  <!-- MQTT and WiFi Configuration Form -->
  <form id="config-form" onsubmit="return saveConfig()">
    <label for="mqtt-host">MQTT Host</label>
    <input type="text" id="mqtt-host" name="mqttHost" required>

    <label for="mqtt-user">MQTT User</label>
    <input type="text" id="mqtt-user" name="mqttUser" required>

    <label for="mqtt-password">MQTT Password</label>
    <input type="password" id="mqtt-password" name="mqttPassword" required>

    <label for="mqtt-topic">MQTT Topic</label>
    <input type="text" id="mqtt-topic" name="mqttTopic" required>

    <label for="mqtt-fulltopic">MQTT Full Topic</label>
    <input type="text" id="mqtt-fulltopic" name="mqttFullTopic" required>

    <label for="ssid">SSID</label>
    <input type="text" id="ssid" name="ssid" required>

    <label for="wifi-password">WiFi Password</label>
    <input type="password" id="wifi-password" name="wifiPassword" required>

    <label for="protocol">Protocol</label>
    <select id="protocol" name="protocol" required>
      <option value="-1">UNKNOWN</option>
      <option value="0">UNUSED</option>
      <option value="1">RC5</option>
      <option value="2">RC6</option>
      <option value="3">NEC</option>
      <option value="4">SONY</option>
      <option value="5">PANASONIC</option>
      <option value="6">JVC</option>
      <option value="7">SAMSUNG</option>
      <option value="8">WHYNTER</option>
      <option value="9">AIWA_RC_T501</option>
      <option value="10">LG</option>
      <option value="11">SANYO</option>
      <option value="12">MITSUBISHI</option>
      <option value="13">DISH</option>
      <option value="14">SHARP</option>
      <option value="15">COOLIX</option>
      <option value="16">DAIKIN</option>
      <option value="17">DENON</option>
      <option value="18">KELVINATOR</option>
      <option value="19">SHERWOOD</option>
      <option value="20">MITSUBISHI_AC</option>
      <option value="21">RCMM</option>
      <option value="22">SANYO_LC7461</option>
      <option value="23">RC5X</option>
      <option value="24">GREE</option>
      <option value="25">PRONTO</option>
      <option value="26">NEC_LIKE</option>
      <option value="27">ARGO</option>
      <option value="28">TROTEC</option>
      <option value="29">NIKAI</option>
      <option value="30">RAW</option>
      <option value="31">GLOBALCACHE</option>
      <option value="32">TOSHIBA_AC</option>
      <option value="33">FUJITSU_AC</option>
      <option value="34">MIDEA</option>
      <option value="35">MAGIQUEST</option>
      <option value="36">LASERTAG</option>
      <option value="37">CARRIER_AC</option>
      <option value="38">HAIER_AC</option>
      <option value="39">MITSUBISHI2</option>
      <option value="40">HITACHI_AC</option>
      <option value="41">HITACHI_AC1</option>
      <option value="42">HITACHI_AC2</option>
      <option value="43">GICABLE</option>
      <option value="44">HAIER_AC_YRW02</option>
      <option value="45">WHIRLPOOL_AC</option>
      <option value="46">SAMSUNG_AC</option>
      <option value="47">LUTRON</option>
      <option value="48">ELECTRA_AC</option>
      <option value="49">PANASONIC_AC</option>
      <option value="50">PIONEER</option>
      <option value="51">LG2</option>
      <option value="52">MWM</option>
      <option value="53">DAIKIN2</option>
      <option value="54">VESTEL_AC</option>
      <option value="55">TECO</option>
      <option value="56">SAMSUNG36</option>
      <option value="57">TCL112AC</option>
      <option value="58">LEGOPF</option>
      <option value="59">MITSUBISHI_HEAVY_88</option>
      <option value="60">MITSUBISHI_HEAVY_152</option>
      <option value="61">DAIKIN216</option>
      <option value="62">SHARP_AC</option>
      <option value="63">GOODWEATHER</option>
      <option value="64">INAX</option>
      <option value="65">DAIKIN160</option>
      <option value="66">NEOCLIMA</option>
      <option value="67">DAIKIN176</option>
      <option value="68">DAIKIN128</option>
      <option value="69">AMCOR</option>
      <option value="70">DAIKIN152</option>
      <option value="71">MITSUBISHI136</option>
      <option value="72">MITSUBISHI112</option>
      <option value="73">HITACHI_AC424</option>
      <option value="74">SONY_38K</option>
      <option value="75">EPSON</option>
      <option value="76">SYMPHONY</option>
      <option value="77">HITACHI_AC3</option>
      <option value="78">DAIKIN64</option>
      <option value="79">AIRWELL</option>
      <option value="80">DELONGHI_AC</option>
      <option value="81">DOSHISHA</option>
      <option value="82">MULTIBRACKETS</option>
      <option value="83">CARRIER_AC40</option>
      <option value="84">CARRIER_AC64</option>
      <option value="85">HITACHI_AC344</option>
      <option value="86">CORONA_AC</option>
      <option value="87">MIDEA24</option>
      <option value="88">ZEPEAL</option>
      <option value="89">SANYO_AC</option>
      <option value="90">VOLTAS</option>
      <option value="91">METZ</option>
      <option value="92">TRANSCOLD</option>
      <option value="93">TECHNIBEL_AC</option>
      <option value="94">MIRAGE</option>
      <option value="95">ELITESCREENS</option>
      <option value="96">PANASONIC_AC32</option>
      <option value="97">MILESTAG2</option>
      <option value="98">ECOCLIM</option>
      <option value="99">XMP</option>
      <option value="100">TRUMA</option>
      <option value="101">HAIER_AC176</option>
      <option value="102">TEKNOPOINT</option>
      <option value="103">KELON</option>
      <option value="104">TROTEC_3550</option>
      <option value="105">SANYO_AC88</option>
      <option value="106">BOSE</option>
      <option value="107">ARRIS</option>
      <option value="108">RHOSS</option>
      <option value="109">AIRTON</option>
      <option value="110">COOLIX48</option>
      <option value="111">HITACHI_AC264</option>
      <option value="112">KELON168</option>
      <option value="113">HITACHI_AC296</option>
      <option value="114">DAIKIN200</option>
      <option value="115">HAIER_AC160</option>
      <option value="116">CARRIER_AC128</option>
      <option value="117">TOTO</option>
      <option value="118">CLIMABUTLER</option>
      <option value="119">TCL96AC</option>
      <option value="120">BOSCH144</option>
      <option value="121">SANYO_AC152</option>
      <option value="122">DAIKIN312</option>
      <option value="123">GORENJE</option>
      <option value="124">WOWWEE</option>
      <option value="125">CARRIER_AC84</option>
      <option value="126">YORK</option>
    </select>

    <button type="submit">Save Configuration</button>
  </form>

  <!-- IR Protocol Display and Test Command -->
  <div class="section">
    <h3>IR Control</h3>
    <div class="status" id="ir-detected">Detected Protocol: <span id="detected-protocol">None</span></div>
 <!-- Protocol Selection Dropdown -->
 <label for="test-protocol">Select Protocol:</label>
 <select id="test-protocol" name="protocol" required>
   <option value="-1">UNKNOWN</option>
   <option value="0">UNUSED</option>
   <option value="1">RC5</option>
   <option value="2">RC6</option>
   <option value="3">NEC</option>
   <option value="4">SONY</option>
   <option value="5">PANASONIC</option>
   <option value="6">JVC</option>
   <option value="7">SAMSUNG</option>
   <option value="8">WHYNTER</option>
   <option value="9">AIWA_RC_T501</option>
   <option value="10">LG</option>
   <option value="11">SANYO</option>
   <option value="12">MITSUBISHI</option>
   <option value="13">DISH</option>
   <option value="14">SHARP</option>
   <option value="15">COOLIX</option>
   <option value="16">DAIKIN</option>
   <option value="17">DENON</option>
   <option value="18">KELVINATOR</option>
   <option value="19">SHERWOOD</option>
   <option value="20">MITSUBISHI_AC</option>
   <option value="21">RCMM</option>
   <option value="22">SANYO_LC7461</option>
   <option value="23">RC5X</option>
   <option value="24">GREE</option>
   <option value="25">PRONTO</option>
   <option value="26">NEC_LIKE</option>
   <option value="27">ARGO</option>
   <option value="28">TROTEC</option>
   <option value="29">NIKAI</option>
   <option value="30">RAW</option>
   <option value="31">GLOBALCACHE</option>
   <option value="32">TOSHIBA_AC</option>
   <option value="33">FUJITSU_AC</option>
   <option value="34">MIDEA</option>
   <option value="35">MAGIQUEST</option>
   <option value="36">LASERTAG</option>
   <option value="37">CARRIER_AC</option>
   <option value="38">HAIER_AC</option>
   <option value="39">MITSUBISHI2</option>
   <option value="40">HITACHI_AC</option>
   <option value="41">HITACHI_AC1</option>
   <option value="42">HITACHI_AC2</option>
   <option value="43">GICABLE</option>
   <option value="44">HAIER_AC_YRW02</option>
   <option value="45">WHIRLPOOL_AC</option>
   <option value="46">SAMSUNG_AC</option>
   <option value="47">LUTRON</option>
   <option value="48">ELECTRA_AC</option>
   <option value="49">PANASONIC_AC</option>
   <option value="50">PIONEER</option>
   <option value="51">LG2</option>
   <option value="52">MWM</option>
   <option value="53">DAIKIN2</option>
   <option value="54">VESTEL_AC</option>
   <option value="55">TECO</option>
   <option value="56">SAMSUNG36</option>
   <option value="57">TCL112AC</option>
   <option value="58">LEGOPF</option>
   <option value="59">MITSUBISHI_HEAVY_88</option>
   <option value="60">MITSUBISHI_HEAVY_152</option>
   <option value="61">DAIKIN216</option>
   <option value="62">SHARP_AC</option>
   <option value="63">GOODWEATHER</option>
   <option value="64">INAX</option>
   <option value="65">DAIKIN160</option>
   <option value="66">NEOCLIMA</option>
   <option value="67">DAIKIN176</option>
   <option value="68">DAIKIN128</option>
   <option value="69">AMCOR</option>
   <option value="70">DAIKIN152</option>
   <option value="71">MITSUBISHI136</option>
   <option value="72">MITSUBISHI112</option>
   <option value="73">HITACHI_AC424</option>
   <option value="74">SONY_38K</option>
   <option value="75">EPSON</option>
   <option value="76">SYMPHONY</option>
   <option value="77">HITACHI_AC3</option>
   <option value="78">DAIKIN64</option>
   <option value="79">AIRWELL</option>
   <option value="80">DELONGHI_AC</option>
   <option value="81">DOSHISHA</option>
   <option value="82">MULTIBRACKETS</option>
   <option value="83">CARRIER_AC40</option>
   <option value="84">CARRIER_AC64</option>
   <option value="85">HITACHI_AC344</option>
   <option value="86">CORONA_AC</option>
   <option value="87">MIDEA24</option>
   <option value="88">ZEPEAL</option>
   <option value="89">SANYO_AC</option>
   <option value="90">VOLTAS</option>
   <option value="91">METZ</option>
   <option value="92">TRANSCOLD</option>
   <option value="93">TECHNIBEL_AC</option>
   <option value="94">MIRAGE</option>
   <option value="95">ELITESCREENS</option>
   <option value="96">PANASONIC_AC32</option>
   <option value="97">MILESTAG2</option>
   <option value="98">ECOCLIM</option>
   <option value="99">XMP</option>
   <option value="100">TRUMA</option>
   <option value="101">HAIER_AC176</option>
   <option value="102">TEKNOPOINT</option>
   <option value="103">KELON</option>
   <option value="104">TROTEC_3550</option>
   <option value="105">SANYO_AC88</option>
   <option value="106">BOSE</option>
   <option value="107">ARRIS</option>
   <option value="108">RHOSS</option>
   <option value="109">AIRTON</option>
   <option value="110">COOLIX48</option>
   <option value="111">HITACHI_AC264</option>
   <option value="112">KELON168</option>
   <option value="113">HITACHI_AC296</option>
   <option value="114">DAIKIN200</option>
   <option value="115">HAIER_AC160</option>
   <option value="116">CARRIER_AC128</option>
   <option value="117">TOTO</option>
   <option value="118">CLIMABUTLER</option>
   <option value="119">TCL96AC</option>
   <option value="120">BOSCH144</option>
   <option value="121">SANYO_AC152</option>
   <option value="122">DAIKIN312</option>
   <option value="123">GORENJE</option>
   <option value="124">WOWWEE</option>
   <option value="125">CARRIER_AC84</option>
   <option value="126">YORK</option>
 </select>
    <button onclick="sendTestCommand()">Send Test IR Command</button>
  </div>

  <div class="status" id="status-message"></div>
</div>

<script>
  function saveConfig() {
    const mqttHost = document.getElementById('mqtt-host').value;
    const mqttUser = document.getElementById('mqtt-user').value;
    const mqttPassword = document.getElementById('mqtt-password').value;
    const mqttTopic = document.getElementById('mqtt-topic').value;
    const mqttFullTopic = document.getElementById('mqtt-fulltopic').value;
    const ssid = document.getElementById('ssid').value;
    const wifiPassword = document.getElementById('wifi-password').value;
    const protocol = document.getElementById('protocol').value;
    const url = `http://192.168.4.1/cm?user=admin&cmnd=${encodeURIComponent(`Backlog MqttHost ${mqttHost}; MqttUser ${mqttUser}; MqttClient ${mqttUser}; MqttPassword ${mqttPassword}; Topic ${mqttTopic}; FullTopic ${mqttFullTopic}; SSID1 ${ssid}; Password1 ${wifiPassword}; Protocol ${protocol};`)}`
    fetch(url)
      .then(response => {
        if (response.ok) {
          document.getElementById('status-message').innerText = 'Configuration Saved!';
        } else {
          document.getElementById('status-message').innerText = 'Failed to Save Configuration.';
        }
      })
      .catch(error => {
        document.getElementById('status-message').innerText = 'Error saving configuration.';
        console.error('Error:', error);
      });
    return false;
  }

  function sendTestCommand() {
    const protocol = document.getElementById('test-protocol').value;
    const url = `http://192.168.4.1/testIR?command=protocol%20${protocol}%3B%20power%201%3B%20temp%2021%3B%20fan_speed%202%3B`;
    fetch(url)
      .then(response => {
        if (response.ok) {
          document.getElementById('status-message').innerText = 'Test IR Command Sent!';
        } else {
          document.getElementById('status-message').innerText = 'Failed to Send Test IR Command.';
        }
      })
      .catch(error => {
        document.getElementById('status-message').innerText = 'Error sending test IR command.';
        console.error('Error:', error);
      });
  }

  function fetchStatus() {
    fetch('http://192.168.4.1/status')
      .then(response => response.json())
      .then(data => {
        document.getElementById('detected-protocol').innerText = data.detectedProtocol || 'None';
      })
      .catch(error => {
        console.error('Error fetching status:', error);
      });
  }
  // Periodically call fetchStatus to update detected and selected protocols
  setInterval(fetchStatus, 5000); // Fetch every 5 seconds
</script>
</body>
</html>
)rawliteral";

void handleRoot()
{

  server.send(200, "text/html", homePage);
}
void startAPServer()
{
  apStarted = true;
  // Set up AP (Access Point)
  WiFi.softAP(ssid);
  Serial.println("AP Started. Connect to network: " + String(ssid));

  // Handle root URL ("/")
  server.on("/", HTTP_GET, handleRoot);

  // Handle form submission
  server.on("/submit", HTTP_POST, handleSubmit);
  // Handle configuration data (wifi credentials, mqtt credentials and protocol)
  server.on("/cm", HTTP_GET, handleCm);
  // Endpoint to send device state
  server.on("/status", HTTP_GET, handleStatus);
  // Endpoint to handle testing ir commands
  server.on("/testIR", HTTP_GET, handleTestIR);
  // Endpoit to send ip address to be connected on router endpoint
  server.on("/ip", HTTP_GET, handleIP);
  server.begin();
  // delay(2000);
  // WiFi.softAPdisconnect(true);
  // Serial.println("Access Point ended.");
}

void startServerOTA()
{
  serverOTA.on("/firmware_url", HTTP_GET, handleGetFirmwareURL);
  serverOTA.begin();
}

void handleCm()
{
  String cmString = server.arg("cmnd");
  // Find the position of the first semicolon to ignore the "Backlog " part
  int startPos = cmString.indexOf(" ") + 1; // Skip space

  // Split the substring starting from startPos by commas
  String parts[9]; // Assuming there are 8 parts separated by commas
  int count = 0;
  for (int i = startPos; i < cmString.length(); i++)
  {
    if (cmString.charAt(i) == ';')
    {
      parts[count++] = cmString.substring(startPos, i);
      startPos = i + 2; // Skip the semicolon and space and move to next part
    }
  }
  // Last part (no semicolon at the end)
  parts[count++] = cmString.substring(startPos);

  // Assign each part to separate variables
  String MqttHost = parts[0].substring(parts[0].indexOf(" ") + 1);
  String MqttUser = parts[1].substring(parts[1].indexOf(" ") + 1);
  String MqttClient = parts[2].substring(parts[2].indexOf(" ") + 1);
  String MqttPassword = parts[3].substring(parts[3].indexOf(" ") + 1);
  String Topic = parts[4].substring(parts[4].indexOf(" ") + 1);
  String FullTopic = parts[5].substring(parts[5].indexOf(" ") + 1);
  String SSID1 = parts[6].substring(parts[6].indexOf(" ") + 1);
  String Password1 = parts[7].substring(parts[7].indexOf(" ") + 1);
  String Protocol = parts[8].substring(parts[8].indexOf(" ") + 1);

  // Print the values to Serial
  Serial.println("MqttHost: " + MqttHost);
  Serial.println("MqttUser: " + MqttUser);
  Serial.println("MqttClient: " + MqttClient);
  Serial.println("MqttPassword: " + MqttPassword);
  Serial.println("Topic: " + Topic);
  Serial.println("FullTopic: " + FullTopic);
  Serial.println("SSID1: " + SSID1);
  Serial.println("Password1: " + Password1);
  Serial.println("Protocol: " + Protocol);

  saveCredentials(SSID1, Password1);
  saveMqttParams(MqttHost, MqttClient, MqttPassword);
  PROTOCOL = Protocol.toInt();
  saveProtocol();
  server.send(200, "text/html", "Configuration saved. Restarting...");
  delay(500);
  WiFi.softAPdisconnect(true);
  Serial.println("Access Point ended.");
  delay(100);
  ESP.restart();
}

void handleStatus()
{
  Serial.println("Received request for /status");
  recieveProtocol();
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["protocol"] = PROTOCOL;
  jsonDoc["uptime"] = millis() / 1000;
  jsonDoc["signalStrength"] = WiFi.RSSI();

  String jsonResponse;
  serializeJson(jsonDoc, jsonResponse);

  server.send(200, "application/json", jsonResponse);
  Serial.println("Response sent for /status");
}

void handleTestIR()
{
  // Get the command parameter from the URL
  String command = server.arg("command");

  // Initialize variables to hold the extracted values
  int protocol = -1;
  int power = -1;
  int temp = -1;
  int fan_speed = -1;

  // Split the command by ';' to separate key-value pairs
  int startPos = 0;
  String parts[4]; // Assuming there are 4 parts separated by commas
  int count = 0;
  for (int i = startPos; i < command.length(); i++)
  {
    if (command.charAt(i) == ';')
    {
      parts[count++] = command.substring(startPos, i);
      startPos = i + 2; // Skip the semicolon and space and move to next part
    }
  }
  // Last part (no semicolon at the end)
  parts[count++] = command.substring(startPos);

  // Assign each part to separate variables
  protocol = parts[0].substring(parts[0].indexOf(" ") + 1).toInt();
  power = parts[1].substring(parts[1].indexOf(" ") + 1).toInt();
  temp = parts[2].substring(parts[2].indexOf(" ") + 1).toInt();
  fan_speed = parts[3].substring(parts[3].indexOf(" ") + 1).toInt();
  // Print results to Serial Monitor
  Serial.print("Protocol: ");
  Serial.println(protocol);
  Serial.print("Power: ");
  Serial.println(power);
  Serial.print("Temperature: ");
  Serial.println(temp);
  Serial.print("Fan Speed: ");
  Serial.println(fan_speed);

  ir_msg msg;
  msg.protocol = protocol;
  msg.power = power;
  msg.fan_speed = fan_speed;
  msg.mode = MODE;
  msg.temp = temp;
  // sending ir command
  send_ir(msg, ir_led);
  // Serial.println("ir command sent");

  // Send response back to client
  String response = "IR command sent";
  server.send(200, "text/plain", response);
}

void handleIP()
{
  String ip = WiFi.localIP().toString();
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["IP"] = ip;
  String jsonResponse;
  serializeJson(jsonDoc, jsonResponse);
  server.send(200, "application/json", jsonResponse);
}

void handleSubmit()
{
  String ssidRes = server.arg("ssid");
  String passwordRes = server.arg("password");
  Serial.println("Received SSID: " + ssidRes);
  Serial.println("Received Password: " + passwordRes);
  // You can save the credentials or use them as needed

  // Send a response back to the client
  server.send(200, "text/html", "Configuration saved. Restarting...");
  saveCredentials(ssidRes, passwordRes);
  delay(2000);

  // WiFi.softAPdisconnect(true);
  // Serial.println("Access Point ended.");
  // Restart the ESP8266 to apply the new configuration
  ESP.restart();
}

void saveCredentials(String ssid, String password)
{
  Serial.println("Saving credentials");
  // Open the credentials file for writing
  File file = LittleFS.open("/credentials.txt", "w");
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }

  // Write SSID and password to the file
  file.println(ssid);
  file.println(password);

  // Close the file
  file.close();
}

void saveMqttParams(String MqttHost, String MqttClient, String MqttPassword)
{
  Serial.println("Saving mqtt parameters");
  // Open the mqttParams file for writing
  File file = LittleFS.open("/mqttParams.txt", "w");
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }
  file.println(MqttHost);
  file.println(MqttClient);
  file.println(MqttPassword);
  file.close();
}

void saveURL(String URL, String updateStarted)
{
  Serial.println("Saving URL and update flag");
  File file = LittleFS.open("/URL.txt", "w");
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }
  file.println(URL);
  file.println(updateStarted);
  file.close();
}

void loadCredentials()
{
  // Open the credentials file for reading
  if (!LittleFS.begin())
  {
    Serial.println("Failed to mount file system");
    return;
  }

  File file = LittleFS.open("/credentials.txt", "r");
  if (!file)
  {
    Serial.println("No saved credentials found");
    return;
  }

  // Read SSID and password from the file
  String ssidRes = file.readStringUntil('\n');
  String passwordRes = file.readStringUntil('\n');
  ssidRes.trim();

  passwordRes.trim();
  wifiSsid = ssidRes;
  wifiPw = passwordRes;
  // Close the file
  file.close();

  Serial.println(ssidRes);
  Serial.println(passwordRes);
}

void loadMqqtParams()
{
  if (!LittleFS.begin())
  {
    Serial.println("Failed to mount file system");
    return;
  }

  File file = LittleFS.open("/mqttParams.txt", "r");
  if (!file)
  {
    Serial.println("No saved mqtt parameters found");
    return;
  }
  String MqttHost = file.readStringUntil('\n');
  String MqttClient = file.readStringUntil('\n');
  String MqttPassword = file.readStringUntil('\n');
  MqttHost.trim();
  MqttClient.trim();
  MqttPassword.trim();
  //////////////////////Setting mqtt parameters
  host = MqttHost;
  device_id = MqttClient;
  device_key = MqttPassword;
  Serial.print("loading mqtt host; ");
  Serial.println(host);
  Serial.print("loading device id; ");
  Serial.println(device_id);
  Serial.print("loading device key; ");
  Serial.println(device_key);
}

bool connectToWiFi()
{
  Serial.println("Connecting to WiFi");

  Serial.println(wifiSsid);
  Serial.println(wifiPw);

  // Connect to Wi-Fi network using loaded credentials
  WiFi.begin(wifiSsid.c_str(), wifiPw.c_str());

  // Wait until connected or timeout (10 seconds)
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30)
  {
    delay(1000);
    Serial.print(".");
    attempts++;
  }

  // Check connection result
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nWiFi connected");
    Serial.println("IP address: " + WiFi.localIP().toString());
    return true;
  }
  else
  {
    Serial.println("\nFailed to connect to WiFi. Check your credentials or try again.");
  }
  return false;
}

// Function to read a string from EEPROM
void readStringFromEEPROM(int startAddress, char *buffer)
{
  EEPROM.begin(512);
  int i = 0;

  // Read each character until null-terminator is encountered
  while (true)
  {
    char character = EEPROM.read(startAddress + i);
    buffer[i] = character;

    if (character == '\0')
    {
      break;
    }

    i++;
  }
  EEPROM.end();
}

void writeStringToEEPROM(int startAddress, char *string)
{
  EEPROM.begin(512);
  int length = strlen(string);

  for (int i = 0; i < length; i++)
  {
    EEPROM.write(startAddress + i, string[i]);
  }

  // Write the null-terminator at the end of the string
  EEPROM.write(startAddress + length, '\0');

  // Commit the changes to EEPROM
  EEPROM.commit();
  EEPROM.end();
}

void saveDataToEEPROM()
{
  EEPROM.begin(512);
  EEPROM.write(temp_address, TEMPERATURE);
  EEPROM.write(fanspeed_address, FAN_SPEED);
  EEPROM.end();
}

void saveProtocol()
{
  EEPROM.begin(512);
  EEPROM.write(prot_address, PROTOCOL);
  EEPROM.end();
  Serial.print("Saving protocol: ");
  Serial.println(PROTOCOL);
}

void loadDataFromEEPROM()
{
  EEPROM.begin(512);
  if (EEPROM.read(temp_address) != 255)
  {
    TEMPERATURE = EEPROM.read(temp_address);
  }
  if (EEPROM.read(fanspeed_address) != 255)
  {
    FAN_SPEED = EEPROM.read(fanspeed_address);
  }
  if (EEPROM.read(prot_address) != 255)
  {
    PROTOCOL = EEPROM.read(prot_address);
  }
  EEPROM.end();
}

// static void connectToWiFi()
// {
//   Serial.begin(115200);
//   Serial.println();
//   Serial.print("Connecting to WIFI SSID ");
//   Serial.println(ssid);

//   WiFi.mode(WIFI_STA);
//   WiFi.disconnect();
//   delay(100);
//   WiFi.begin(ssid, password);
//   while (WiFi.status() != WL_CONNECTED)
//   {
//     delay(500);
//     Serial.print(".");
//   }

//   Serial.print("WiFi connected, IP address: ");
//   Serial.println(WiFi.localIP());
// }

void setOTA()
{
  // Hostname for OTA identification
  ArduinoOTA.setHostname("myESP8266");

  // Password for OTA authentication (optional)
  // ArduinoOTA.setPassword("admin");

  ArduinoOTA.begin();
}

static void initializeTime()
{
  Serial.print("Setting time using SNTP");

  configTime(-5 * 3600, 0, NTP_SERVERS);
  time_t now = time(NULL);
  while (now < 1510592825)
  {
    delay(500);
    Serial.print(".");
    now = time(NULL);
  }
  Serial.println("done!");
}

static char *getCurrentLocalTimeString()
{
  time_t now = time(NULL);
  return ctime(&now);
}

static void printCurrentTime()
{
  Serial.print("Current time: ");
  Serial.print(getCurrentLocalTimeString());
}

void publishToMqtt(const char *topic, const char *payload, int len)
{
  int result = mqtt_client.publish(
      topic,
      payload);
  if (result == 0)
  {
    Serial.println("Publish failure with 0");
  }
  else if (result == -1)
  {
    Serial.println("Publish failure with -1");
  }
  else
  {
    Serial.println("Message published successfully");
  }
}

void onDeviceTwinGet(const char *payload, size_t length)
{
  Serial.print("twin payload recieved : ");
  Serial.println(payload);
}

void receivedCallback(char *topic, byte *payload, unsigned int length)
{
  ////////////////////////////////////////
  String topic_data;
  int topic_index;
  int req_id_index;
  int twin_index;
  String method;
  String req_id;
  // std::string data;
  String data;
  /////////////////////////////////////////

  Serial.print("Received [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
    data += (char)payload[i];
  }
  Serial.println("");

  StaticJsonDocument<256> doc; // Adjust the size based on your JSON
  DeserializationError error = deserializeJson(doc, data);

  if (error)
  {
    Serial.print(F("Failed to deserialize JSON: "));
    Serial.println(error.c_str());
  }

  topic_data = topic;

  topic_index = topic_data.indexOf("POST/");
  req_id_index = topic_data.indexOf("?$rid=");
  method = topic_data.substring(topic_index + 5, req_id_index - 1);
  Serial.print(method);
  Serial.println(" method invoked");
  req_id = topic_data.substring(req_id_index + 6);
  if (method == "OtaUrl")
  {
    const char *url = doc["state"];
    firmwareURL = String(url);
  }
  else if (method == "Upgrade")
  {
    const char *state = doc["state"];
    if (strcmp(state, "1") == 0)
    {
      if (firmwareURL)
      {
        updateStarted = "true";
        // Substring to be replaced
        String searchString = ".bin.gz";
        // Replacement substring
        String replaceString = "-minimal.bin.gz";

        // Find the position of the substring to be replaced
        int pos = firmwareURL.indexOf(searchString);

        // If the substring is found
        if (pos != -1)
        {
          // Maximum length of the modified string
          const int bufferSize = firmwareURL.length() + (replaceString.length() - searchString.length()) + 1;
          // Buffer to hold the modified string
          char charMinimalURL[bufferSize + 10];

          // Copy characters before the substring
          firmwareURL.substring(0, pos).toCharArray(charMinimalURL, bufferSize);
          // Concatenate the replacement substring
          strcat(charMinimalURL, replaceString.c_str());
          // Concatenate characters after the substring
          strcat(charMinimalURL, firmwareURL.substring(pos + searchString.length()).c_str());
          Serial.println(charMinimalURL);
          String minimalURL(charMinimalURL);
          Serial.println("Minimal firmware url = " + minimalURL);
          updateStarted = "true";
          saveURL(firmwareURL, updateStarted);
          updateFirmware(minimalURL);
        }
      }
    }
  }
  else
  {
    // Handling kelon protocol for hisene ac; This protocol was not properly implemented in IRremoteESP8266.h//
    if (PROTOCOL == 103 || PROTOCOL == 112)
    {
      decode_results results;
      const uint16_t kKelonHdrMark = 9000;
      const uint16_t kKelonHdrSpace = 4600;
      const uint16_t kKelonBitMark = 560;
      const uint16_t kKelonOneSpace = 1680;
      const uint16_t kKelonZeroSpace = 600;
      const uint32_t kKelonGap = 2 * kDefaultMessageGap;
      const uint16_t kKelonFreq = 38000;
      uint8_t duty = kDutyDefault;
      irsend.begin();

      int nbits = 48;
      uint64_t data;
      uint64_t dataOnOff = 0x02840683; // ON/OFF // 10010010000001000000011010000011
      uint64_t dataFan0 = 0x92000683;
      uint64_t dataFan1 = 0x92010683;
      uint64_t dataFan2 = 0x92010683;
      uint64_t dataFan3 = 0x92010683;
      uint64_t data32 = 0xE2000683;
      uint64_t data31 = 0xD2000683;
      uint64_t data30 = 0xC2000683;
      uint64_t data29 = 0xB2000683;
      uint64_t data28 = 0xA2000683;
      uint64_t data27 = 0x92000683; // 25         // 10010010000000000000011010000011
      uint64_t data26 = 0x82000683; // 23
      uint64_t data25 = 0x72000683; // 23
      uint64_t data24 = 0x62000683; // 22
      uint64_t data23 = 0x52000683; // 21
      uint64_t data22 = 0x42000683; // 20
      uint64_t data21 = 0x32000683; // 19
      uint64_t data20 = 0x22000683; // 18
      uint64_t data19 = 0x12000683; // 17
      uint64_t data18 = 0x2000683;  // 18
      uint64_t data1 = 0x10000000;

      if (method == "power")
      {
        const char *power = doc["state"];
        if (strcmp(power, "ON") == 0)
        {
          loadDataFromEEPROM();
          Serial.print("Last saved temperature: ");
          Serial.println(TEMPERATURE);
          for (int i = 0; i < (TEMPERATURE - 18); i++)
          {
            dataOnOff += data1;
          }
          data = dataOnOff;
          irsend.sendGeneric(kKelonHdrMark, kKelonHdrSpace,
                             kKelonBitMark, kKelonOneSpace,
                             kKelonBitMark, kKelonZeroSpace,
                             kKelonBitMark, kKelonGap,
                             data, nbits, kKelonFreq, false, // LSB First.
                             0, kDutyDefault);
        }
        else if (strcmp(power, "OFF") == 0)
        {
          saveDataToEEPROM();
          Serial.print("Saving temperature: ");
          Serial.println(TEMPERATURE);
          Serial.print("Saving fan_speed: ");
          Serial.println(FAN_SPEED);

          data = dataOnOff;
          irsend.sendGeneric(kKelonHdrMark, kKelonHdrSpace,
                             kKelonBitMark, kKelonOneSpace,
                             kKelonBitMark, kKelonZeroSpace,
                             kKelonBitMark, kKelonGap,
                             data, nbits, kKelonFreq, false, // LSB First.
                             0, kDutyDefault);
        }
      }
      else if (method == "temp")
      {
        TEMPERATURE = doc["temp"];
        // if (TEMPERATURE == 16) data = data16;
        // else if (TEMPERATURE == 17) data = data17;
        if (TEMPERATURE == 18)
          data = data18;
        else if (TEMPERATURE == 19)
          data = data19;
        else if (TEMPERATURE == 20)
          data = data20;
        else if (TEMPERATURE == 21)
          data = data21;
        else if (TEMPERATURE == 22)
          data = data22;
        else if (TEMPERATURE == 23)
          data = data23;
        else if (TEMPERATURE == 24)
          data = data24;
        else if (TEMPERATURE == 25)
          data = data25;
        else if (TEMPERATURE == 26)
          data = data26;
        else if (TEMPERATURE == 27)
          data = data27;
        else if (TEMPERATURE == 28)
          data = data28;
        else if (TEMPERATURE == 29)
          data = data29;
        else if (TEMPERATURE == 30)
          data = data30;
        else if (TEMPERATURE == 31)
          data = data31;
        else if (TEMPERATURE == 32)
          data = data32;
        else if (TEMPERATURE > 32)
          data = data32;
        else if (TEMPERATURE < 18)
          data = data18;

        irsend.sendGeneric(kKelonHdrMark, kKelonHdrSpace,
                           kKelonBitMark, kKelonOneSpace,
                           kKelonBitMark, kKelonZeroSpace,
                           kKelonBitMark, kKelonGap,
                           data, nbits, kKelonFreq, false, // LSB First.
                           3, kDutyDefault);
        delay(50);
      }
    }
    else
    {
      if (method == "power")
      {
        ir_msg msg;
        const char *power = doc["state"];
        if (strcmp(power, "ON") == 0)
        {
          POWER = 1;
        }
        else if (strcmp(power, "OFF") == 0)
        {
          POWER = 0;
        }

        if (POWER == 1)
        {
          // char buffer[20];
          // readStringFromEEPROM(prot_address, buffer);
          // PROTOCOL = buffer;
          // Serial.print("Saved protocol: ");
          // Serial.println(buffer);
          loadDataFromEEPROM();
          Serial.print("Saved protocol: ");
          Serial.println(PROTOCOL);
          Serial.print("Last saved temperature: ");
          Serial.println(TEMPERATURE);
          Serial.print("Last saved fan_speed: ");
          Serial.println(FAN_SPEED);
        }
        else if (POWER == 0)
        {
          saveDataToEEPROM();
          Serial.print("Saving temperature: ");
          Serial.println(TEMPERATURE);
          Serial.print("Saving fan_speed: ");
          Serial.println(FAN_SPEED);
        }
        msg.protocol = PROTOCOL;
        msg.power = POWER;
        msg.fan_speed = FAN_SPEED;
        msg.mode = MODE;
        msg.temp = TEMPERATURE;
        // sending ir command
        send_ir(msg, ir_led);
        // Serial.println("ir command sent");
      }

      else if (method == "temp")
      {
        ir_msg msg;
        TEMPERATURE = doc["temp"];
        msg.protocol = PROTOCOL;
        msg.power = 1;
        msg.fan_speed = FAN_SPEED;
        msg.mode = MODE;
        msg.temp = TEMPERATURE;
        // sending ir command
        send_ir(msg, ir_led);
        // Serial.println("ir command sent");
      }

      else if (method == "fan")
      {
        ir_msg msg;
        FAN_SPEED = doc["fanSpeed"];
        msg.protocol = PROTOCOL;

        msg.power = 1;
        msg.fan_speed = FAN_SPEED;
        msg.mode = MODE;
        msg.temp = TEMPERATURE;
        // sending ir command
        send_ir(msg, ir_led);
        // Serial.println("ir command sent");
      }

      else if (method == "mode")
      {
        ir_msg msg;
        MODE = doc["mode"];
        msg.protocol = PROTOCOL;
        msg.power = 1;
        msg.mode = MODE;
        msg.fan_speed = FAN_SPEED;
        msg.temp = TEMPERATURE;
        // sending ir command
        send_ir(msg, ir_led);
        // Serial.println("ir command sent");
      }
    }
  }
  if (method == "protocol")
  {
    PROTOCOL = doc["protocol"];
    // const char *protocol = doc["protocol"];
    //  if (strcmp(protocol, "LG2") == 0)
    //  {
    //    PROTOCOL = 1;
    //  }
    //  else if (strcmp(protocol, "KELON") == 0)
    //  {
    //    PROTOCOL = 2;
    //  }
    //  else if (strcmp(protocol, "COOLIX") == 0)
    //  {
    //    PROTOCOL = 3;
    //  }
    //  else if (strcmp(protocol, "SONY") == 0)
    //  {
    //    PROTOCOL = 4;
    //  }
    //  else if (strcmp(protocol, "DAIKIN") == 0)
    //  {
    //    PROTOCOL = 5;
    //  }
    //  else if (strcmp(protocol, "HAIER_AC") == 0)
    //  {
    //    PROTOCOL = 6;
    //  }
    //  else if (strcmp(protocol, "WHIRLPOOL_AC") == 0)
    //  {
    //    PROTOCOL = 7;
    //  }
    //  else if (strcmp(protocol, "TEKNOPOINT") == 0)
    //  {
    //    PROTOCOL = 8;
    //  }
    //  else if (strcmp(protocol, "GREE") == 0)
    //  {
    //    PROTOCOL = 9;
    //  }
    //  else if (strcmp(protocol, "TCL112AC") == 0)
    //  {
    //    PROTOCOL = 10;
    //  }
    //  else if (strcmp(protocol, "TCL96AC") == 0)
    //  {
    //    PROTOCOL = 11;
    //  }
    //  else if (strcmp(protocol, "SAMSUNG") == 0)
    //  {
    //    PROTOCOL = 12;
    //  }
    //  else if (strcmp(protocol, "PRONTO") == 0)
    //  {
    //    PROTOCOL = 13;
    //  }
    //  else if (strcmp(protocol, "PIONEER") == 0)
    //  {
    //    PROTOCOL = 14;
    //  }

    // writeStringToEEPROM(prot_address, PROTOCOL);
    saveProtocol();
  }

  //   else if(method == "raw"){

  //   ir_msg msg;

  //   const char* PROTOCOL  = doc["protocol"]; // "LG2"
  //  // String protocol = String(PROTOCOL);
  //   POWER = doc["state"]; // 1
  //   TEMPERATURE = doc["temp"]; // 22
  //   FAN_SPEED = doc["fanSpeed"]; // 0
  //   MODE= doc["mode"]; //1

  //  // set msg
  //   msg.protocol    = PROTOCOL;
  //   msg.power       = POWER;
  //   msg.fan_speed   = FAN_SPEED;
  //   msg.mode        = MODE;
  //   msg.temp        = TEMPERATURE;
  //  //sending ir command
  //   send_ir(msg, ir_led);
  //   //Serial.println("ir command sent");

  //   }

  ///////////////////
  String response_topic = "$iothub/methods/res/200/?$rid=" + req_id;
  String response_payload = "success";
  Serial.print("response topic");
  Serial.println(response_topic);

  StaticJsonDocument<200> doc_res; // Choose a capacity that suits your data size
  doc_res["Success"] = true;

  // Optional: Print the JsonDocument to Serial for debugging
  String json;
  ArduinoJson::V6215PB2::serializeJson(doc_res, json);

  ///////////////////////
  publishToMqtt(response_topic.c_str(), json.c_str(), response_payload.length());
}

static void initializeClients()
{
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.user_agent = AZ_SPAN_FROM_STR(AZURE_SDK_CLIENT_USER_AGENT);

  wifi_client.setTrustAnchors(&cert);
  if (az_result_failed(az_iot_hub_client_init(
          &client,
          az_span_create((uint8_t *)(host.c_str()), host.length()),
          az_span_create((uint8_t *)(device_id.c_str()), device_id.length()),
          &options)))
  {
    Serial.println("Failed initializing Azure IoT Hub client");
    return;
  }

  mqtt_client.setServer(host.c_str(), port);
  mqtt_client.setCallback(receivedCallback);
}

/*
 * @brief           Gets the number of seconds since UNIX epoch until now.
 * @return uint32_t Number of seconds.
 */
static uint32_t getSecondsSinceEpoch() { return (uint32_t)time(NULL); }

static int generateSasToken(char *sas_token, size_t size)
{
  az_span signature_span = az_span_create((uint8_t *)signature, sizeofarray(signature));
  az_span out_signature_span;
  az_span encrypted_signature_span = az_span_create((uint8_t *)encrypted_signature, sizeofarray(encrypted_signature));

  uint32_t expiration = getSecondsSinceEpoch() + ONE_HOUR_IN_SECS;

  // Get signature
  if (az_result_failed(az_iot_hub_client_sas_get_signature(
          &client, expiration, signature_span, &out_signature_span)))
  {
    Serial.println("Failed getting SAS signature");
    return 1;
  }

  // Base64-decode device key
  int base64_decoded_device_key_length = base64_decode_chars(device_key.c_str(), device_key.length(), base64_decoded_device_key);

  if (base64_decoded_device_key_length == 0)
  {
    Serial.println("Failed base64 decoding device key");
    return 1;
  }

  // SHA-256 encrypt
  br_hmac_key_context kc;
  br_hmac_key_init(
      &kc, &br_sha256_vtable, base64_decoded_device_key, base64_decoded_device_key_length);

  br_hmac_context hmac_ctx;
  br_hmac_init(&hmac_ctx, &kc, 32);
  br_hmac_update(&hmac_ctx, az_span_ptr(out_signature_span), az_span_size(out_signature_span));
  br_hmac_out(&hmac_ctx, encrypted_signature);

  // Base64 encode encrypted signature
  String b64enc_hmacsha256_signature = base64::encode(encrypted_signature, br_hmac_size(&hmac_ctx));

  az_span b64enc_hmacsha256_signature_span = az_span_create(
      (uint8_t *)b64enc_hmacsha256_signature.c_str(), b64enc_hmacsha256_signature.length());

  // URl-encode base64 encoded encrypted signature
  if (az_result_failed(az_iot_hub_client_sas_get_password(
          &client,
          expiration,
          b64enc_hmacsha256_signature_span,
          AZ_SPAN_EMPTY,
          sas_token,
          size,
          NULL)))
  {
    Serial.println("Failed getting SAS token");
    return 1;
  }

  return 0;
}

void sendDeviceTwin()
{
  String twin_topic = "$iothub/twin/PATCH/properties/reported/?$rid=10";
  StaticJsonDocument<512> twin_doc;
  char twin_payload[300];
  twin_doc["hostname"] = hostname;
  twin_doc["ip"] = WiFi.localIP().toString();
  twin_doc["rssi"] = String(WiFi.RSSI());
  twin_doc["ssid"] = wifiSsid;
  twin_doc["state"] = "ON";
  twin_doc["version"] = "0.0.0.2";
  twin_doc["protocol"] = PROTOCOL;

  serializeJson(twin_doc, twin_payload);
  publishToMqtt(twin_topic.c_str(), String(twin_payload).c_str(), String(twin_payload).length());
  Serial.println("Sent device twin");
}

static int connectToAzureIoTHub()
{
  size_t client_id_length;
  char mqtt_client_id[128];
  if (az_result_failed(az_iot_hub_client_get_client_id(
          &client, mqtt_client_id, sizeof(mqtt_client_id) - 1, &client_id_length)))
  {
    Serial.println("Failed getting client id");
    return 1;
  }

  mqtt_client_id[client_id_length] = '\0';

  char mqtt_username[128];
  // Get the MQTT user name used to connect to IoT Hub
  if (az_result_failed(az_iot_hub_client_get_user_name(
          &client, mqtt_username, sizeofarray(mqtt_username), NULL)))
  {
    Serial.println("Failed to get MQTT username");
    strcpy(mqtt_username, host.c_str());
    strcat(mqtt_username, "/");
    strcat(mqtt_username, device_id.c_str());
    Serial.println("Username generated");
  }

  Serial.print("Client ID: ");
  Serial.println(mqtt_client_id);

  Serial.print("Username: ");
  Serial.println(mqtt_username);

  mqtt_client.setBufferSize(MQTT_PACKET_SIZE);

  while (!mqtt_client.connected())
  {
    time_t now = time(NULL);

    Serial.print("MQTT connecting ... ");

    if (mqtt_client.connect(mqtt_client_id, mqtt_username, sas_token))
    {
      Serial.println("connected.");
      digitalWrite(LED_PIN, HIGH);
    }
    else
    {
      Serial.print("failed, status code =");
      Serial.print(mqtt_client.state());
      Serial.println(". Trying again in 5 seconds.");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }

  int r;
  r = mqtt_client.subscribe(AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC);
  if (r == -1)
  {
    Serial.println("Could not subscribe for cloud-to-device messages.");
  }
  else
  {
    Serial.println("Subscribed for cloud-to-device messages; message id:" + String(r));
  }

  r = mqtt_client.subscribe(AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC);
  if (r == -1)
  {
    Serial.println("Could not subscribe for twin.");
  }
  else
  {
    Serial.println("Subscribed for twin messages; message id:" + String(r));
    sendDeviceTwin();
  }
  return 0;
}

static void establishConnection()
{
  generateWifiHost();
  Serial.println("Launcing ac controller...");
  loadCredentials();
  loadMqqtParams();
  if (wifiSsid == "") /////////////////////////change for testing purposes////////////////// original =  if (wifiSsid == "")//////////////////////////
  {
    startAPServer();
  }
  else
  {
    WiFi.mode(WIFI_STA);
    isWifiConnected = connectToWiFi();
    delay(5000);
    if (!isWifiConnected)
    {
      Serial.print("isWifiConnected = ");
      Serial.println(isWifiConnected);
      delay(30000);
      ESP.restart();
    }
    else
    {
      // connectToWiFi();
      setOTA();
      startServerOTA();
      initializeTime();
      printCurrentTime();
      initializeClients();
      loadDataFromEEPROM();
      Serial.print("Saved protocol: ");
      Serial.println(PROTOCOL);
      // The SAS token is valid for 1 hour by default in this sample.
      // After one hour the sample must be restarted, or the client won't be able
      // to connect/stay connected to the Azure IoT Hub.
      if (generateSasToken(sas_token, sizeofarray(sas_token)) != 0)
      {
        Serial.println("Failed generating MQTT password");
      }
      else
      {
        connectToAzureIoTHub();
      }

      // digitalWrite(LED_PIN, LOW);
    }
  }
}

// static char* getTelemetryPayload()
// {
//   az_span temp_span = az_span_create(telemetry_payload, sizeof(telemetry_payload));
//   temp_span = az_span_copy(temp_span, AZ_SPAN_FROM_STR("{ \"msgCount\": "));
//   (void)az_span_u32toa(temp_span, telemetry_send_count++, &temp_span);
//   temp_span = az_span_copy(temp_span, AZ_SPAN_FROM_STR(" }"));
//   temp_span = az_span_copy_u8(temp_span, '\0');

//   return (char*)telemetry_payload;
// }

void setup()
{
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
#if DECODE_HASH
  // Ignore messages with less than minimum on or off pulses.
  irrecv.setUnknownThreshold(kMinUnknownSize);
#endif                                       // DECODE_HASH
  irrecv.setTolerance(kTolerancePercentage); // Override the default tolerance.
  irrecv.enableIRIn();                       // Start the receiver
  // initializeWifi();
  establishConnection();
}

void loop()
{
  // ir_msg msg;
  // // TEMPERATURE = doc["temp"];
  // msg.protocol = PROTOCOL;
  // msg.power = 1;
  // msg.fan_speed = FAN_SPEED;
  // msg.mode = MODE;
  // msg.temp = TEMPERATURE;
  // // sending ir command
  // send_ir(msg, ir_led);
  // delay(1000);
  // // Serial.println("ir command sent");

  if (apStarted)
  {
    server.handleClient();
  }

  if ((wifiSsid != "") && ((WiFi.status() != WL_CONNECTED) || !mqtt_client.connected()))
  {
    Serial.println("Wifi not connected");
    digitalWrite(LED_PIN, LOW);
    establishConnection();
    delay(10000);
  }
  else
  {

    ArduinoOTA.handle();
    mqtt_client.loop();
    serverOTA.handleClient();
    delay(500);
  }
}
