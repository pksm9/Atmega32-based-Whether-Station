#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <dht.h>

// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(29, DHT11);

void setup()
{
  // Initialize the LCD
  lcd.init();

  // Turn on the backlight
  lcd.backlight();
}

void loop()
{
  // Clear the display
  lcd.clear();

  // Set the cursor to the first column of the first row
  lcd.setCursor(0, 0);

  // Print a message to the LCD
  lcd.print("Hello, world!");

  // pinMode(31, INPUT);
  // while (true) {
  //   lcd.clear();
  //   lcd.print(analogRead(31));
  //   delay(1000);
  // }
    
  // float h = dht.readHumidity();
  // lcd.print(h);
  // float t = dht.readTemperature();
  // lcd.print(t);

  delay(2000);
}
