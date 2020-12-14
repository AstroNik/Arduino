/*
 * 
 * Code Written By: Vedika Maheshwari
 * Date: Sunday November 8th, 2020
 * Description: Code for the arduino device to send sensor data to the server
 * 
*/


#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <WiFiManager.h>

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

//JSON Document Variables for user credential check
StaticJsonDocument<200> userLoginDoc;
String requestBodyLogin;

//Wifi Manager Variables 
WiFiManager wifiManager;
WiFiClientSecure client;
char username[50];
char deviceName[80];

//Device ID Variable
String deviceID = "987654321";
String MemoryVar;
bool idInMemory = false;

//Battery Amount Variables
int batteryAmount = 100;
int voltage;

//needed to use C SDK based fuctions - needed for light sleep
extern "C" {
  #include "user_interface.h"
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(1019);

  //Connect to Wi-Fi 
  connectToWifi();

  /*
  //check if device id is in memory 
  if (checkDeviceIDInMemory() == true) {
    Serial.println("The device id is in memory");
    //read the device id from memory, and save it to deviceID variable
    for (int i=0; i<9; i++){
      MemoryVar += EEPROM.read(i); 
    }
    Serial.println(MemoryVar);
  }
  else {
    Serial.println("New device id will be assigned to this device");
    getDeviceID();
    Serial.println(MemoryVar);
  }*/
  
  //Send the user email, plant name and device id to server
  sendUserCredentialsToBackend();
}

void loop() {  
  //check the server response to determine if the user email exists in the database
  if (serverResponse == "UserNotFound") {
    Serial.println("The user email does not exist in the database");
  }
  else {
    //send the sensor data to the server if user email is valid
    createTLSConnection();
  }  
}

