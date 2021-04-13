/*
    Copyright (C) 2021 Finlay Maroney

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <ArduinoRS485.h> // the ArduinoDMX library depends on ArduinoRS485
#include <ArduinoDMX.h>

#define LED3 40
#define LED2 39
#define LED1 4

const int universeSize = 512;

DMXClass *dmx_1;
DMXClass *dmx_2;

void setup() {
  pinMode(LED3,OUTPUT);
  pinMode(LED2,OUTPUT);
  pinMode(LED1,OUTPUT);
  RS485Class *rs485_1 = new RS485Class(Serial1,18,A5,A5);
  RS485Class *rs485_2 = new RS485Class(Serial2,16,A7,A7);
  dmx_1 = new DMXClass(*rs485_1);
  dmx_2 = new DMXClass(*rs485_2);
  
  Serial.begin(57600);
  
  // initialize the DMX library with the universe size
  if (!dmx_1->begin(universeSize)) {
    //Serial.println("Failed to initialize DMX!");
    digitalWrite(LED3,HIGH);
    while (1); // wait for ever
  }
  if (!dmx_2->begin(universeSize)) {
    //Serial.println("Failed to initialize DMX!");
    digitalWrite(LED3,HIGH);
    while (1); // wait for ever
  }
  Serial.println("Initialized DMX!");
  digitalWrite(LED2,HIGH);
}

void loop() {
  dmx_1->beginTransmission();
  dmx_2->beginTransmission();
  
  Serial.print('a'); // I do not know why this works but it stops all my serial comms from breaking
  if(Serial.available()){
    digitalWrite(LED1,HIGH);
    byte cmd = Serial.read();
    if(cmd == 1 || cmd == '1'){
      // Send
      byte universe = Serial.read();
      int channel = Serial.read() | (((int)Serial.read()) << 8);
      byte value = Serial.read();
      if(universe == 1){
        dmx_1->write(channel,value);
      }else if(universe == 2){
        dmx_2->write(channel,value);
      }
    }
    Serial.print('a');
  }
  dmx_1->endTransmission();
  dmx_2->endTransmission();
  delay(100);
}
