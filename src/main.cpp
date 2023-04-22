#include <Arduino.h>

#include <LiquidCrystal_PCF8574.h>
#include <WiFi.h>
#include <Wire.h>

#include "pins.h"

LiquidCrystal_PCF8574 lcd(0x27);

void setup()
{
  Wire.begin(esp32pins::kI2CSdaPin, esp32pins::kI2CSclPin);

  lcd.begin(20, 4);
  lcd.setBacklight(255);

  WiFi.begin("", "");

  lcd.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }

  lcd.clear();
  lcd.print("WiFi connected");
}

void loop()
{
}