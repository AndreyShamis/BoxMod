#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "FS.h"
/**
 ****************************************************************************************************
*/
// WiFi settings
#define   WIFI_SSID                         "RadiationG"
#define   WIFI_PASS                         "polkalol"

#define   MESSAGE_OPT                       1
// Custom settings
#define   CHECK_INTERNET_CONNECT            1                       // For disable internet connectiviy check use 0
#define   RECONNECT_AFTER_FAILS             100                     // 20 = ~1 min -> 100 =~ 4min

// Thermometer and wire settings
#define   LOAD_VCC                          D7                      // D7 13

// NTP settings
#define   NTP_SERVER                        "1.asia.pool.ntp.org"   // Pool of ntp server http://www.pool.ntp.org/zone/asia
#define   NTP_TIME_OFFSET_SEC               10800                   // Time offset
#define   NTP_UPDATE_INTERVAL_MS            60000                   // NTP Update interval - 1 min

#define   UART_BAUD_RATE                    9600 //921600

#define   LOOP_DELAY                        1                       // Wait each loop for LOOP_DELAY
// Unchangeable settings
#define   INCORRECT_EPOCH                   200000                  // Minimum value for time epoch
/**
   Set delay between load disabled to load enabled in seconds
   When the value is 60, load can be automatically enabled after 1 minutes
   in case keeped temperature is higher of current temp
*/
#define   OFF_ON_DELAY_SEC                  30


#define MAX_PWM  1020
/**
   shows counter values identical to one second
   For example loop_delay=10, counter sec will be 100 , when (counter%100 == 0) happens every second
*/
#define COUNTER_IN_LOOP_SECOND              (int)(1000/LOOP_DELAY)
#define NTP_UPDATE_COUNTER                  (COUNTER_IN_LOOP_SECOND*60*3)
#define CHECK_INTERNET_CONNECTIVITY_CTR     (COUNTER_IN_LOOP_SECOND*120)

enum LogType {
  INFO      = 0,
  WARNING   = 1,
  ERROR     = 2,
  PASS      = 3,
  FAIL_t    = 4,
  CRITICAL  = 5,
  DEBUG     = 6,
} ;

/**
 ****************************************************************************************************
*/
const char          *ssid                     = WIFI_SSID;
const char          *password                 = WIFI_PASS;
int                 counter                   = 0;
bool                loadStatus                = 0;
bool                secure_disabled           = false;
int                 last_disable_epoch        = 0;
bool                internet_access           = 0;
unsigned short      internet_access_failures  = 0;
/**
 ****************************************************************************************************
*/
ESP8266WebServer    server(80);
WiFiUDP             ntpUDP;
IPAddress           pingServer (8, 8, 8, 8);    // Ping server
/**
    You can specify the time server pool and the offset (in seconds, can be changed later with setTimeOffset()).
    Additionaly you can specify the update interval (in milliseconds, can be changed using setUpdateInterval()). */
NTPClient           timeClient(ntpUDP, NTP_SERVER, NTP_TIME_OFFSET_SEC, NTP_UPDATE_INTERVAL_MS);


/**
 ****************************************************************************************************
*/
//ADC_MODE(ADC_VCC);

/**
 ****************************************************************************************************
 ****************************************************************************************************
 ****************************************************************************************************
*/
extern "C" {
#include "user_interface.h"

  extern struct rst_info *resetInfo;
}

void setup() {
  //ADC_MODE(ADC_VCC);
  pinMode(LOAD_VCC, OUTPUT);
  resetInfo = ESP.getResetInfoPtr();
  if (CHECK_INTERNET_CONNECT) {
    internet_access = 0;
  }
  else {
    internet_access = 1;
  }
  disableLoad();
  Serial.begin(UART_BAUD_RATE);
  Serial.println("");
  message("Serial communication started.", PASS);
  message("Starting SPIFFS....", INFO);
  SPIFFS.begin();
  message("SPIFFS startted.", PASS);
  //message("Compile SPIFFS", INFO);
  //  SPIFFS.format();

  wifi_connect();
  server_start();
  timeClient.begin();
  timeClient.forceUpdate();
  message(" ----> All started <----", PASS);
  wdt_enable(80000);

}

