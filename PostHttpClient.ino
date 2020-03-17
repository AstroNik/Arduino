#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#define SERVER_IP "52.152.247.223"

char* ssid = "Oceans11";
char* password = "Why1sy0urname";

const int AirValue = 850; //Analog Value when not in water
const int WaterValue = 458; //Analog Value when fully submerged
int soilMoistureValue = 0;
int soilMoisturePercent = 0;

void setup() {

  Serial.begin(115200);

  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to the WiFi network: ");

}

void loop() {
  // wait for WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) {

    WiFiClient client;
    HTTPClient http;

    Serial.print("[HTTP] begin...\n");
    // configure traged server and url
    http.begin(client, "http://" SERVER_IP "/dataProcess"); //HTTP
    http.addHeader("Content-Type", "application/json");

    soilMoistureValue = analogRead(A0);
    Serial.println(soilMoistureValue);
    soilMoisturePercent = map(soilMoistureValue, AirValue, WaterValue, 0, 100);
  
    StaticJsonDocument<256> jDoc;
    JsonObject root = jDoc.to<JsonObject>();
    root["AirValue"]= AirValue;
    root["WaterValue"] = WaterValue;
    root["SoilMoistureValue"] = soilMoistureValue;
    root["SoilMoisturePercent"] = soilMoisturePercent;

    String requestBody;
    serializeJson(jDoc,requestBody);
    serializeJson(jDoc,Serial);

    int httpCode = http.POST(requestBody);

    // httpCode will be negative on error
    if (httpCode > 0) {

    } else {
      Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  }
  delay(10000);
}
