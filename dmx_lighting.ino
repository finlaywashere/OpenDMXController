#include <ArduinoSoftRS485.h>
#include <ArduinoSoftDMX.h>

#define RX1 2
#define TX1 3
#define RX2 4
#define TX2 5

#define RE1 A4
#define DE1 A5
#define RE2 A6
#define DE2 A7

#define UNIVERSE_SIZE 512

DMXClass* dmx1;
DMXClass* dmx2;

void setup() {
  Serial.begin(9600);
  while(!Serial);
  
  Serial.println("Initializing!");

  CustomSoftwareSerial *cs1 = new CustomSoftwareSerial(RX1,TX1);
  CustomSoftwareSerial *cs2 = new CustomSoftwareSerial(RX2,TX2);
  
  RS485Class* rs485_1 = new RS485Class(*cs1,TX1,DE1,RE1);
  RS485Class* rs485_2 = new RS485Class(*cs2,TX2,DE2,RE2);

  dmx1 = new DMXClass(*rs485_1);
  dmx2 = new DMXClass(*rs485_2);
  
  if(!dmx1->begin(UNIVERSE_SIZE)){
    Serial.println("Failed to initialize DMX #1!");
    while(1);
  }
  if(!dmx2->begin(UNIVERSE_SIZE)){
    Serial.println("Failed to initialize DMX #2!");
    while(1);
  }
}

void loop() {
  dmx1->beginTransmission();
  dmx1->write(1,255);
  dmx1->write(2,255);
  dmx1->endTransmission();

  delay(1000);

  dmx1->beginTransmission();
  dmx1->write(1,0);
  dmx1->write(2,0);
  dmx1->endTransmission();

  delay(1000);
}
