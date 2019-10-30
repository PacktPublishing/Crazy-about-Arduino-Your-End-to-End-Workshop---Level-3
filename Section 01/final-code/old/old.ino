/* 
   Udemy Course: Crazy about Arduino - Your End-to-End Workshop 
   Level:3 
   Website: https://www.udemy.com/crazy-about-arduino-your-end-to-end-workshop-level-3/
   Description:   
   Author: Idan Gabrieli
   Date: 11/2016 
*/
/******************************************/
#include <DHT.h>
#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>  // must be incuded here so that Arduino library object file references work

#define DHTTYPE DHT11   // DHT 11

// enter the Ethernet Shield MAC address 
byte mac[] = { 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };
char webServer[] = "10.100.102.2"; 

// initialize the Ethernet client library
EthernetClient client;

// variables declaration
const int lightPIN = A0;
const int noisePIN = A1;
const int dhtPIN=8;
//const int buzzerPIN = 10;

// initialize DHT sensor.
DHT dht(dhtPIN, DHTTYPE);

const long interval=10000; //10 seconds 
long previousMillis = 0;
unsigned long currentMillis = 0;
// 0 - Temperature; 1 - Humidity; 2 - Noise; 3 -Light 
float currentSensors[4];
// sensors thresholds levels (reading from MySQL DB) 
int thresholdRange[4][2];

/******************************************/
void setup() 
{
/*----open serial communications--------*/ 
  Serial.begin(9600);
/*----setting each digital pin mode-----*/
//  pinMode(buzzerPin,OUTPUT);            // Buzzer is an output 
/*----start the ethernet connection with DHCP IP allocation-----*/
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
  }
/*----print the allocated IP address*/
  Serial.print("Using DHCP- Arduino IP address: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print(".");
  }
  Serial.println();
/*----start the DHT sensor---------*/
  dht.begin();
  delay(1000);     // let the DHT sensor some starting time 
  setupThresholdsLevels();
  Serial.println("System is ready...");
}
/******************************************/
void loop() {
  currentMillis = millis();
  if(currentMillis - previousMillis > interval) { // READ ONLY ONCE PER INTERVAL
    previousMillis = currentMillis;
    currentSensors[0] = dht.readTemperature();
    currentSensors[1] = dht.readHumidity(); 
    currentSensors[2] = 0;
    currentSensors[3] = 0;
    Serial.print("Temperature: ");
    Serial.print(currentSensors[0]);
    Serial.print(" *C ");
    Serial.print("Humidity: ");
    Serial.print(currentSensors[1]);
    Serial.print(" %\t"); 
/*    
// Compute heat index in Celsius
    float hic = dht.computeHeatIndex(currentSensors[0], currentSensors[1], false);   
    Serial.print("Heat index: ");
    Serial.print(hic);
    Serial.println(" *C ");   
*/
     
/*--------push data to the web server with GET request-------*/
    if (client.connect(webServer,80)) { 
      Serial.println("connected.");
/*----send sensors data to the web server using GET request---*/ 
      sendDataWebserver();
/*----display the response message from the server---*/
      displayWebserverResponse();
      checkThresholdsLevels();
    }
    else {
    // If Arduino can't connect to the server (your computer or web page)
      Serial.println("--> connection failed\n");
    }
    if (client.connected()) { 
      client.stop();  // DISCONNECT FROM THE SERVER
    }
  }
}
/******************************************/
void sendDataWebserver() {
  String sensorsData="sensor1="+String(currentSensors[0])+"&sensor2="+String(currentSensors[1])+"&sensor3="+String(currentSensors[2])+"&sensor4="+String(currentSensors[3]);
  Serial.println("sending sensors data to web server...");      
  Serial.println("GET /mydatalogger/addget.php?"+sensorsData+" HTTP/1.0");
  Serial.println("Connection: close"); // telling the server that we are over transmitting the message
  Serial.println(); // empty line
  client.println("GET /mydatalogger/addget.php?"+sensorsData+" HTTP/1.0");
  client.println("Connection: close"); // telling the server that we are over transmitting the message
  client.println(); // empty line
}
/******************************************/
void displayWebserverResponse() {
  long timestamp=millis();
  do {
    if (client.available()) {
      char c = client.read();
      Serial.print(c);
    }
  } while ((millis()-timestamp)<=10000);
  
  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();    // closing connection to server
  } 
}
/******************************************/
void setupThresholdsLevels() {
/*
  Serial.println("sending query request to web server...");      
  Serial.println("GET /mydatalogger/read_thresholds.php HTTP/1.0");
  Serial.println("Connection: close"); // telling the server that we are over transmitting the message
  Serial.println(); // empty line
  client.println("GET /mydatalogger/read_thresholds.php HTTP/1.0");
  client.println("Connection: close"); // telling the server that we are over transmitting the message
  client.println(); // empty line
*/
  thresholdRange[0][0]=10;
  thresholdRange[0][1]=25;
  thresholdRange[1][0]=0;
  thresholdRange[1][1]=100;
}
/******************************************/
void checkThresholdsLevels() {
  int minimum, maximum;
  /*--------push data to the web server with GET request-------*/
    if (client.connect(webServer,80)) { 
      Serial.println("connected.");
/*----send sensors data to the web server using GET request---*/ 
      sendDataWebserver();
/*----display the response message from the server---*/
      displayWebserverResponse();
      checkThresholdsLevels();
    }
    else {
    // If Arduino can't connect to the server (your computer or web page)
      Serial.println("--> connection failed\n");
    }
    if (client.connected()) { 
      client.stop();  // DISCONNECT FROM THE SERVER
    }
  }
  for(int i=0;i<=1;i++) {
    minimum=thresholdRange[i][0];
    maximum=thresholdRange[i][1];
/*----check if sensor value crossed the threshold range (min and max)-----*/
    if(currentSensors[i]<=minimum || currentSensors[i]>=maximum) {
      String thresholdData = "sensorName="+getSensorName(i)+"&sensorValue="+String(currentSensors[i]); 
      Serial.println("sending threshold event to web server...");      
      Serial.println("GET /mydatalogger/threshold_event.php?"+thresholdData+" HTTP/1.0");
      Serial.println("Connection: close"); // telling the server that we are over transmitting the message
      Serial.println(); // empty line
      client.println("GET /mydatalogger/threshold_event.php?"+thresholdData+" HTTP/1.0");
      client.println("Connection: close"); // telling the server that we are over transmitting the message
      client.println(); // empty line
      displayWebserverResponse();
      do {} while (1);
    }
  }
}
/******************************************/
String getSensorName(int index) {
  switch (index) {
     case 0: return "temperature";
     case 1: return "humidity";
     case 2: return "noise";
     case 3: return "light";
  }
}

