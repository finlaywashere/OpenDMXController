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
#include <EEPROM.h>

// Uncomment the next line to enable debugging on the debug port
#define DEBUG_PORT

#define SERIAL_NUMBER 0x0105000000000001

void(* resetFunc) (void) = 0;

const int universeSize = 512;

DMXClass *dmx_1;
DMXClass *dmx_2;

int code = 0;

void setup() {
  EEPROM.begin();

  // Setup output ports
  RS485Class *rs485_1 = new RS485Class(Serial1, 18, A5, A5);
  RS485Class *rs485_2 = new RS485Class(Serial2, 16, A7, A7);
  dmx_1 = new DMXClass(*rs485_1);
  dmx_2 = new DMXClass(*rs485_2);
  
  Serial.begin(115200);
  // Start auxilliary serial port to receive commands from addons
  Serial3.begin(115200);

  #ifdef DEBUG_PORT
    Serial3.println("Started controller!");
  #endif
}
int count = 0;
void loop() {
  //TODO: Implement code path for receiving commands from Serial3
  if (Serial.available()) {
    digitalWrite(4, HIGH);
    int cmd = Serial.read();
    #ifdef DEBUG_PORT
      Serial3.print("Received command with code ");
      Serial3.print(cmd);
      Serial3.println();
    #endif
    if (cmd == 1) {
      digitalWrite(39, HIGH);
      // Write command
      // Wait for the data
      int count = 0;
      while (Serial.available() < 4) {
        delay(1);
        count++;
        if (count >= 500) {
          while (Serial.available())
            Serial.read();
          #ifdef DEBUG_PORT
            Serial3.println("Timed out waiting for send command!");
          #endif
          return;
        }
      }
      // Send
      int universe = Serial.read();
      int channel = Serial.read() | (((int)Serial.read()) << 8);
      int value = Serial.read();
      #ifdef DEBUG_PORT
        Serial3.print("Received send command for universe ");
        Serial3.print(universe);
        Serial3.print(" channel ");
        Serial3.print(channel);
        Serial3.print(" value ");
        Serial3.print(value);
        Serial3.println();
      #endif
      if (universe == 1) {
        dmx_1->beginTransmission();
        dmx_1->write(channel, value);
        dmx_1->endTransmission();
      } else if (universe == 2) {
        dmx_2->beginTransmission();
        dmx_2->write(channel, value);
        dmx_2->endTransmission();
      }
      Serial.write(1);
    } else if (cmd == 2) {
      // SN command
      Serial.write(2);
      for (uint8_t i = 0; i < 8; i++) {
        Serial.write((uint8_t) (SERIAL_NUMBER >> (i*8)));
      }
    } else if (cmd == 3) {
      // Reset command
      Serial.write(3);
      Serial.flush();
      //resetFunc();
    } else if (cmd == 4) {
      digitalWrite(40, HIGH);
      // Identify command
      Serial.write(4);
      Serial.write(0xFF); // Magic number
      Serial.write(1); // Protocol rev
      Serial.write(0); // Software major version
      Serial.write(9); // Software minor version
      Serial.write(4); // Hardware revision
      Serial.write(2); // 2 universes
    } else if (cmd == 5) {
      // Initialize command
      // This isn't in the setup function because the serial stuff isn't necessarily connected during the setup phase and errors need to be reported (so its not just broken mysteriously)
      Serial.write(5);
      // initialize the DMX library with the universe size
      code = 0;
      if (!dmx_1->begin(universeSize)) {
        code = 1;
      }
      if (!dmx_2->begin(universeSize)) {
        code = 2;
      }
      
      Serial3.print("Received initialize command! Current code is ");
      Serial3.print(code);
      Serial3.println();
      
      Serial.write(code); // This is currently the only self testing supported (maybe more will be added in future revisions
    }
  }
  count++;
  if (count >= 100) {
    count = 0;
    dmx_1->beginTransmission();
    dmx_2->beginTransmission();
    dmx_1->endTransmission();
    dmx_2->endTransmission();
  }
  delay(1);
}
