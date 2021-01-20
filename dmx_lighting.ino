/*
  DMX Fade

  This sketch fades the value of DMX channel 1 between 0 and 255 in steps to create a fade effect.
  All other slots are set to a value of 0.

  Circuit:
   - DMX light
   - MKR board
   - MKR 485 shield
     - ISO GND connected to DMX light GND (pin 1)
     - Y connected to DMX light Data + (pin 2)
     - Z connected to DMX light Data - (pin 3)
     - Jumper positions
       - Z \/\/ Y set to ON

  created 5 July 2018
  by Sandeep Mistry
*/

#include <ArduinoRS485.h> // the ArduinoDMX library depends on ArduinoRS485
#include <ArduinoDMX.h>

const int universeSize = 512;

int brightness = 0;
int fadeAmount = 5;

DMXClass *dmx_1;
DMXClass *dmx_2;

void setup() {
  //pinMode(3,OUTPUT);
  //digitalWrite(3,HIGH);

  RS485Class *rs485_1 = new RS485Class(Serial1,18,A5,A5);
  RS485Class *rs485_2 = new RS485Class(Serial2,16,A7,A7);
  dmx_1 = new DMXClass(*rs485_1);
  dmx_2 = new DMXClass(*rs485_2);
  
  Serial.begin(9600);

  // initialize the DMX library with the universe size
  if (!dmx_1->begin(universeSize)) {
    Serial.println("Failed to initialize DMX!");
    while (1); // wait for ever
  }
  if (!dmx_2->begin(universeSize)) {
    Serial.println("Failed to initialize DMX!");
    while (1); // wait for ever
  }
  Serial.println("Initialized DMX!");
}

void loop() {
  // set the value of channel 2
  dmx_1->beginTransmission();
  dmx_1->write(1, 255);
  dmx_1->write(2, brightness);
  dmx_1->endTransmission();

  // change the brightness for the next round
  brightness += fadeAmount;

  // reverse fade direction when on edge of range
  if (brightness <= 0 || brightness >= 255) {
    fadeAmount = -fadeAmount;
  }

  // delay for dimming effect
  delay(30);
}
