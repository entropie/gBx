#include <OneWire.h>
#include <DallasTemperature.h>

#include "ThingSpeak.h"

#include <ArduinoJson.h>

#include <SPI.h>
#include <Ethernet.h>

#include <Wire.h>

// DS18B20 / OneWire
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);


const char * myCounterReadAPIKey = "";
unsigned int counterFieldNumber = 1;
const char * myWriteAPIKey = "UH453E629HFU1DS3";
unsigned long myChannelNumber = 782398;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };


// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 0, 146);
IPAddress myDns(8, 8, 8, 8);

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;

int number = 0;


// Addresses of 2 DS18B20s
uint8_t sensor1[8] = { 0x28, 0xAA, 0xCF, 0xB3, 0x1D, 0x13, 0x02, 0x86 };
uint8_t sensor2[8] = { 0x28, 0xCD, 0xD1, 0xF0, 0x32, 0x14, 0x01, 0x31 };


const int soilHygrometers[] = {
  A0,
  A1
};


typedef struct {
     String ident;
     double value;
} Result;
#define RESULT_SIZE 5


void setup(void) {
  Serial.begin(9600);

  
  // start the Ethernet connection:
  Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
      while (true) {
        delay(1); // do nothing, no point running without Ethernet hardware
      }
    }
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip, myDns);
  } else {
    Serial.print("  DHCP assigned IP ");
    Serial.println(Ethernet.localIP());
  }
  // give the Ethernet shield a second to initialize:
  delay(1000);


  ThingSpeak.begin(client); 
  sensors.begin();
}

void loop(void) {
  sensors.requestTemperatures();

  Result result[RESULT_SIZE];

  result[0].ident = "tmp0";
  result[0].value = 0;

  // DS18B20
  result[1].ident = "tmp1";
  result[1].value = getTemperature(sensor1);

  result[2].ident = "tmp2";
  result[2].value = getTemperature(sensor2);


  int resOffset = 3;

  String ident;
   // hygros
  for (int i=0; i<sizeof soilHygrometers/sizeof soilHygrometers[0]; i++) {
    int s = soilHygrometers[i];
    ident = "hygro" + String(i);
    result[resOffset + i].ident = ident;
    result[resOffset + i].value = getSoilHumidity(s);
  };
  
  for (int i=0; i<sizeof result/sizeof result[0]; i++) {
    Result s = result[i];
    Serial.print("  ");
    Serial.print(s.ident);
    Serial.print(" : ");
    Serial.print(s.value);

    Serial.println();
    
    ThingSpeak.setField(i+1, (String)s.value);
  }

  int x;
  // write to the ThingSpeak channel
  x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if(x == 200){
    Serial.println("Channel update successful.");
  }
  else{
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }

  number++;
  if(number > 99){
    number = 0;
  }
  delay(30000);
}


double getTemperature(DeviceAddress deviceAddress) {
  float tempC = sensors.getTempC(deviceAddress);
  return tempC;
}

int getSoilHumidity(int analogDevice) {
  int value = analogRead(analogDevice);
  value = constrain(value,400,1023); 
  value = map(value,400,1023,100,0);
  return value; 
}
