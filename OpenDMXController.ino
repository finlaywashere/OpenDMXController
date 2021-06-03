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

#define MAGIC_HIGH 0xBE
#define MAGIC_LOW 0xEF
#define PROTOCOL 2
#define SOFT_REV_MAJOR 0
#define SOFT_REV_MINOR 9
#define HARD_REV 4


void(* resetFunc) (void) = 0;

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
  if(Serial.available() || Serial3.available()){
    HardwareSerial *serial;
    if(Serial.available())
      serial = &Serial;
    else
      serial = &Serial3;
    
    byte cmd = serial->read();
    if(cmd == 1){
      // Write command
      // Wait for the data
      while(serial->available() < 4){}
      // Send
      int universe = serial->read();
      int channel = serial->read() | (((int)serial->read()) << 8);
      int value = serial->read();
      if(universe == 1){
        dmx_1->beginTransmission();
        dmx_1->write(channel,value);
        dmx_1->endTransmission();
      }else if(universe == 2){
        dmx_2->beginTransmission();
        dmx_2->write(channel,value);
        dmx_2->endTransmission();
      }
      serial->write(1);
    }else if(cmd == 2){
      // SN command
      serial->write(2);
      for (uint8_t i = 14; i < 24; i += 1) {
        serial->print(boot_signature_byte_get(i), HEX);
      }
    }else if(cmd == 3){
      // Reset command
      serial->write(3);
      resetFunc();
    }else if(cmd == 4){
      // Identify command
      serial->write(4);
      serial->write(MAGIC_HIGH);
      serial->write(MAGIC_LOW);
      serial->write(PROTOCOL);
      serial->write(SOFT_REV_MAJOR);
      serial->write(SOFT_REV_MINOR);
      serial->write(HARD_REV);
    }else if(cmd == 5){
      // Initialize command
      // This isn't in the setup function because the serial stuff isn't necessarily connected during the setup phase and errors need to be reported (so its not just broken mysteriously)
      serial->write(5);
      // initialize the DMX library with the universe size
      int code = 0;
      if (!dmx_1->begin(universeSize)) {
        code = 253;
      }
      if (!dmx_2->begin(universeSize)) {
        code = 254;
      }
      serial->write(code); // This is currently the only self testing supported (maybe more will be added in future revisions
    }
  }
  count++;
  if(count >= 50){
    count = 0;
    dmx_1->beginTransmission();
    dmx_2->beginTransmission();
    dmx_1->endTransmission();
    dmx_2->endTransmission();
  }
  delay(1);
}
