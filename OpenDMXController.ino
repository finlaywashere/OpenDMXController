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

#define LED_1 4
#define LED_2 39
#define LED_3 40

#define EXP_1 9
#define EXP_2 8
#define EXP_3 7
#define EXP_4 6
#define EXP_5 15 // RX3
#define EXP_6 14 // TX3

#define CAP_DMX 1 << 0
#define CAP_RDM 1 << 1
#define CAP_LED 1 << 2
#define CAP_CHAIN 1 << 3
#define CAP_EEPROM 1 << 4
#define CAP_SELFTEST 1 << 5

#define LED_FIXED_COLOR 1 << 0
#define LED_POWER 1 << 1
#define LED_RGB 1 << 2
#define LED_SET 1 << 3
#define LED_GET 1 << 4
#define LED_FINE 1 << 5

#define EEPROM_START 0
#define EEPROM_SIZE 4096 - EEPROM_START

#define EEPROM_READ 1 << 0
#define EEPROM_WRITE 1 << 1
#define EEPROM_CHANNEL 1 << 2 // Save channel information to trigger from EEPROM
#define EEPROM_CUE 1 << 3 // Save cues to EEPROM
#define EEPROM_OPERATION 1 << 4 // Capability for this device to operate with commands off EEPROM without a host

void(* resetFunc) (void) = 0;

int universeSize;

RS485Class *rs485_1;
RS485Class *rs485_2;
DMXClass *dmx_1;
DMXClass *dmx_2;

int code = 0;

