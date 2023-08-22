#include <Arduino.h>

#include <ArduinoJson.h>
#include <LiquidCrystal_PCF8574.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <Wire.h>

#include "pins.h"
// see config.h.example
#include "config.h"
#include "trucksim_topics.h"

LiquidCrystal_PCF8574 lcd(0x27);
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

// Map pin char used for distance display.
byte char_mappin[] = {
    B00000,
    B01110,
    B11111,
    B11111,
    B11111,
    B01110,
    B00100,
    B00100};

// Stopwatch char used for ETA display.
byte char_stopwatch[] = {
    B00000,
    B01110,
    B10101,
    B10101,
    B10101,
    B10011,
    B10001,
    B01110};

// Mug char used for rest stop display.
byte char_mug[] = {
    B01010,
    B00101,
    B00000,
    B01111,
    B11111,
    B10111,
    B11111,
    B01111};

const char *weekdays[10] = {
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
  lcd.clear();
  // use second row for this message
  // this is a workaround, so when the board restarts and no game info message is recieved
  // it can go directly to the displaying telemetry
  // if the first row is used, the game time message from telemetry will overlap with WiFi connected message
  lcd.setCursor(0, 1);
  lcd.print("WiFi connected...");
  lcd.setCursor(0, 2);
  lcd.print("Waiting for game...");
}

/**
 * Screen displayed when game is started (recieved non-null on trucksim/gameinfo topic).
 */
void welcome_screen(const char *game_id, unsigned int game_version)
{
  lcd.clear();
  lcd.print("TruckSim Clock");
  lcd.setCursor(0, 1);

  if (game_id != nullptr)
    lcd.printf("Game: %s", game_id);
  else
    lcd.print("Game: unknown");

  lcd.setCursor(0, 2);
  lcd.printf("Version: %d", game_version);
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

  lcd.setCursor(0, 0);
  lcd.print(time_str);
}

/**
 * Display given amount of time (in seconds) as a time until some event, e.g. ETA or rest stop.
 * Row and col specify the position on the screen, label is a single character to display before the formated time.
 *
 * current_time global variable (containing current game time in seconds) is used to calculate the time when the event will happen.
 *
 * E. g.: P: 01h 23m MON 12:34
 */
void display_time_until(uint32_t seconds, uint8_t row, uint8_t col, char label)
{
  uint32_t hours = 0, minutes = 0;
  seconds_to_hours_minutes(seconds, hours, minutes);

  // buffer for time string in weekday hh:mm format, filled
  // by seconds_to_hhmmdd function
  char time_str[16];
  seconds_to_hhmmdd(seconds + current_time, time_str);

  lcd.setCursor(col, row);
  lcd.printf("%c: %02uh %02um %s", label, hours, minutes, time_str);
}

/**
 * Display given ETA (in seconds until the event) on the screen.
 */
void display_eta(uint32_t seconds)
{
  display_time_until(seconds, 2, 0, '\x02');
}

/**
 * Display given rest stop (in seconds until the event) on the screen.
 */
void display_rest_stop(uint32_t seconds)
{
  display_time_until(seconds, 3, 0, '\x03');
}

/**
 * Display given distance (in km) to target on the screen.
 */
void display_distance(float_t distance)
{
  lcd.setCursor(0, 1);
  lcd.printf("\x01: %14.1f km", distance);
}

/**
 * Screen used to setup telemetry display with default values.
 */
void default_telemetry_screen()
{
  lcd.clear();
  display_current_time(0);
  display_distance(0);
  display_eta(0);
  display_rest_stop(0);
}

/**
 * Screen displayed when game is exited (null on trucksim/gameinfo topic).
 */
void goodbye_screen()
{
  lcd.clear();
  lcd.print("Goodbye...");
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
      goodbye_screen();
    else
    {
      // Briefly display welcome screen, then switch to the telemetry screen.
      welcome_screen(game_id, game_version);
      // This delay means that processing this topic will block main thread for 3 seconds.
      // If another message is recieved in this time, it will be processed after this delay
      // This should not interfere with program function however.
      delay(3000);
      // Display telemetry screen with default values.
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
  Wire.begin(esp32pins::kI2CSdaPin, esp32pins::kI2CSclPin);

  lcd.begin(20, 4);
  lcd.setBacklight(255);

  WiFi.begin(kWifiSSID, kWifiPassword);

  lcd.print("Connecting to WiFi");

  lcd.createChar(1, char_mappin);    // use \x01 to display this char
  lcd.createChar(2, char_stopwatch); // use \x02 to display this char
  lcd.createChar(3, char_mug);       // use \x03 to display this char

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }

  mqtt_client.setServer(kMqttServer, 1883);
  mqtt_client.setCallback(callback);

  pinMode(esp32pins::kOnOffSwitchPin, INPUT_PULLUP);

  waiting_screen();
}

void loop()
{
  if (!mqtt_client.connected())
  {
    mqtt_reconnect();
  }

  if (digitalRead(esp32pins::kOnOffSwitchPin) == HIGH)
    lcd.setBacklight(0);
  else
    lcd.setBacklight(255);

  mqtt_client.loop();
}