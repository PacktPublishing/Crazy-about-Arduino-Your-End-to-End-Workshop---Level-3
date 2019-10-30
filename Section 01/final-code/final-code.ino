/* 
   Udemy Course: Crazy about Arduino - Your End-to-End Workshop - Level-3 
   Website: https://www.udemy.com/crazy-about-arduino-your-end-to-end-workshop-level-3/
   Description:   
   Author: Idan Gabrieli
*/
/******************************************/
#include <DHT.h>
#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>  // must be included here so that Arduino library object file references work

/*--------DHT sensor-------------------------*/
// variables declaration
#define DHTTYPE DHT11   // DHT 11
const byte dhtPIN=8;
// initialize DHT sensor.
DHT dht(dhtPIN, DHTTYPE);

/*--------ultrasonic sensor------------------*/
const int triggerPin = 12;      // pin connected to Trig Pin in the ultrasonic distance sensor
const int echoPin = 11;         // pin connected to Echo Pin in the ultrasonic distance sensor
const int thresholdLedPin = 13; // pin connected to threshold LED indicator  
const int thresholdLevel = 20;  // minimum distance in cm to turn ON the LED 
const long interval=60000; //10 seconds 
unsigned long previousMillis = 0;
unsigned long currentMillis = 0;

/*--------PIR sensor-------------------------*/
const int pirSensorPin=3;      
/*--------LDR sensor-------------------------*/
const int ldrPin = A1;

// 0 - Temperature; 1 - Humidity; 2 - Noise; 3 -Light 
float currentSensors[4];
// sensors thresholds levels (reading from MySQL DB) 
int thresholdRange[4][2];


//the time we give the sensor to calibrate (10-60 secs according to the datasheet)
const int calibrationTime = 10;    
//the time when the sensor outputs a low impulse
long unsigned int lowIn;  
//the amount of milliseconds the sensor has to be low 
//before we assume all motion has stopped
long unsigned int minPause = 5000;  
boolean lockLow = true;
boolean takeLowTime;  

/*--------Ethernet-------------------------*/
// the MAC - media access control (ethernet hardware) address for the Ethernet shield
byte mac[] = { 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };
char webServerIP[] = "10.100.102.4"; 
//char webServerURL =""; 
String httpResponse;

// initialize the Ethernet client library
EthernetClient client;

/******************************************/
void setup() 
{
/*----open serial communications and setup digital pins mode--------*/ 
  Serial.begin(9600);
/*----setting each digital pin mode---------------------------------*/
  pinMode(triggerPin,OUTPUT);           // Trigger is an output pin
  pinMode(echoPin,INPUT);               // Echo is an input pin
  pinMode(thresholdLedPin,OUTPUT);      // LED is an output pin
  digitalWrite(thresholdLedPin, LOW);
  pinMode (pirSensorPin, INPUT); // set PIR sensor as input 

/*----start the ethernet connection with DHCP IP allocation-----*/
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    do {} while (1); 
  }
/*----print the allocated IP address*/
  printIPAddress();
