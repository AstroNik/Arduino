#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

// WLAN
#define mySSID "Rogers 456"
#define myPASSWORD "A6472140090"

const char*  ssid = mySSID;
const char* password = myPASSWORD;

const char* host = "ecoders.ca";
String url = "/dataProcess";
const char* fingerprint = "33337bd569ceb38f5d79a02919910233f93aea53"; 


//Sensor Data Variables
const int AirValue = 856; //Analog Value when not in water
const int WaterValue = 458; //Analog Value when fully submerged
int SoilMoistureAmount = 0;
int SoilMoisturePercent = 0;
StaticJsonDocument<200> doc;
String data;
int httpCode;
String requestBody;
char jsonChar[100];


const int httpsPort = 443;

void setup() {
  Serial.begin(115200);

  //connect to the wifi
  connectToWifi();

  createTLSConnection();
  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  
}

void createTLSConnection() {
  // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  client.setInsecure();
  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }
  
  if (client.verify(fingerprint, host)) {
    Serial.println("fingerprint matches");
    readSensorData();
    JSONDocument();
  } else {
    Serial.println("fingerprint doesn't match");
  }

  Serial.println("\nStarting connection to server...");
  if (!client.connect(host, 443))
    Serial.println("Connection failed!");
  else {
    Serial.println("Connected to server!");
    //this is where the issue is when i try to send the json document to the server, then it gives me an error 

  }
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    Serial.println(line);
    Serial.println();
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String line = client.readStringUntil('\n');
  Serial.println("reply was:");
  Serial.println("==========");
  Serial.println(line);
  Serial.println("==========");
  Serial.println("closing connection");
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
    Serial.println("connected...yeey :)");
}

void JSONDocument() {
  JsonObject root = doc.to<JsonObject>();

  //sending data to the JSON document
  root["AirValue"] = AirValue;
  root["WaterValue"] = WaterValue;
  root["SoilMoistureValue"] = SoilMoistureAmount;
  root["SoilMoisturePercent"] = SoilMoisturePercent;

  serializeJsonPretty(doc, requestBody);
  serializeJsonPretty(doc, Serial);

  serializeJsonPretty(doc, jsonChar);
  //Serial.println("This is JSON DOcument");
  //Serial.println(jsonChar);
  //root.printTo((char*)jsonChar, root.size() + 1);
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
