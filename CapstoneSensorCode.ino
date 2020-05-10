#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager


//Sensor Data Variables
const int AirValue = 856; //Analog Value when not in water
const int WaterValue = 458; //Analog Value when fully submerged
int SoilMoistureAmount = 0;
int SoilMoisturePercent = 0;

String serverHost = "https://ecoders.ca";
String data;
HTTPClient http;
StaticJsonDocument<200> doc;
int httpCode;
String requestBody;


void setup() {
    Serial.begin(115200);
    //DeviceID();
    
    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    //reset saved settings
    //wifiManager.resetSettings();

    //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    wifiManager.autoConnect("AutoConnectAP");
    //or use this for auto generated name ESP + ChipID
    //wifiManager.autoConnect();

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
    
      readSensorData();
    
      JSONDocument();

      sendPOSTRequest();
  }
}

/*
void DeviceID() {
  deviceID = random(000000001,999999999);
  Serial.println(deviceID);
}*/

void JSONDocument() {
  JsonObject root = doc.to<JsonObject>();

  //sending data to the JSON document
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
  delay(10000); //wait 10 seconds
  
  //Determine reading from sensor
  SoilMoistureAmount = analogRead(A0);
  SoilMoisturePercent = map(SoilMoistureAmount, AirValue, WaterValue, 0, 100); 

  realisticPercentValue();
}

//make sure smPercent is 0-100 value
void realisticPercentValue() {
  if (SoilMoisturePercent > 100) {
    SoilMoisturePercent = 100;
  } 
  else if (SoilMoisturePercent < 0) {
    SoilMoisturePercent = 0;
  }
}