/*----start the DHT sensor---------*/
  dht.begin();
  delay(1000);     // let the DHT sensor some starting time 
  setupThresholdsLevels();
  Serial.println("System is ready...");
}
/******************************************/
void loop() {
  currentMillis = millis();
  if(currentMillis - previousMillis > interval) { 
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
    String sensorsData="sensor1="+String(currentSensors[0])+"&sensor2="+String(currentSensors[1])+"&sensor3="+String(currentSensors[2])+"&sensor4="+String(currentSensors[3]);
    httpRequest("GET /mydatalogger/addget.php?"+sensorsData+" HTTP/1.0");
    checkThresholdsLevels();
  }
  refreshIP();
}
/******************************************/
void printIPAddress()
{
  Serial.print("Using DHCP- Arduino IP address: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print(".");
  }
  Serial.println(Ethernet.localIP());
  Serial.println();
}
/******************************************/
void refreshIP()
{
  switch (Ethernet.maintain())
    {
      case 1:
        //renewed fail
        Serial.println("Error: renewed fail");
        break;
      case 2:
        //renewed success
        Serial.println("Renewed success");
        //print your local IP address:
        printIPAddress();
        break;
      case 3:
        //rebind fail
        Serial.println("Error: rebind fail");
        break;
      case 4:
        //rebind success
        Serial.println("Rebind success");
        //print your local IP address:
        printIPAddress();
        break;
      default:
        //nothing happened
        break;
  }
}
/******************************************/
/*--------push data to the web server with GET request-------*/
void httpRequest(String request) 
{
   if (client.connect(webServerIP,80)) { 
      Serial.println("connected.");
/*----send sensors data to the web server using GET request---*/ 
      Serial.println("sending data to web server...");      
      Serial.println(request);
      Serial.println("Connection: close"); // telling the server that we are over transmitting the message
      Serial.println(); // empty line
      client.println(request);
      client.println("Connection: close"); // telling the server that we are over transmitting the message
      client.println(); // empty line
/*----display the response message from the server---*/
      webServerIPResponse();
    }
    else {
    // If Arduino can't connect to the server (your computer or web page)
      Serial.println("--> connection failed\n");
    }
    if (client.connected()) { 
      client.stop();  
    }
}
/******************************************/
void webServerIPResponse() 
{
  long timestamp=millis();
  httpResponse=""; // used to store the relevant string on the thresholds level stored in the database 
  bool start=false; 
  do {
    if (client.available()) {
      char c = client.read();
//      Serial.print(c);
      if (start) httpResponse=httpResponse+c;
      if (c=='@') start=true; 
    }
  } while ((millis()-timestamp)<=3000);

  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();    // closing connection to server
  } 
}
/******************************************/
void setupThresholdsLevels() 
{
  Serial.println("sending query request to web server...");      
  httpRequest("GET /mydatalogger/read_thresholds.php HTTP/1.0");
  Serial.println("parsing and storing the thresholds level...");      
//  Serial.println("********************");
//  Serial.println(httpResponse);
//  Serial.println("parsing the data:");
  for(byte i=0; i<=3; i++) {
    thresholdRange[i][0]=stringParser(i*2+1,httpResponse);
    Serial.println(thresholdRange[i][0]);
    thresholdRange[i][1]=stringParser(i*2+2,httpResponse);
    Serial.println(thresholdRange[i][1]);
  }
 Serial.println("done"); 
}
/******************************************/
int stringParser(byte parameterNum, String str1) 
{
  byte counter=0; byte i=0;
  while (counter!=parameterNum) {
    if (str1[i]=='$') counter++;
    i++;
  }
  byte from=i;
  while (counter!=parameterNum+1) {
    if (str1[i]=='$' || i==str1.length()) counter++;
    i++;
  }
  byte upto=i;  
  String str2=str1.substring(from,upto);
  return(str2.toInt());
}
/******************************************/
void checkThresholdsLevels() 
{
  int minimum, maximum;
  for(byte i=0;i<=1;i++) {
    minimum=thresholdRange[i][0];
    maximum=thresholdRange[i][1];
/*----check if sensor value crossed the threshold range (min and max)-----*/
    if(currentSensors[i]<=minimum || currentSensors[i]>=maximum) {
      String thresholdData = "sensorName="+getSensorName(i)+"&sensorValue="+String(currentSensors[i]); 
      httpRequest("GET /mydatalogger/threshold_event.php?"+thresholdData+" HTTP/1.0"); 
    }
  }
}
/******************************************/
String getSensorName(byte index) 
{
  switch (index) {
     case 0: return "temperature";
     case 1: return "humidity";
     case 2: return "noise";
     case 3: return "light";
  }
}
/******************************************/
/******************************************/
// LDR Sensor Functions 
/******************************************/
String getSensorLight()
{
  int value = analogRead(ldrPin);
  String lightLevel;
//  Serial.print("Analog Reading = ");
//  Serial.print(value); 
  if(value < 200)      lightLevel="Dark      ";
  else if(value < 400) lightLevel="Dim       ";
  else if(value < 600) lightLevel="Light     ";  
  else if(value < 800) lightLevel="Bright    ";
  else                 lightLevel="V.Bright";  
  return lightLevel; 
}
/******************************************/
// Ultrasonic Sensor Functions 
/******************************************/
int getSensorDistance()
{
  long duration;
  int cm, inches; 
  digitalWrite(triggerPin, LOW);                   
  delayMicroseconds(2);
  digitalWrite(triggerPin, HIGH);          // Trigger pin to HIGH
  delayMicroseconds(10);                   // 10us high 
  digitalWrite(triggerPin, LOW);           // Trigger pin to LOW
  duration = pulseIn(echoPin,HIGH);        // Waits for the echo pin to get high and returns the duration in microseconds 
/* 
The echo ping travels out and back, so to find the distance of the 
object we take half of the distance travelled.  The speed of sound is 340.29 m/s or 29 microseconds per centimeter.
340.29 m/s --> 34029 cm/s --> 29.386 micro seconds/cm 
*/
  cm = (duration/2) / 29.386;
  inches = (duration/2) / 74;  
  Serial.print("Distance = ");             // Output to serial
  Serial.print(cm);
  Serial.println(" cm");   
//  Serial.print(inches);
//  Serial.print("in, ");

// checking if threshold level was crossed 
  if (cm <= thresholdLevel) 
    digitalWrite(thresholdLedPin, HIGH); 
  else
    digitalWrite(thresholdLedPin, LOW); 
  return cm;
}
/******************************************/
void CheckPIR()
{
     if(digitalRead(pirSensorPin) == HIGH) //motion is detected by the PIR sensor
     {
       digitalWrite(13, HIGH);   //the led visualizes the sensors output pin state          
       if(lockLow)
       {  
         //makes sure we wait for a transition to LOW before any further output is made:
         lockLow = false;            
         Serial.println("---");
         Serial.print("motion detected at ");
         Serial.print(millis()/1000);
         Serial.println(" sec"); 
         delay(50);
       }         
         takeLowTime = true;
     }

     if(digitalRead(pirSensorPin) == LOW) //stopped detecting motion 
     {       
       digitalWrite(13, LOW);  //the led visualizes the sensors output pin state

       if(takeLowTime) // taking the time only once when crossing from high to low 
       {
        lowIn = millis();          //save the time of the transition from high to LOW
        takeLowTime = false;       //make sure this is only done at the start of a LOW phase
       }
       //if the sensor is low for more than the given pause, 
       //we assume that no more motion is going to happen
       int pause = millis() - lowIn;
       if(!lockLow && (pause > minPause))
       {  
           //makes sure this block of code is only executed again after 
           //a new motion sequence has been detected
           lockLow = true;                        
           Serial.print("motion ended at ");      //output
           Serial.print((millis() - minPause)/1000);
           Serial.println(" sec");
           delay(50);
       }
    }   
}
//******************************************
//  END OF PROGRAM
//******************************************

