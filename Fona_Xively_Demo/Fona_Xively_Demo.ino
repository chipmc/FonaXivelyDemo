/* Chip McClelland - Cellular Data Logger
   BSD license, Please keep my name and the required Adafruit text in any redistribution

This sketch will connect to Xively and periodically updload the current temperature and humidity as well as the number
of times the PIR sensor had detected a person. The counts are then reset and you can access the data on your Xively site.

use can use the site - www.xively.com - for free development account.  Production use will cost money. 
The API set for Xively is defined here: https://xively.com/dev/docs/api/quick_reference/api_resources/

 IDE: Arduino 1.0 or later
 I had to add the following line to the Adafruit_FONA.cpp file: typedef char PROGMEM prog_char;
 Otherwise this code would not compile

 Hardware - Adafruit Fona GSM Board
 Temperature Sensor - Microchip TC74 I2C Temperature Sensor
 Person Counter - Parralax PIR Sensor - on D5

 I made use of the Adafruit Fona library and parts of the example code
 /*************************************************** 
  This is an example for our Adafruit FONA Cellular Module

  Designed specifically to work with the Adafruit FONA 
  ----> http://www.adafruit.com/products/1946
  ----> http://www.adafruit.com/products/1963

  These displays use TTL Serial to communicate, 2 pins are required to 
  interface
  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ****************************************************/

#include <SoftwareSerial.h>
#include "Adafruit_FONA.h"
#include <Wire.h>

#define FONA_RX 2
#define FONA_TX 3
#define FONA_RST 4
#define PIRpin 5
int temp_address = 72; //1001000 written as decimal number

// this is a large buffer for replies
char replybuffer[255];

SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
Adafruit_FONA fona = Adafruit_FONA(&fonaSS, FONA_RST);

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);

int f=0;                          // Temperature in degrees F
int Interval = 10000;             // Time between measurements in seconds
unsigned long Reporting = 20000;  // Time between uploads to Xively
int PersonCount = 0;              // Count the number of PIR hits in reporting interval
unsigned long LastReading = 0;    // When did we last read the sensors - they are slow so will read between sends
unsigned long LastReporting = 0;  // When did we last send data to Xively

void setup() {
  Wire.begin();
  Serial.begin(19200);
  Serial.println("");
  Serial.println(F("FONA basic test"));
  Serial.println(F("Initializing....(May take 3 seconds)"));
  
  // See if the FONA is responding
  if (! fona.begin(4800)) {  // make it slow so its easy to read!
    Serial.println(F("Couldn't find FONA"));
    while (1);
  }
  Serial.println(F("FONA is OK"));
  
  uint8_t n = fona.getNetworkStatus();  // Read the Network / Cellular Status
  Serial.print(F("Network status ")); 
  Serial.print(n);
  Serial.print(F(": "));
    if (n == 0) Serial.println(F("Not registered"));
    if (n == 1) Serial.println(F("Registered (home)"));
    if (n == 2) Serial.println(F("Not registered (searching)"));
    if (n == 3) Serial.println(F("Denied"));
    if (n == 4) Serial.println(F("Unknown"));
    if (n == 5) Serial.println(F("Registered roaming"));
  
  fona.enableGPRS(true);
  Serial.println(F("GPRS Serivces Started"));
  
  //Start talking to the Temperature sensor at the specified address
  Wire.beginTransmission(temp_address); 
  //Send a bit asking for register zero, the data register
  Wire.write(0); 
  //Complete Transmission 
  Wire.endTransmission(); 
}


void loop() {
  
  if (LastReporting + Reporting <= millis()) {   // This checks to see if it is time to send to Xively
    Send2Xively();
    LastReporting = millis();
  }
   
  if (LastReading + Interval <= millis()) {     // This checks to see if it is time to take a sensor reading
    Wire.requestFrom(temp_address, 1);          //Request 1 Byte from the specified address
    while(Wire.available() == 0);               //wait for response 
    int c = Wire.read();                        // Get the temp and read it into a variable  
    f = round(c*9.0/5.0 +32.0);                 //Do some math to convert the Celsius to Fahrenheit
    Serial.print(F("The current Temperature is: "));
    Serial.print(f);                             //Send the temperature in degrees F to the serial monitor
    Serial.print(F("F "));    
    
    if (digitalRead(PIRpin)) PersonCount++;     // Read the person counter too.
    Serial.print(F(" and we have counted "));
    Serial.print(PersonCount);
    Serial.println(F(" people"));
    LastReading = millis();                     // Record the time of the last sensor readings
  }
}

// Tthis function is to send the sensor data to Xively - each step is commented in the serial terminal
void Send2Xively()
{
  Serial.print(F("Start the connection to Xively: "));
  if (SendATCommand("AT+CIPSTART=\"tcp\",\"api.xively.com\",\"8081\"",'C','T')) {
    Serial.println("Connected");
  }
  Serial.print(F("Begin to send data to the remote server: "));
  if (SendATCommand("AT+CIPSEND",'\n','>')) {
    Serial.println("Sending");
  }
 fona.print("{\"method\": \"put\",\"resource\": \"/feeds/xxxxxxxxxx.json/\",\"params\"");   // Replace Feed ID with the one you get form Xively
 delay(50);                                               // Avoid filling up the serial buffer
 fona.print(": {},\"headers\": {\"X-ApiKey\":");          //in here, you should replace your API Key
 delay(50);
 fona.print("\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\""); // Replace API key with the one you get from Xively
 delay(50);
 fona.print("},\"body\":");                                          //Now the data
 delay(50);
 fona.print(" {\"version\": \"1.0.0\",\"datastreams\":");
 delay(50);
 fona.print("[{\"id\": \"Temperature\",\"current_value\" : \""); fona.print(f); fona.println("\"},");
 delay(50);
 fona.print("{\"id\": \"Person_Count\",\"current_value\" : \""); fona.print(PersonCount); fona.println("\"}]}}");
 PersonCount = 0;
 delay(50);
 fona.println((char)26);                                       //This terminates the JSON SEND with a carriage return
 Serial.print(F("Send JSON Package: "));
 if (SendATCommand("",'2','0')) {                              // The 200 code from Xively means it was successfully uploaded
    Serial.println("Sent");
  }
  delay(2000);
  
 Serial.print(F("Close connection to Xively: "));              // Close the connection
 if (SendATCommand("AT+CIPCLOSE",'G','M')) {
    Serial.println("Closed");
  }
}

boolean SendATCommand(char Command[], char Value1, char Value2) {
 unsigned char buffer[64];                                  // buffer array for data recieve over serial port
 int count = 0;
 int complete = 0;
 fona.println(Command);
 while(!complete)                                             // Need to give the modem time to complete command 
 {
   while(!fona.available());
   while(fona.available()) {                                 // reading data into char array 
     buffer[count++]=fona.read();                           // writing data into array
     if(count == 64) break;
   }
   //  Serial.write(buffer,count);                           // Uncomment if needed to debug
   for (int i=0; i <= count; i++) {
     if(buffer[i]==Value1 && buffer[i+1]==Value2) complete = 1; 
   }
 }
 return 1;                                                   // Returns "True"  - "False" sticks in the loop for now
}
