#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

const int AirValue = 856; //Analog Value when not in water
const int WaterValue = 458; //Analog Value when fully submerged
int SoilMoistureAmount = 0;
int SoilMoisturePercent = 0;

const char* ssid = "";
const char* password = "";String serverHost = "http://52.152.247.223/dataProcess";

String data;
HTTPClient http;
StaticJsonDocument<200> doc; //creating a json document
int httpCode;
String requestBody;

void setup() {
  Serial.begin(115200);  
  
  connectToWiFi();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    //Determine reading from sensor
    readSensorData();    
    
    JSONDocument();   
    
    sendPOSTRequest();
    }
}

void JSONDocument() {
  JsonObject root = doc.to<JsonObject>();  //sending data to the JSON document
  
  root["AirValue"] = AirValue;
  root["WaterValue"] = WaterValue;
  root["SoilMoistureValue"] = SoilMoistureAmount;
  root["SoilMoisturePercent"] = SoilMoisturePercent;  
  
  serializeJsonPretty(doc, requestBody);
  serializeJsonPretty(doc, Serial);
  
}

void sendPOSTRequest() {
  http.begin(serverHost);
  http.addHeader("Content-Type", "Sensor Data");
  httpCode = http.POST(requestBody);
  if (httpCode > 0) {
    Serial.println("\nSuccessful HTTP Post Request");
  }
  else {
    Serial.println("\nError on HTTP POST Request");
  }
  http.end();
}

void readSensorData() {
  delay(10000); //wait 10 seconds  //Determine reading from sensor
  
  SoilMoistureAmount = analogRead(A0);
  SoilMoisturePercent = map(SoilMoistureAmount, AirValue, WaterValue, 0, 100);   
  
  realisticPercentValue();
}

void realisticPercentValue() {
  //make sure smPercent is 0-100 value
  if (SoilMoisturePercent > 100) {
    SoilMoisturePercent = 100;
  } 
  else if (SoilMoisturePercent < 0) {
    SoilMoisturePercent = 0;
  }
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.printf("Connecting... \n");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.printf("\nConnected \n");
}