void loop() {
  if (resetInfo->reason != 6 && resetInfo->reason <= 6) {
    message("getResetReason: " + ESP.getResetReason() + " |getResetInfo: " + ESP.getResetInfo(), INFO);
  }
  server.handleClient();

  message(" ", PASS);

  delay(2000);
  message("  +++++++++++++++++++ Turning on +++++++++++++++++++ ", PASS);
  yield();
  //  for (int i = 1; i <= 1000; i++ ) {
  //    analogWrite(LOAD_VCC, i);
  //    //analogWriteFreq(1);
  //    delay(10);
  //  }
  for (float i = 0; i <= 10; i = i + 0.1 ) {
    enableCoil(i);
    //analogWriteFreq(1);
    delay(100);
  }
  for (int i = 0; i <= 100; i++ ) {
    enableCoil(i);
    //analogWriteFreq(1);
    delay(100);
  }
  message("  +++++++++++++++++++ Turning on +++++++++++++++++++ END ", PASS);

  //  message(" -------------------------------------------------- Turning off", PASS);
  //
  //  //  for (int i = 1000; i >= 0; i--) {
  //  //    analogWrite(LOAD_VCC, i);
  //  //    //analogWriteFreq(1);
  //  //    delay(10);
  //  //  }
  //  for (int i = 100; i >= 0; i--) {
  //    enableCoil(i);
  //    //analogWriteFreq(1);
  //    delay(30);
  //  }
  //
  //  message(" -------------------------------------------------- Turning off END", PASS);



  delay(200);
  message(" analogWrite 0", PASS);
  analogWrite(LOAD_VCC, 0);
  delay(200);
  if (Serial.available() > 0) {
    Serial.println("Received data " + String(read_serial()));
  }



  if (Serial.available() > 0) {
    Serial.println("Received data " + String(read_serial()));
  }

  //}
  /**
     Analog output
     analogWrite(pin, value) enables software PWM on the given pin.
     PWM may be used on pins 0 to 16.
     Call analogWrite(pin, 0) to disable PWM on the pin.
     value may be in range from 0 to PWMRANGE, which is equal to 1023 by default.
     PWM range may be changed by calling analogWriteRange(new_range).

     PWM frequency is 1kHz by default. Call analogWriteFreq(new_frequency) to change the frequency.
  */

  //  if (WiFi.status() != WL_CONNECTED) {
  //    internet_access = 0;
  //    delay(2000);
  //    // Check if not connected , disable all network services, reconnect , enable all network services
  //    if (WiFi.status() != WL_CONNECTED) {
  //      message("WIFI DISCONNECTED", FAIL_t);
  //      reconnect_cnv();
  //    }
  //  }

  if (CHECK_INTERNET_CONNECT) {
    if (counter % CHECK_INTERNET_CONNECTIVITY_CTR == 0 || !internet_access)
    {
      message("Checking Network Connection", INFO);
      internet_access = Ping.ping(pingServer, 2);
      int avg_time_ms = Ping.averageTime();
      message("Ping result is " + String(internet_access) + " avg_time_ms:" + String(avg_time_ms), INFO);
      if (!internet_access) {
        internet_access_failures++;
        delay(500);
      }
      else {
        internet_access_failures = 0;
      }
    }

    if (internet_access_failures >= RECONNECT_AFTER_FAILS) {
      message("No Internet connection. internet_access_failures FLAG, reconnecting all.", CRITICAL);
      internet_access_failures = 0;
      reconnect_cnv();
    }
  }
  if (counter == 0) {
    print_all_info();

  }
  if (counter % NTP_UPDATE_COUNTER == 0) {
    if (internet_access) {
      update_time();
    }
  }
  //ESP.deepSleep(sleepTimeS * 1000000, RF_DEFAULT);
  delay(LOOP_DELAY);
  counter++;
  if (counter >= 16000) {
    counter = 1;
    //printTemperatureToSerial();
  }
}

int getPercentage(int value) {
  if (value == 0) {
    return 0;
  }
  if (value  > 1000) {
    return 100;
  }
}


int percentageToValue(float percent) {
  if (percent <= 0) {
    return 0;
  }
  if (percent  >= 100) {
    return MAX_PWM;
  }

  return (int)((MAX_PWM * percent) / 100);
}

