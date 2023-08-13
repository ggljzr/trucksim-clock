#include <Arduino.h>

#include <ArduinoJson.h>
#include <LiquidCrystal_PCF8574.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <Wire.h>

#include "pins.h"
// see config.h.example
#include "config.h"

LiquidCrystal_PCF8574 lcd(0x27);
WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);

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

/**
 * Screen displayed when waiting for WiFi connection.
 */
void waiting_screen()
{
  lcd.clear();
  lcd.print("WiFi connected...");
  lcd.setCursor(0, 1);
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
 * Display given distance (in km) to target on the screen.
 */
void display_distance(uint32_t distance)
{
  lcd.setCursor(0, 1);
  lcd.printf("\x01: %14d km", distance);
}

/**
 * Screen used to setup telemetry display with default values.
 */
void default_telemetry_screen()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MON 00:00");
  display_distance(0);
  lcd.setCursor(0, 2);
  lcd.print("\x02: 99h 59m MON 00:00");
  lcd.setCursor(0, 3);
  lcd.print("\x03: 99h 59m MON 00:00");
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
  DynamicJsonDocument doc(1024);
  auto err = deserializeJson(doc, payload);

  if (err)
  {
    return;
  }

  if (strcmp(topic, "trucksim/gameinfo") == 0)
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

  if (strcmp(topic, "trucksim/channel/truck/navigation/distance") == 0)
  {
    uint32_t distance = doc["value"];
    display_distance(distance);
    return;
  }
}

void mqtt_reconnect()
{
  while (!mqtt_client.connected())
  {
    if (mqtt_client.connect("ESP32Client"))
    {
      mqtt_client.subscribe("trucksim/gameinfo");
      mqtt_client.subscribe("trucksim/channel/truck/navigation/distance");
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

  delay(100);

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

  waiting_screen();
}

void loop()
{
  if (!mqtt_client.connected())
  {
    mqtt_reconnect();
  }

  if (digitalRead(esp32pins::kOnOffSwitchPin) == LOW)
    lcd.setBacklight(0);
  else
    lcd.setBacklight(255);

  mqtt_client.loop();
}