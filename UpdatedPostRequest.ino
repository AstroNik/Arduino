// Modified by Nikhil Kapadia
// 991495131
// October 31st 2020

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

//Reads the voltage of battery internally and not externally (since the battery is already connected to the board)
//ADC_MODE(ADC_VCC);

//Server Variables
const char* host = "www.ecoders.ca";
String url = "/dataProcess";
String urlEmail = "/devicelogin";
const int httpsPort = 443;
String serverResponse;

//Sensor Data Variables
const int AirValue = 856; //Analog Value when not in water
const int WaterValue = 458; //Analog Value when fully submerged
int SoilMoistureValue = 0;
int SoilMoisturePercent = 0;

//JSON Document Variables
StaticJsonDocument<200> userLoginDoc;
String requestBodyLogin;

//Wifi Manager Variables
WiFiManager wifiManager;
WiFiClientSecure client;
char username[50];
char deviceName[80];

//Device ID and Memory Variables
String deviceID = "123456789";

//Battery Amount Variable
int batteryAmount;
int voltage;


//needed to use C SDK based fuctions - needed for light sleep
extern "C" {
#include "user_interface.h"
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);

  connectToWifi();

  //Send the user email, plant name and device id to server
  sendUserCredentialsToBackend();
}

void loop() {
  //determine what the server response is after retrieving the user email, plant name and device id


  //check the server response to determine if the user email exists in the database
  if (serverResponse == "HTTP/1.1 520 Origin Error") {
    Serial.println("The user email does not exist in the database");
  }
  else {
    //send the sensor data to the server if user email is valid
    createTLSConnection();
  }

  //Loop every 3 minutes
  delay(60000 * 15 - 800);

}

//Create a secure connection and send sensor data to the server
void createTLSConnection() {
  StaticJsonDocument<200> doc;
  String requestBody;

  // Use WiFiClientSecure class to create TLS connection
  client.setInsecure();
  batteryLevel();
  SoilMoistureValue = analogRead(A0);
  SoilMoisturePercent = map(SoilMoistureValue, AirValue, WaterValue, 0, 100);

  if (SoilMoisturePercent > 100) {
    SoilMoisturePercent = 100;
  }
  else if (SoilMoisturePercent < 0) {
    SoilMoisturePercent = 0;
  }
  else {
    SoilMoisturePercent = SoilMoisturePercent;
  }

  JsonObject root = doc.to<JsonObject>();
  //sending data to JSON document
  root["UID"] = serverResponse;
  root["DeviceId"] = deviceID;
  root["Battery"] = 50;
  root["AirValue"] = AirValue;
  root["WaterValue"] = WaterValue;
  root["SoilMoistureValue"] = SoilMoistureValue;
  root["SoilMoisturePercent"] = SoilMoisturePercent;
  serializeJsonPretty(doc, requestBody);
  serializeJsonPretty(doc, Serial);

  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection failed");
    return;
  }
  else {
    Serial.println("Connection successful!");
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
  }

  //After sending the post request to the server esp goes into light sleep
  //lightSleep();
}

//
void lightSleep() {
  Serial.println("ESP8266 going into light sleep for 15 minutes");
  WiFi.mode(WIFI_STA);
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  delay(60000 * 15);
  //ESP.deepSleep(900e6);
}

//Retrieves the voltage of battery, and changes it to battery percentage amount
void batteryLevel() {
  voltage = ESP.getVcc();

  //determines the battery percentage amount with voltage amount
  switch (voltage) {
    case 0 ... 750:
      batteryAmount = 20;
      break;
    case 751 ... 1500:
      batteryAmount = 50;
      break;
    case 1501 ... 2250:
      batteryAmount = 75;
      break;
    default:
      batteryAmount = 100;
      break;
  }
}

void connectToWifi() {
  //reset saved settings
  wifiManager.resetSettings();
  //fetches ssid and pass from eeprom and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration

  //initializing the email parameter
  WiFiManagerParameter custom_username("username", "Enter Email: ", username, 50);
  WiFiManagerParameter custom_deviceName("deviceName", "Enter Device Name: ", deviceName, 80);

  //If it restarts and router is not yet online, it keeps rebooting and retrying to connect
  wifiManager.setTimeout(120);

  //Placing parameter on wifiManager page
  wifiManager.addParameter(&custom_username);
  wifiManager.addParameter(&custom_deviceName);

  //Creating the access point for user to connect to
  wifiManager.autoConnect("ECOders Sensor");

  //or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();
  //if you get here you have connected to the WiFi
  Serial.println("Successfully connected to the wifi");

  //copy the username parameter values to the variable 'username'
  strcpy(username, custom_username.getValue());
  strcpy(deviceName, custom_deviceName.getValue());
}

void sendUserCredentialsToBackend() {
  //Place credentials in JSON document
  saveCredentialsInJsonDocument();

  // Use WiFiClientSecure class to create TLS connection
  client.setInsecure();
  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection failed");
    return;
  }
  else {
    Serial.println("Connection successful!");

    while (client.connected()) {
      client.print(String("POST ") + urlEmail + " HTTP/1.1\r\n" +
                   "Host: " + host + "\r\n" +
                   "Connection: close\r\n" +
                   "Content-Type: application/json" + "\r\n" +
                   "Content-Length: " + requestBodyLogin.length() + "\r\n" +
                   "\r\n" + requestBodyLogin + "\r\n");

      Serial.println("request sent");

      while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
          Serial.println("headers received");
          break;
        }
      }

      //additional quotation marks, so i have to remove them
      serverResponse = client.readStringUntil('\n');
      char no = '"';
      char c;
      for (int i = 0; i < serverResponse.length(); ++i) {
        c = serverResponse.charAt(i);
        if (c == no) {
          serverResponse.remove(i, 1);
        }
      }

      Serial.println("reply was:");
      Serial.println("==========");
      Serial.println(serverResponse);
      Serial.println("==========");
      Serial.println("closing connection");
    }
  }

}
//Saving the user login credentials in a JSON document
void saveCredentialsInJsonDocument() {
  JsonObject rootOne = userLoginDoc.to<JsonObject>();
  //sending data to JSON document
  rootOne["Email"] = username;
  rootOne["DeviceId"] = deviceID;
  rootOne["DeviceName"] = deviceName;
  serializeJsonPretty(userLoginDoc, requestBodyLogin);
  serializeJsonPretty(userLoginDoc, Serial);
}
//Place sensor data and other required parameters in a JSON document
void JSONDocument() {

}
//Retrieve sensor data
void readSensorData() {
  //  delay(10000); //wait 10 seconds
  //Determine reading from sensor

}
//Make sure SoilMoisturePercent is 0-100 value
void realisticPercentValue() {

}