void enableCoil(float percent) {
  if (percent <= 0) {
    digitalWrite(LOAD_VCC, LOW);
  }
  if (percent >= 100) {
    analogWrite(LOAD_VCC, MAX_PWM);
  }
  int value = percentageToValue(percent);
  if (value >= 0 && value <= MAX_PWM) {
    message("Writing "  + String(value), INFO);
    analogWrite(LOAD_VCC, value );
  }
  else {
    message("BAD percent " + String(percent) + " "  + String(value), ERROR);
  }

}
String read_serial() {
  String data = "";
  // send data only when you receive data:
  while (Serial.available() > 0) {
    // get the new byte:
    char inChar = (char)Serial.read();
    data += inChar;

    // say what you got:
    //Serial.print("I received: ");
    //Serial.println(data);
  }
  return data;
}
/**
    Reconnect to wifi - in success enable all services and update time
*/
void reconnect_cnv() {
  close_all_services();
  delay(1000);
  if (wifi_connect()) {
    timeClient.begin();
    server_start();
    delay(200);
    update_time();
  } else {
    message("Cannot reconnect to WIFI... ", FAIL_t);
    delay(1000);
  }
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

/**
   Enable Load
*/
void enableLoad() {

  loadStatus = 1;
  digitalWrite(LOAD_VCC, 1);
}

/**
   Disable Load
*/
void disableLoad() {
  loadStatus = 0;
  last_disable_epoch = timeClient.getEpochTime();
  digitalWrite(LOAD_VCC, 0);
}

/**
   Set WiFi connection and connect
*/
bool wifi_connect() {
  WiFi.mode(WIFI_STA);       //  Disable AP Mode - set mode to WIFI_AP, WIFI_STA, or WIFI_AP_STA.
  WiFi.begin(ssid, password);

  // Wait for connection
  message("Connecting to [" + String(ssid) + "][" + String(password) + "]...", INFO);
  int con_counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
    con_counter++;
    if (con_counter % 20 == 0) {
      message("", INFO);
      message("Still connecting...", WARNING);
    }
    if (con_counter == 150) {
      message("", INFO);
      message("Cannot connect to [" + String(ssid) + "] ", FAIL_t);
      WiFi.disconnect();
      message(" ----> Disabling WiFi...", INFO);
      WiFi.mode(WIFI_OFF);
      return false;
    }
  }
  message("", INFO);
  message("Connected to [" + String(ssid) + "]  IP address: " + WiFi.localIP().toString(), PASS);
  //  Serial.print("PASS: );
  //  Serial.println(WiFi.localIP());
  if (MDNS.begin("esp8266")) {
    message("MDNS responder started", PASS);
  }
  message("-----------------------------------", INFO);
  return true;
}

/**
  Keep type of mesages
*/
//static inline char *stringFromLogType(enum LogType lt)
static const char *stringFromLogType(const enum LogType lt)
{
  static const char *strings[] = {"INFO", "WARN", "ERROR", "PASS", "FAIL", "CRITICAL", "DEBUG"};
  return strings[lt];
}

/**
   Print message to Serial console
*/
void message(const String msg, const enum LogType lt) {
  if (MESSAGE_OPT) {
    if (msg.length() == 0) {
      Serial.println(msg);
    }
    else {
      Serial.println(String(timeClient.getEpochTime()) + " : " + timeClient.getFormattedTime() + " : " + String(stringFromLogType(lt)) + " : " + msg);
    }
  }
}
/**
   Start WEB server
*/
void server_start() {
  server.on("/", handleRoot);
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);
  message("Staring HTTP server...", INFO);
  server.begin();
  message("HTTP server started", PASS);
}


/**
  Close all network services
*/
void close_all_services() {
  message(" ----> Starting close all network services <----", INFO);

  message(" ----> Closing NTP Client...", INFO);
  timeClient.end();

  message(" ----> Closing WEB Server...", INFO);
  server.close();

  message(" ----> Disconnecting WIFI...", INFO);
  WiFi.disconnect();
  message(" ----> Disabling WiFi...", INFO);
  WiFi.mode(WIFI_OFF);
  message(" ----> WiFi disabled...", INFO);

  yield();
  message(" ----> Finished closing all network services <----", INFO);
}