//Create a secure connection and send sensor data to the server
void createTLSConnection() {
  //JSON Document Variables for sensor data 
  StaticJsonDocument<200> doc;
  String requestBody;

  /* 
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
      case 2251 ... 5000:
        batteryAmount = 100;
        break;
      default: 
        batteryAmount = 0;
        break;
    }
  }*/

  //Determine data readings from sensor
  SoilMoistureValue = analogRead(A0);
  SoilMoisturePercent = map(SoilMoistureValue, AirValue, WaterValue, 0, 100); 

  //The value of SoilMoisturePercent can equal to negative values and values over 100
  //Check was created to make sure SoilMoisturePercent is in the 0-100 percent range
  if (SoilMoisturePercent > 100) {
    SoilMoisturePercent = 100;
  } 
  else if (SoilMoisturePercent < 0) {
    SoilMoisturePercent = 0;
  }
  
  // Use WiFiClientSecure class to create TLS connection
  client.setInsecure();

  //Place serverResponse, deviceID, batteryAmount and sensor data in a JSON document
  JsonObject root = doc.to<JsonObject>();
  //Placing data in JSON document
  root["UID"] = serverResponse;
  root["DeviceId"] = voltage;
  root["Battery"] = batteryAmount;
  root["AirValue"] = AirValue;
  root["WaterValue"] = WaterValue;
  root["SoilMoistureValue"] = SoilMoistureValue;
  root["SoilMoisturePercent"] = SoilMoisturePercent;

  //Save the JSON document data as a string
  serializeJsonPretty(doc, requestBody);
  serializeJsonPretty(doc, Serial);

  //Connect to the host
  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection failed");
    return;
  }
  else {
    Serial.println("Connection successful!");
    while (client.connected()) {
      //Sending POST request to server with JSON document
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
  
  //After sending the post request, device goes into light sleep for 15 minutes
  lightSleep();
}

//Determine if device id is in the device's memory
bool checkDeviceIDInMemory() {
  for (int i = 0; i < 9; i++) {
    Serial.println(EEPROM.read(i));
    if (EEPROM.read(i) != 0) {
      idInMemory = true;
      break;
    }
    Serial.println("This byte is a 0");
  }
  return idInMemory;
}

//Creates a random new device id 
void getDeviceID() {
  deviceID = random(1,999999999);
  Serial.println("");
  Serial.println("Device ID: ");
  Serial.println(deviceID);
  Serial.println("");

  MemoryVar = (String)deviceID;
  
  addDataToEEPROM();
}

//Saves each int seperately into the memory as bytes
void addDataToEEPROM(){
  //initializing variables to hold each digit seperately 
  int n1,n2,n3,n4,n5,n6,n7,n8,n9;

  //seperate each digit from the deviceID and save it in seperate variables
  //start from the right most digit
  n9 = deviceID % 10;
  n8 = deviceID / 10 % 10;
  n7 = deviceID / 100 % 10;
  n6 = deviceID / 1000 % 10;
  n5 = deviceID / 10000 % 10;
  n4 = deviceID / 100000 % 10;
  n3 = deviceID / 1000000 % 10;
  n2 = deviceID / 10000000 % 10;
  n1 = deviceID / 100000000 % 10;
  
  //Write each digit to EEPROM
  EEPROM.write(0,n1);
  EEPROM.write(1,n2);
  EEPROM.write(2,n3);
  EEPROM.write(3,n4);
  EEPROM.write(4,n5);
  EEPROM.write(5,n6);
  EEPROM.write(6,n7);
  EEPROM.write(7,n8);
  EEPROM.write(8,n9);

  //commit to EEPROM
  bool var = EEPROM.commit();
  Serial.println("Commit value: ");
  Serial.println(var);
}

//
void lightSleep() {
  Serial.println("ESP8266 going into light sleep for 15 minutes");
  WiFi.mode(WIFI_STA);
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  delay(60000*15);
}

//Connect to wifi and retrieve user email 
void connectToWifi() {
    //reset saved settings
    wifiManager.resetSettings();
    //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name "ECOders Sensor"

    //initializing the email and deviceName parameter
    WiFiManagerParameter custom_username("username", "Enter Email: ", username, 50);
    WiFiManagerParameter custom_deviceName("deviceName", "Enter Device Name: ", deviceName, 80);
    
    //If it restarts and router is not yet online, it keeps rebooting and retrying to connect
    wifiManager.setTimeout(180);

    //Placing parameters on wifiManager's page
    wifiManager.addParameter(&custom_username);
    wifiManager.addParameter(&custom_deviceName);

    //Creating the access point for user to connect to
    wifiManager.autoConnect("ECOders Sensor");
    
    //if you get here you have connected to the WiFi
    Serial.println("Successfully connected to the wifi");

    //copy the username parameter values to the variable 'username'
    strcpy(username, custom_username.getValue());
    strcpy(deviceName, custom_deviceName.getValue());
}

void sendUserCredentialsToBackend() {
  //Place credentials in JSON document
  saveCredentialsInJsonDocument();

  //Use WiFiClientSecure class to create secure connection - figure out what this line does
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
      //Sending POST request to server with JSON document
      client.print(String("POST ") + urlEmail + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n" + 
                 "Content-Type: application/json" + "\r\n" +
                 "Content-Length: " + requestBodyLogin.length() + "\r\n" +
                 "\r\n" + requestBodyLogin + "\r\n");
                 
      Serial.println("request sent");

      //Retrieve and save server response in variable, serverResponse
      //Another while loop is needed, otherwise the serverResponse consists of all specific details, whereas we only need the UID as the serverResponse
      while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
          Serial.println("headers received");
          break;
        }
      }

      //Server sends the serverResponse with a pair of quotation marks to indicate a string
      //The arduino also adds an additional pair of quotation marks to indicate a string
      //Therefore, the additional pair of quotation marks have to be removed so that the server can handle request being sent otherwise data is not sent successfully
      char quotationMarks = '"';
      char serverResponseCharacter;

      serverResponse = client.readStringUntil('\n');
      //Checks each char in the server response to find and remove all quotation marks from the serverResponse
      for (int i = 0; i < serverResponse.length(); ++i){
          serverResponseCharacter = serverResponse.charAt(i);
          if(serverResponseCharacter == quotationMarks){
              serverResponse.remove(i, 1);
          }
      }
      
      //printing out server response onto the serial
      Serial.println("reply was:");
      Serial.println("==========");
      Serial.println(serverResponse);
      Serial.println("==========");
      Serial.println("closing connection");
    }
  }
}

//Saving user email, deviceID and user created device name in a JSON document
void saveCredentialsInJsonDocument() {
  JsonObject rootOne = userLoginDoc.to<JsonObject>();
  //sending data to JSON document
  rootOne["Email"] = username;
  rootOne["DeviceId"] = deviceID;
  rootOne["DeviceName"] = deviceName;

  //Save the JSON document authentication data as a string
  serializeJsonPretty(userLoginDoc, requestBodyLogin);
  serializeJsonPretty(userLoginDoc, Serial);
}
