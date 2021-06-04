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
#include <HardwareSerial.h>
#include <avr/boot.h>

//void(* resetFunc) (void) = 0;

const int universeSize = 512;

DMXClass *dmx_1;
DMXClass *dmx_2;

void setup() {
  // Setup output ports
  RS485Class *rs485_1 = new RS485Class(Serial1,18,A5,A5);
  RS485Class *rs485_2 = new RS485Class(Serial2,16,A7,A7);
  dmx_1 = new DMXClass(*rs485_1);
  dmx_2 = new DMXClass(*rs485_2);
  
  Serial.begin(115200);
  // Start auxilliary serial port to receive commands from addons
  Serial3.begin(115200);
}
int count = 0;
void loop() {
  //TODO: Implement code path for receiving commands from Serial3
  if(Serial.available()){
    byte cmd = Serial.read();
    if(cmd == 1){
      // Write command
      // Wait for the data
      int count = 0;
      while(Serial.available() < 4){
        delay(1);
        count++;
        if(count >= 50){
          for(int i = 0; i < 4; i++)
            Serial.read();
          return;
        }
      }
      // Send
      int universe = Serial.read();
      int channel = Serial.read() | (((int)Serial.read()) << 8);
      int value = Serial.read();
      if(universe == 1){
        dmx_1->beginTransmission();
        dmx_1->write(channel,value);
        dmx_1->endTransmission();
      }else if(universe == 2){
        dmx_2->beginTransmission();
        dmx_2->write(channel,value);
        dmx_2->endTransmission();
      }
      Serial.write(1);
    }else if(cmd == 2){
      // SN command
      Serial.write(2);
      for (uint8_t i = 14; i < 24; i += 1) {
        Serial.print(boot_signature_byte_get(i), HEX);
      }
    }else if(cmd == 3){
      // Reset command
      Serial.write(3);
      Serial.flush();
      //resetFunc();
    }else if(cmd == 4){
      // Identify command
      Serial.write(4);
      Serial.write(0xFF); // Magic number
      Serial.write(1); // Protocol rev
      Serial.write(0); // Software major version
      Serial.write(9); // Software minor version
      Serial.write(4); // Hardware revision
      Serial.write(2); // 2 universes
    }else if(cmd == 5){
      // Initialize command
      // This isn't in the setup function because the serial stuff isn't necessarily connected during the setup phase and errors need to be reported (so its not just broken mysteriously)
      Serial.write(5);
      // initialize the DMX library with the universe size
      int code = 0;
      if (!dmx_1->begin(universeSize)) {
        code = 253;
      }
      if (!dmx_2->begin(universeSize)) {
        code = 254;
      }
      Serial.write(code); // This is currently the only self testing supported (maybe more will be added in future revisions
    }
  }
  count++;
  if(count >= 100){
    count = 0;
    dmx_1->beginTransmission();
    dmx_2->beginTransmission();
    dmx_1->endTransmission();
    dmx_2->endTransmission();
  }
  delay(1);
}
