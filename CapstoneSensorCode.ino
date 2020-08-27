#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
const char* host = "www.ecoders.ca";
String url = "/dataProcess";
const int httpsPort = 443;
//Sensor Data Variables
const int AirValue = 856; //Analog Value when not in water
const int WaterValue = 458; //Analog Value when fully submerged
int SoilMoistureValue = 0;
int SoilMoisturePercent = 0;
//JSON document variables
StaticJsonDocument<200> doc;
String requestBody;
StaticJsonDocument<200> userLoginDoc;
String requestBodyLogin;
String serverResponse;
int deviceID = 0;
WiFiManager wifiManager;
WiFiClientSecure client;
const char *username;
const char *password;

// Required for LIGHT_SLEEP_T delay mode
extern "C" {
  #include "user_interface.h"
}

void setup() {
  Serial.begin(115200);

  connectToWifi();
  sendUserCredentialsToBackend();
  getDeviceID();
}
void loop() {  
  //proceed to send sensor data if user credentials match the backend credentials
  if (serverResponse == "true") {
    createTLSConnection();
  }
  else {
    Serial.println("The user credentials did not match the one in the backend");
    //TO DO: figure out how to redirect the user to be able to successfully input their credentials again 
  }
  //Loop every 3 minutes
  delay(60000*3-800); 
}
void createTLSConnection() {
  // Use WiFiClientSecure class to create TLS connection
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
  
  //After sending the post request to the server esp goes into light sleep
  lightSleep();
}
void getDeviceID() {
  deviceID = random(1,999999999);
  Serial.print("Device ID: ");
  Serial.println(deviceID);
}
void lightSleep() {
  Serial.println("ESP8266 going into light sleep for 15 minutes");
  WiFi.mode(WIFI_STA);
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  delay(60000*15);
  //ESP.deepSleep(900e6);
}
void connectToWifi() {
  //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    //reset saved settings
    wifiManager.resetSettings();
    //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration

    //initializing the login credential parameters
    WiFiManagerParameter custom_username("username", "Enter Username: ", NULL, 50);
    WiFiManagerParameter custom_password("password", "Enter Password: ", NULL, 50);
    
    //If it restarts and router is not yet online, it keeps rebooting and retrying to connect
    wifiManager.setTimeout(120);

    //Placing parameters on wifiManager page
    wifiManager.addParameter(&custom_username);
    wifiManager.addParameter(&custom_password);

    //placing user inputed values into variables
    username = custom_username.getValue();
    password = custom_password.getValue();
    
    wifiManager.autoConnect("AutoConnectAP");
    //or use this for auto generated name ESP + ChipID
    //wifiManager.autoConnect();
    //if you get here you have connected to the WiFi
    Serial.println("Connected...yeey :)");
}
void sendUserCredentialsToBackend() {
  //Place credentials in JSON document
  saveCredentialsInJsonDocument();

  //Start a connection with the server
  Serial.println("\nStarting connection to server to send user credentials...");
  client.setInsecure();
  if (client.connect(host, 443)) {
    Serial.println("Connection successful!");

    //Send a post request to the server
    client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n" + 
               "Content-Type: application/json" + "\r\n" +
               "Content-Length: " + requestBodyLogin.length() + "\r\n" +
               "\r\n" + requestBodyLogin + "\r\n");

    //Wait up to 10 seconds for server to respond
    int i = 0;
    while ((!client.available()) && (i < 1000)) {
      delay(10);
      i++;
    }
    //Read ther server's response
    if (client.available())
    {
      serverResponse = client.readStringUntil('\r');
      Serial.println("THIS IS THE START OF SERVER RESPONSE"); //for testing purposes
      Serial.print(serverResponse);
      Serial.println("THIS IS THE END OF SERVER RESPONSE"); //for testing purposes
    }
    client.stop();
  }
  else {
    Serial.println("Connection failed!");
  }
}
//Saving the user login credentials in a JSON document
void saveCredentialsInJsonDocument() {
  JsonObject root = userLoginDoc.to<JsonObject>();
  //sending data to JSON document
  root["Username"] = username;
  root["Password"] = password;
  serializeJsonPretty(userLoginDoc, requestBodyLogin);
  serializeJsonPretty(userLoginDoc, Serial);
}
//Place sensor data and other required parameters in a JSON document
void JSONDocument() {
  JsonObject root = doc.to<JsonObject>();
  //sending data to JSON document
  root["UID"] = "q0SPKvUv4WYTcunf9yzFTFiSqHu1";  //TO DO: retrieve the UID from the server
  root["DeviceId"] = deviceID;
  root["Battery"] = 50;
  root["AirValue"] = AirValue;
  root["WaterValue"] = WaterValue;
  root["SoilMoistureValue"] = SoilMoistureValue;
  root["SoilMoisturePercent"] = SoilMoisturePercent;
  root["Token"]=" ";
  serializeJsonPretty(doc, requestBody);
  serializeJsonPretty(doc, Serial);
}
//Retrieve sensor data
void readSensorData() {
  delay(10000); //wait 10 seconds
  //Determine reading from sensor
  SoilMoistureValue = analogRead(A0);
  SoilMoisturePercent = map(SoilMoistureValue, AirValue, WaterValue, 0, 100); 
  realisticPercentValue();
}
//Make sure SoilMoisturePercent is 0-100 value
void realisticPercentValue() {
  if (SoilMoisturePercent > 100) {
    SoilMoisturePercent = 100;
  } 
  else if (SoilMoisturePercent < 0) {
    SoilMoisturePercent = 0;
  }
}