void setup() {
  EEPROM.begin();

  // Setup output ports
  rs485_1 = new RS485Class(Serial1, 18, A5, A5);
  rs485_2 = new RS485Class(Serial2, 16, A7, A7);
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
      // Write command
      // Wait for the data
      int count = 0;
      while (Serial.available() < 5) {
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
      int mode = Serial.read();
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
      resetFunc();
    } else if (cmd == 4) {
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
      int count = 0;
      while (Serial.available() < 1) {
        delay(1);
        count++;
        if (count >= 500) {
          while (Serial.available())
            Serial.read();
          #ifdef DEBUG_PORT
            Serial3.println("Timed out waiting for initialize command!");
          #endif
          return;
        }
      }
      universeSize = Serial.read() | (((int) Serial.read()) << 8); // Universe size is sent as 2 bytes
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
      #ifdef DEBUG_PORT
        Serial3.print("Received initialize command! Current code is ");
        Serial3.print(code);
        Serial3.println();
      #endif
      
      Serial.write(code); // This is currently the only self testing supported (maybe more will be added in future revisions
    } else if (cmd == 6){
      // Capabilities command
      int cap = CAP_DMX | CAP_LED | CAP_EEPROM | CAP_SELFTEST; // Current capabilities
      Serial.write(6); // Acknowledge command
      Serial.write((byte) cap);
      Serial.write((byte) (cap >> 8));
      Serial.write((byte) (cap >> 16));
      Serial.write((byte) (cap >> 24));
      
    } else if (cmd == 7){
      // LED info command
      Serial.write(7); // Acknowledge command
      Serial.write(4); // 4 LEDs
      Serial.write(3); // 3 Usable
      Serial.write(LED_SET | LED_GET); // Supported operations
      // Power LED
      Serial.write(LED_FIXED_COLOR | LED_POWER); // Indicate that it is a power LED
      Serial.write(0);
      Serial.write(255); // It is green
      Serial.write(0);
      Serial.write(0); // X = 0
      Serial.write(0); // Y = 0
      // LED 3
      Serial.write(LED_FIXED_COLOR);
      Serial.write(index0);
      Serial.write(255); // It is green
      Serial.write(0);
      Serial.write(0); // X = 0
      Serial.write(1); // Y = 1
      // LED 2
      Serial.write(LED_FIXED_COLOR);
      Serial.write(255); // It is red
      Serial.write(0);
      Serial.write(0);
      Serial.write(0); // X = 0
      Serial.write(1); // Y = 2
      // LED 2
      Serial.write(LED_FIXED_COLOR);
      Serial.write(255); // It is red
      Serial.write(0);
      Serial.write(0);
      Serial.write(0); // X = 0
      Serial.write(1); // Y = 3
    } else if (cmd == 8){
      // LED get/set command
      // Wait for the data
      int count = 0;
      while (Serial.available() < 6) {
        delay(1);
        count++;
        if (count >= 500) {
          while (Serial.available())
            Serial.read();
          #ifdef DEBUG_PORT
            Serial3.println("Timed out waiting for LED get/set command!");
          #endif
          return;
        }
      }
      Serial.write(8);
      int operation = Serial.read();
      int index = Serial.read();
      int red = Serial.read();
      int green = Serial.read();
      int blue = Serial.read();
      int mode = Serial.read();
      int pin = 0;
      // Index starts at 1 because 0 is the power LED which cannot be changed
      if(index == 1){
        pin = LED_3;
      }else if(index == 2){
        pin = LED_2;
      }else if(index == 3){
        pin = LED_1;
      }
      if(pin == 0){
        Serial.write(0xFF);
      }else{
        if(operation == LED_GET){
          // We don't support fine grained LED control so just digitalRead it
          // On non RGB LEDs the red value is the value of the LED
          Serial.write(LED_GET); // Ack
          Serial.write(digitalRead(pin) * 255);
          Serial.write(0);
          Serial.write(0);
        }else if(operation == LED_SET){
          int value = LOW;
          if(red > 0)
            value = HIGH;
          digitalWrite(pin,value);
          Serial.write(LED_SET); // Ack
        }else{
          Serial.write(0xFF);
        }
      }
    } else if (cmd == 9){
      // This controller advertizes EEPROM support to its host so support it
      // EEPROM info command
      // Note that the start of the EEPROM is always 0 even if it doesn't physically, this is because stuff may be stored in the EEPROM
      // For the controller to use so silently add an offset if needed and take it out of the size
      Serial.write(9);

      Serial.write((byte) EEPROM_SIZE);
      Serial.write((byte) (EEPROM_SIZE >> 8));
      Serial.write((byte) (EEPROM_SIZE >> 16));
      Serial.write((byte) (EEPROM_SIZE >> 24));
      
      Serial.write(EEPROM_READ | EEPROM_WRITE);
    } else if (cmd == 10){
      // EEPROM read/write command
      // Wait for the data
      int count = 0;
      while (Serial.available() < 6) {
        delay(1);
        count++;
        if (count >= 500) {
          while (Serial.available())
            Serial.read();
          #ifdef DEBUG_PORT
            Serial3.println("Timed out waiting for EEPROM read/write command!");
          #endif
          return;
        }
      }
      Serial.write(10);
      int operation = Serial.read();
      int address = Serial.read() | (((int) Serial.read()) << 8) | (((int) Serial.read()) << 16) | (((int) Serial.read()) << 24);
      if(address >= EEPROM_SIZE){
        Serial.write(0xFF);
      }else{
        address += EEPROM_START;
        if(operation == EEPROM_READ){
          int value = EEPROM.read(address);
          Serial.write(EEPROM_READ);
          Serial.write(value);
        }else if(operation == EEPROM_WRITE){
          int value = Serial.read();
          EEPROM.write(address, value);
          Serial.write(EEPROM_WRITE);
        }else{
          Serial.write(0xFF);
        }
      }
    } else if (cmd == 11){
      Serial.write(11);
      dmx_1->end();
      dmx_2->end();
      rs485_1->begin(250000); // Use same baud rate as DMX to make it more realistic
      rs485_2->begin(250000);

      rs485_1->receive();
      int magic = 0xBEEF;
      rs485_1->beginTransmission();
      rs485_1->write((byte) magic);
      rs485_1->write((byte) (magic >> 8));
      rs485_1->endTransmission();
      int count = 0;
      while(!rs485_2->available() < 2){
        count++;
        delay(1);
        if(count > 500){
          while (Serial.available())
            Serial.read();
          #ifdef DEBUG_PORT
            Serial3.println("Timed out waiting for EEPROM read/write command!");
          #endif
        }
      }
      int newMagic = rs485_2->read() | (((int) rs485_2->read()) << 8);
      if(newMagic != magic){
        #ifdef DEBUG_PORT
          Serial3.print("Incorrect magic number for self test! Expected ");
          Serial3.print(magic);
          Serial3.print(" but got ");
          Serial3.print(newMagic);
          Serial3.println();
        #endif
        rs485_1->noReceive();
        rs485_1->end();
        rs485_2->end();
        Serial.write(1); // Failed
      }else{
        rs485_1->noReceive();
        rs485_1->end();
        rs485_2->end();
        Serial.write(0); // Passed
      }
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
