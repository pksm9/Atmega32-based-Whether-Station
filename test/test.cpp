#include <Arduino.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

#define LDR_PIN 22 //PC6
#define DHT_PIN 7 //PB7
#define AT_STATUS_PIN 2 //PB2
#define NETWORK_STATUS_PIN 3 //PB3
#define RX 4 //PB4
#define TX 5 //PB5
#define DHTTYPE DHT11

DHT dht(DHT_PIN, DHTTYPE);

LiquidCrystal_I2C lcd(0x27, 20, 4);
SoftwareSerial espSerial(RX, TX); // RX, TX
String request;
String response;

void atCommand(String command) {
    Serial.print(command);
    espSerial.print(command);
    delay(1000);

    digitalWrite(AT_STATUS_PIN, HIGH);
    while (espSerial.available()) {
        response += espSerial.readString();
    }
    digitalWrite(AT_STATUS_PIN, LOW);
    Serial.println(response);
}

void networkCommand(String fieldKey, String value) {
  atCommand("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",80\r\n");
  
  // Build the request
  request = "GET /update?api_key=R92FOM2FF6I40JL1&"+fieldKey+"="+value+"\r\n";
  atCommand("AT+CIPSEND="+String(request.length() + 4)+"\r\n");
  atCommand(request);
  atCommand("AT+CIPCLOSE\r\n");
}

void setup() {
  Serial.begin(115200);
  espSerial.begin(115200);

  pinMode(LDR_PIN, INPUT);
  pinMode(DHT_PIN, INPUT);
  pinMode(AT_STATUS_PIN, OUTPUT);
  pinMode(NETWORK_STATUS_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();

  atCommand("AT+CWJAP=\"Xperia\",\"12345678\"\r\n");
  dht.begin();

  while (true) {
    digitalWrite(RX, HIGH);
    delay(1000);
    digitalWrite(RX, LOW);
    delay(1000);
  }
}

void loop() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int lux = analogRead(LDR_PIN);

  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temperature);
  lcd.print("  H:");
  lcd.print(humidity);
  lcd.setCursor(0, 1);
  lcd.print("L:");
  lcd.print(lux);

  networkCommand("field1", String(temperature));
  networkCommand("field2", String(humidity));
  networkCommand("field3", String(lux));
  digitalWrite(NETWORK_STATUS_PIN, HIGH);
  delay(500);
  digitalWrite(NETWORK_STATUS_PIN, LOW);
}