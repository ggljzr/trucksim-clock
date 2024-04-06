#include <Arduino.h>

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <Wire.h>

#include "pins.h"
// see config.h.example
#include "config.h"
#include "trucksim_topics.h"

WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);

// Current game time in seconds, updated from trucksim/channel/game/time topic.
uint32_t current_time{0};
// Store current ETA and rest stop time for recalculation
// when current time changes.
// Current ETA
uint32_t current_eta{0};
// Current rest stop time
uint32_t current_rest_stop{0};

const char *weekdays[] = {
    "MON",
    "TUE",
    "WED",
    "THU",
    "FRI",
    "SAT",
    "SUN"};

/**
 * Screen displayed when waiting for WiFi connection.
 */
void waiting_screen()
{
  Serial2.write("page 0");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
}

/**
 * Formats given time (in seconds) as 'Weekday hh:mm' and stores it in out_buffer.
 * Outbuffer has to be at least 4 (weekday and space) + 5 (hh:mm) + 1 (zero term) = 10 characters long.
 */
void seconds_to_hhmmdd(uint32_t seconds, char *out_buffer)
{
  uint8_t weekday_num = (seconds / (60 * 60 * 24)) % 7;
  uint32_t hours = (seconds / 3600) % 24;
  uint32_t minutes = (seconds / 60) % 60;

  snprintf(out_buffer, 10, "%s %02u:%02u", weekdays[weekday_num], hours, minutes);
}

void seconds_to_hours_minutes(uint32_t seconds, uint32_t &hours, uint32_t &minutes)
{
  hours = seconds / 3600;
  minutes = (seconds % 3600) / 60;
}

void display_current_time(uint32_t seconds)
{
  char time_str[16];
  seconds_to_hhmmdd(seconds, time_str);

  Serial2.write("clocl.txt=\"");
  Serial2.write(time_str);
  Serial2.write("\"");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
}

/**
 * Formats given amount of time (in seconds) as a time until some event, e.g. ETA or rest stop.
 *
 * current_time global variable (containing current game time in seconds) is used to calculate the time when the event will happen.
 *
 * Outbuffer has to be at least 20 characters long.
 * E. g.: Tue 13:30 (01h 23m)
 */
void seconds_to_time_until(uint32_t seconds, char *out_buffer)
{
  uint32_t hours_eta = 0, minutes_eta = 0;
  seconds_to_hours_minutes(seconds, hours_eta, minutes_eta);

  // buffer for time string in weekday hh:mm format, filled
  // by seconds_to_hhmmdd function
  uint8_t weekday_num = (seconds / (60 * 60 * 24)) % 7;
  uint32_t hours = (seconds / 3600) % 24;
  uint32_t minutes = (seconds / 60) % 60;

  snprintf(out_buffer, 20, "%s %02u:%02u (%02uh %02um)", weekdays[weekday_num], hours, minutes, hours_eta, minutes_eta);
}

/**
 * Display given ETA (in seconds until the event) on the screen.
 */
void display_eta(uint32_t seconds)
{
  char eta_str[20];
  seconds_to_time_until(seconds, eta_str);

  Serial2.write("eta.txt=\"");
  Serial2.write(eta_str);
  Serial2.write("\"");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
}

/**
 * Display given rest stop (in seconds until the event) on the screen.
 */
void display_rest_stop(uint32_t seconds)
{
  char rest_stop_str[20];
  seconds_to_time_until(seconds, rest_stop_str);

  Serial2.write("rest.txt=\"");
  Serial2.write(rest_stop_str);
  Serial2.write("\"");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
}

/**
 * Display given distance (in km) to target on the screen.
 */
void display_distance(float_t distance)
{
  char distance_str[20];
  snprintf(distance_str, 20, "\x01: %14.0f km", distance);
  Serial2.write("dist.txt=\"");
  Serial2.write(distance_str);
  Serial2.write("\"");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
}

/**
 * Screen used to setup telemetry display with default values.
 */
void default_telemetry_screen()
{
  display_current_time(0);
  display_distance(0);
  display_eta(0);
  display_rest_stop(0);
}

void callback(char *topic, byte *payload, unsigned int length)
{
  StaticJsonDocument<512> doc;
  auto err = deserializeJson(doc, payload);

  if (err)
  {
    return;
  }

  if (strcmp(topic, trucksim_topics::kGameInfo) == 0)
  {
    unsigned int game_version = doc["game_version"];
    const char *game_id = doc["game_id"];

    if (game_id == nullptr)
      waiting_screen();
    else
    {
      default_telemetry_screen();
    }

    return;
  }

  if (strcmp(topic, trucksim_topics::kTime) == 0)
  {
    // current game time form API as minutes from MON 00:00
    // converted to seconds
    uint32_t current_time_minutes = doc["value"];
    current_time = current_time_minutes * 60;
    display_current_time(current_time);

    // recaulculate ETA and rest stop
    // since current time has changed
    display_eta(current_eta);
    display_rest_stop(current_rest_stop);
    return;
  }

  if (strcmp(topic, trucksim_topics::kDistance) == 0)
  {
    // distance from API in meters
    float_t distance = doc["value"];
    display_distance(distance / 1000.0f);
    return;
  }

  if (strcmp(topic, trucksim_topics::kEta) == 0)
  {
    // ETA from API in seconds
    current_eta = doc["value"];
    display_eta(current_eta);
    return;
  }

  if (strcmp(topic, trucksim_topics::kRestStop) == 0)
  {
    // rest stop from API in minutes
    uint32_t current_rest_stop_minutes = doc["value"];
    // convert to seconds
    current_rest_stop = current_rest_stop_minutes * 60;
    display_rest_stop(current_rest_stop);
    return;
  }
}

void mqtt_reconnect()
{
  while (!mqtt_client.connected())
  {
    if (mqtt_client.connect("ESP32Client"))
    {
      mqtt_client.subscribe(trucksim_topics::kGameInfo);
      mqtt_client.subscribe(trucksim_topics::kTime);
      mqtt_client.subscribe(trucksim_topics::kDistance);
      mqtt_client.subscribe(trucksim_topics::kEta);
      mqtt_client.subscribe(trucksim_topics::kRestStop);
    }
    else
    {
      delay(5000);
    }
  }
}

void setup()
{
  // nextion is connected to serial2
  Serial2.begin(115200);

  WiFi.begin(kWifiSSID, kWifiPassword);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }

  mqtt_client.setServer(kMqttServer, 1883);
  mqtt_client.setCallback(callback);

  waiting_screen();
}

void loop()
{
  if (!mqtt_client.connected())
  {
    mqtt_reconnect();
  }

  mqtt_client.loop();
}