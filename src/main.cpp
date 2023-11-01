#include <Arduino.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
// #include <DHT.h>

#define LDR_PIN 31
#define DHT_PIN 7
#define AT_STATUS_PIN 2
#define NETWORK_STATUS_PIN 3
#define RX 4
#define TX 5
// #define DHTTYPE DHT11

// DHT dht(DHT_PIN, DHTTYPE);

LiquidCrystal_I2C lcd(0x27, 20, 4);
SoftwareSerial espSerial(RX, TX); // RX, TX
String request;
String response;

void atCommand(String command)
{
  Serial.print(command);
  espSerial.print(command);
  delay(1000);

  digitalWrite(AT_STATUS_PIN, HIGH);
  while (espSerial.available())
  {
    response += espSerial.readString();
  }
  digitalWrite(AT_STATUS_PIN, LOW);
  Serial.println(response);
}

void networkCommand(String fieldKey, String value)
{
  atCommand("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",80\r\n");

  // Build the request
  request = "GET /update?api_key=R92FOM2FF6I40JL1&" + fieldKey + "=" + value + "\r\n";
  atCommand("AT+CIPSEND=" + String(request.length() + 4) + "\r\n");
  atCommand(request);
  atCommand("AT+CIPCLOSE\r\n");
}

void setup()
{
  Serial.begin(115200);
  espSerial.begin(115200);

  pinMode(LDR_PIN, INPUT);
  pinMode(AT_STATUS_PIN, OUTPUT);
  pinMode(NETWORK_STATUS_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();

  // dht.begin();

  atCommand("AT+CWJAP=\"Xperia\",\"12345678\"\r\n");
}

void loop()
{
  // float temperature = dht.readTemperature();
  // float humidity = dht.readHumidity();
  float lux = analogRead(LDR_PIN);

  int temp_adc_val;
  float temp_val;
  temp_adc_val = analogRead(DHT_PIN); /* Read Temperature */
  temp_val = (temp_adc_val * 4.88);   /* Convert adc value to equivalent voltage */
  temp_val = (temp_val / 10);         /* LM35 gives output of 10mv/°C */
  // Serial.print(temp_val);
  // Serial.print(" Degree Celsius\n");
  // delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temp_val);
  // lcd.print("  H:");
  // lcd.print(humidity);
  lcd.setCursor(0, 1);
  lcd.print("L:");
  lcd.print(lux);

  networkCommand("field1", String(temp_val));
  // networkCommand("field2", String(humidity));
  networkCommand("field3", String(lux));
  digitalWrite(NETWORK_STATUS_PIN, HIGH);
  delay(500);
  digitalWrite(NETWORK_STATUS_PIN, LOW);
}