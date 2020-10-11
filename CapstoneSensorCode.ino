//MAKE SURE TO LOOK OVER THE DEVICE ID, AND MAKE SURE THE ONE YOU ARE SENDING IS AN INT AND NOT A STRING
//make sure to add the into at the top, mentioning who the code is by and all that 




#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

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
StaticJsonDocument<200> doc;
String requestBody;
StaticJsonDocument<200> userLoginDoc;
String requestBodyLogin;

//Wifi Manager Variables
WiFiManager wifiManager;
WiFiClientSecure client;
char username[50];
char deviceName[80];

//Device ID and Memory Variables
int deviceID;
String MemoryVar;
bool idInMemory = false;


// Required for LIGHT_SLEEP_T delay mode
extern "C" {
  #include "user_interface.h"
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  
  connectToWifi();

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
  }
  //Send the user email, plant name and device id to server
  sendUserCredentialsToBackend();
}

void loop() {  
  //determine what the server response is after retrieving the user email, plant name and device id
  if (serverResponse == "HTTP/1.1 520 Origin Error") {
    Serial.println("The user email does not exist in the database");
    //TO DO: figure out how to redirect the user to be able to successfully input their credentials again 
  }
  else {
    //send the sensor data to the server if user email is valid
    createTLSConnection();
  }
  
  //Loop every 3 minutes
  delay(60000*3-800); 
}


//Create a secure connection and send sensor data to the server
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

//Determines if device id is in the device's memory
bool checkDeviceIDInMemory() {
  for (int i = 0; i < 9; i++) {
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
  //seperate each digit from the deviceID and save it in seperate variables
  int n1,n2,n3,n4,n5,n6,n7,n8,n9;
  n9 = deviceID % 10;
  n8 = deviceID / 10 % 10;
  n7 = deviceID / 100 % 10;
  n6 = deviceID / 1000 % 10;
  n5 = deviceID / 10000 % 10;
  n4 = deviceID / 100000 % 10;
  n3 = deviceID / 1000000 % 10;
  n2 = deviceID / 10000000 % 10;
  n1 = deviceID / 100000000 % 10;
  
  //Write these numbers to the eeprom
  EEPROM.write(0,n1);
  EEPROM.write(1,n2);
  EEPROM.write(2,n3);
  EEPROM.write(3,n4);
  EEPROM.write(4,n5);
  EEPROM.write(5,n6);
  EEPROM.write(6,n7);
  EEPROM.write(7,n8);
  EEPROM.write(8,n9);

  EEPROM.commit();
}

void lightSleep() {
  Serial.println("ESP8266 going into light sleep for 15 minutes");
  WiFi.mode(WIFI_STA);
  //wifi_set_sleep_type(LIGHT_SLEEP_T);
  //delay(60000*15);
  ESP.deepSleep(900e6);
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

    //initializing the email parameter
    WiFiManagerParameter custom_username("username", "Enter Username: ", username, 50);
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
  
  Serial.println("\nStarting connection to server...");
  if (!client.connect(host, 443))
    Serial.println("Connection failed!");
  else {
    Serial.println("Connection successful!");
  }
  
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
    serverResponse = client.readStringUntil('\n');
    Serial.println("reply was:");
    Serial.println("==========");
    Serial.println(serverResponse);
    Serial.println("==========");
    Serial.println("closing connection");
  }
  
}
//Saving the user login credentials in a JSON document
void saveCredentialsInJsonDocument() {
  JsonObject rootOne = userLoginDoc.to<JsonObject>();
  //sending data to JSON document
  rootOne["Email"] = username;
  rootOne["DeviceId"] = MemoryVar;
  rootOne["DeviceName"] = deviceName;
  serializeJsonPretty(userLoginDoc, requestBodyLogin);
  serializeJsonPretty(userLoginDoc, Serial);
}
//Place sensor data and other required parameters in a JSON document
void JSONDocument() {
  JsonObject root = doc.to<JsonObject>();
  //sending data to JSON document
  root["UID"] = serverResponse;
  root["DeviceId"] = MemoryVar;
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
