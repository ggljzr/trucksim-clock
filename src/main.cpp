#include <Arduino.h>

#include <LiquidCrystal_PCF8574.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <Wire.h>

#include "pins.h"

constexpr auto kWifiSSID = "";
constexpr auto kWifiPassword = "";

constexpr auto kMqttServer = "192.168.22.83";

LiquidCrystal_PCF8574 lcd(0x27);
WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);

void callback(char *topic, byte *payload, unsigned int length)
{
  lcd.clear();
  lcd.print(topic);
  lcd.setCursor(0, 1);
  for (int i = 0; i < length; i++)
  {
    lcd.print((char)payload[i]);
  }
}

void mqtt_reconnect()
{
  while (!mqtt_client.connected())
  {
    if (mqtt_client.connect("ESP32Client"))
    {
      mqtt_client.subscribe("esp32/test");
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
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }

  lcd.clear();
  lcd.print("WiFi connected");

  mqtt_client.setServer(kMqttServer, 1883);
  mqtt_client.setCallback(callback);
}

void loop()
{
  if (!mqtt_client.connected())
  {
    mqtt_reconnect();
  }
  mqtt_client.loop();
}