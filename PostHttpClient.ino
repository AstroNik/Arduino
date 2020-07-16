#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
const char* host = "www.ecoders.ca";
String url = "/dataProcess";
const int httpsPort = 443;
//Sensor Data Variables
const int AirValue = 856; //Analog Value when not in water
const int WaterValue = 458; //Analog Value when fully submerged
int SoilMoistureValue = 0;
int SoilMoisturePercent = 0;
StaticJsonDocument<200> doc;
String requestBody;
String deviceID = "";
void setup() {
  Serial.begin(115200);
  //connect to the wifi
  connectToWifi();
  getDeviceID();
}
void loop() {
  createTLSConnection();
}
void createTLSConnection() {
  // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  client.setInsecure();
  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection failed");
    return;
  }
  readSensorData();
  JSONDocument();
  Serial.println("\nStarting connection to server...");
  if (!client.connect(host, 443))
    Serial.println("Connection failed!");
  else {
    Serial.println("Connection successful!");
  }
  while (client.connected()) {
    client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n" + 
               "Content-Type: application/json" + "\r\n" +
               "Content-Length: " + requestBody.length() + "\r\n" +
               "\r\n" + requestBody + "\r\n");

    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("Headers received");
      break;
    }
  }
  //after sending the post request to the server esp goes into deep sleep
  deepSleep();
}
void getDeviceID() {
  deviceID = WiFi.macAddress();
  Serial.println("Device ID = " + deviceID);
}
void deepSleep() {
  Serial.println("ESP8266 going into deep sleep for 15 minutes");
  ESP.deepSleep(900e6);
}
void connectToWifi() {
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
    Serial.println("Connected...yeey :)");
}
void JSONDocument() {
  JsonObject root = doc.to<JsonObject>();
  //sending data to JSON document
  root["UID"]="";
  root["DeviceId"]=0;
  root["Battery"]=50;
  root["AirValue"]= AirValue;
  root["WaterValue"] = WaterValue;
  root["SoilMoistureValue"] = SoilMoistureValue;
  root["SoilMoisturePercent"] = SoilMoisturePercent;
  serializeJsonPretty(doc, requestBody);
  serializeJsonPretty(doc, Serial);
}
void readSensorData() {
  delay(10000); //wait 10 seconds
  //Determine reading from sensor
  SoilMoistureValue = analogRead(A0);
  SoilMoisturePercent = map(SoilMoistureValue, AirValue, WaterValue, 0, 100); 
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