/**
   Update time by NTP client
*/
void update_time() {
  if (timeClient.getEpochTime() < INCORRECT_EPOCH) {
    unsigned short counter_tmp = 0;
    while (timeClient.getEpochTime() < INCORRECT_EPOCH && counter_tmp < 2) {
      message("Incorrect time, trying to update: #:" + String(counter_tmp) , CRITICAL);
      counter_tmp++;
      timeClient.update();
      timeClient.forceUpdate();
      timeClient.update();
      if (timeClient.getEpochTime() < INCORRECT_EPOCH) {
        delay(1000 + 50 * counter_tmp);
        yield();
      }
      else {
        break;
      }
    }
  }
  else {
    timeClient.forceUpdate();
    timeClient.update();
  }
  message("Time updated." , PASS);
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

/***
  WEB Server function
*/
/**
  WEB Server function
*/
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: " + server.uri() + "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args() + "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

/**
  WEB Server function
*/
void handleRoot() {
  String message = build_index();
  server.send(200, "text/html", message);
}

/**

*/
String build_index() {
  //"'current_temperature': '" + String(getTemperature(outsideThermometerIndex)) + "'," +
  //"'inside_temperature': '" + String(getTemperature(getInsideThermometer())) + "'," +
  String ret_js = String("") + "load = \n{" +
                  "'internet_access': '" + String(internet_access) + "'," +
                  "'load_status': '" + String(loadStatus) + "'," +
                  "'flash_chip_id': '" + String(ESP.getFlashChipId()) + "'," +
                  "'flash_chip_size': '" + String(ESP.getFlashChipSize()) + "'," +
                  "'flash_chip_speed': '" + String(ESP.getFlashChipSpeed()) + "'," +
                  "'flash_chip_mode': '" + String(ESP.getFlashChipMode()) + "'," +
                  "'core_version': '" + ESP.getCoreVersion() + "'," +
                  "'sdk_version': '" + String(ESP.getSdkVersion()) + "'," +
                  "'boot_version': '" + ESP.getBootVersion() + "'," +
                  "'boot_mode': '" + String(ESP.getBootMode()) + "'," +
                  "'cpu_freq': '" + String(ESP.getCpuFreqMHz()) + "'," +
                  "'mac_addr': '" + WiFi.macAddress() + "'," +
                  "'wifi_channel': '" + String(WiFi.channel()) + "'," +
                  "'rssi': '" + WiFi.RSSI() + "'," +
                  "'sketch_size': '" + String(ESP.getSketchSize()) + "'," +
                  "'free_sketch_size': '" + String(ESP.getFreeSketchSpace()) + "'," +
                  "'time_str': '" + timeClient.getFormattedTime() + "'," +
                  "'time_epoch': '" + timeClient.getEpochTime() + "'," +
                  "'hostname': '" + WiFi.hostname() + "'" +
                  "};\n";
  String ret = String("") + "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><title>Load Info</title></head>" +
               " <script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.0/jquery.min.js'></script>\n" +
               " <script src='http://tm.anshamis.com/js/boxmod.js'></script>\n" +
               " <link rel='stylesheet' type='text/css' href='http://tm.anshamis.com/css/boxmod.css'>\n" +
               "<body><script>" + ret_js + "</script>\n" +
               "<div id='content'></div>" +
               "<script>\n " +               "$(document).ready(function(){ onLoadPageLoad(); });</script>\n" +
               "</body></html>";
  return ret;
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

/**
  Write to file content on SPIFFS
*/
void save_setting(const char* fname, String value) {
  File f = SPIFFS.open(fname, "w");
  if (!f) {
    Serial.print("Cannot open file:");
    Serial.println(fname);
    return;
  }
  f.println(value);
  Serial.print("Written:");
  Serial.println(value);
  f.close();
}

/**
  Read file content from SPIFFS
*/
String read_setting(const char* fname) {
  String s      = "";
  File f = SPIFFS.open(fname , "r");
  if (!f) {
    Serial.print("file open failed:");
    Serial.println(fname);
  }
  else {
    s = f.readStringUntil('\n');
    f.close();
  }
  return s;
}
/**
 ****************************************************************************************************
*/

/**

*/
void print_all_info() {
  message("", INFO);
  message("Load status: " + String(loadStatus) + " |HostName: " + WiFi.hostname() + " |Ch: " + String(WiFi.channel()) + " |RSSI: " + WiFi.RSSI() + " |MAC: " + WiFi.macAddress(), INFO);
  message("Flash Chip Id/Size/Speed/Mode: " + String(ESP.getFlashChipId()) + "/" + String(ESP.getFlashChipSize()) + "/" + String(ESP.getFlashChipSpeed()) + "/" + String(ESP.getFlashChipMode()), INFO);
  message("SdkVersion: " + String(ESP.getSdkVersion()) + "\tCoreVersion: " + ESP.getCoreVersion() + "\tBootVersion: " + ESP.getBootVersion(), INFO);
  message("CpuFreqMHz: " + String(ESP.getCpuFreqMHz()) + " \tBootMode: " + String(ESP.getBootMode()) + "\tSketchSize: " + String(ESP.getSketchSize()) + "\tFreeSketchSpace: " + String(ESP.getFreeSketchSpace()), INFO);
  resetInfo = ESP.getResetInfoPtr();
  if (resetInfo->reason != 6 && resetInfo->reason <= 6) {
    message("getResetReason: " + ESP.getResetReason() + " |getResetInfo: " + ESP.getResetInfo(), INFO);
  }

}
