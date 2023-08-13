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

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.println("Message arrived");
  DynamicJsonDocument doc(1024);
  auto err = deserializeJson(doc, payload);

  if (err)
  {
    lcd.clear();
    lcd.print("Error parsing JSON");
    return;
  }

  unsigned int game_version = doc["game_version"];
  const char *game_id = doc["game_id"];

  lcd.clear();
  lcd.print(topic);
  lcd.setCursor(0, 1);

  if (game_id != nullptr)
    lcd.printf("Game: %s", game_id);
  else
    lcd.print("Game: unknown");
  lcd.setCursor(0, 2);
  lcd.printf("Version: %d", game_version);
}

void mqtt_reconnect()
{
  while (!mqtt_client.connected())
  {
    if (mqtt_client.connect("ESP32Client"))
    {
      mqtt_client.subscribe("trucksim/gameinfo");
    }
    else
    {
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);

  Wire.begin(esp32pins::kI2CSdaPin, esp32pins::kI2CSclPin);

  lcd.begin(20, 4);
  lcd.setBacklight(255);

  WiFi.begin(kWifiSSID, kWifiPassword);

  lcd.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }

  lcd.clear();
  lcd.print("WiFi connected");

  mqtt_client.setServer(kMqttServer, 1883);
  mqtt_client.setCallback(callback);

  Serial.println("Setup done");
}

void loop()
{
  if (!mqtt_client.connected())
  {
    mqtt_reconnect();
  }
  mqtt_client.loop();
}