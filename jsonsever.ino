
#include <ESP8266WiFi.h>
#include <DHT.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");


float temp;
float hum;
unsigned long count = 1;

const char* ssid = "IR_Lab";
const char* password = "ccsadmin";

WiFiClient client;
HTTPClient http;
DHT dht11(D4, DHT11); 

void ReadDHT11() {
  temp = dht11.readTemperature();
  hum = dht11.readHumidity();
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password); 
  dht11.begin(); 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  timeClient.begin();
  timeClient.setTimeOffset(25200);
}


void loop() {
  static unsigned long lastTime = 0;
  unsigned long timerDelay = 15000;
  
  if ((millis() - lastTime) > timerDelay) {
    ReadDHT11();
    String runtimeString = "lastTime: " + String(lastTime) + " Count: " + String(count);
    if (isnan(hum) || isnan(temp)) {
      Serial.println("Failed to read from DHT sensor");
    } else {
      Serial.print("Humidity: ");
      Serial.println(hum);
      Serial.print("Temperature: ");
      Serial.println(temp);
      timeClient.update();

      time_t epochTime = timeClient.getEpochTime();
      String formattedTime = timeClient.getFormattedTime();

      struct tm* ptm = gmtime((time_t*)&epochTime);
      int currentDay = ptm->tm_mday;
      int currentMonth = ptm->tm_mon + 1;
      int currentYear = ptm->tm_year + 1900;

      String currentDate = String(currentDay) + "-" + String(currentMonth) + "-" + String(currentYear) + " " + String(formattedTime);
      StaticJsonDocument<200> jsonDocument;
      jsonDocument["humidity"] = hum;
      jsonDocument["temperature"] = temp;
      jsonDocument["runtime"] = lastTime = millis();
      jsonDocument["timestamp"] = currentDate;


      String jsonData;
      serializeJson(jsonDocument, jsonData);

      http.begin(client, "http://192.168.0.20:80/dht11_data");
      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.POST(jsonData);

      if (httpResponseCode > 0) {
        Serial.println("HTTP Response code: " + String(httpResponseCode));
        String payload = http.getString();
        Serial.println("Returned payload:");
        Serial.println(payload);
        count += 1;
      } else {
        Serial.println("Error on sending POST: " + String(httpResponseCode));
      }
      http.end();
    }
    lastTime = millis();
  }
}
