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

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

// Uncomment the next line to enable debugging on the debug port
// #define DEBUG_PORT
#ifdef DEBUG_PORT
  #pragma message ( "Compiling with debug support" )
#endif

// Version information

#define PROTOCOL_REV 1
#define VERSION_MAJOR 1
#define VERSION_MINOR 0

#define CAP_DMX 1 << 0
#define CAP_RDM 1 << 1
#define CAP_LED 1 << 2
#define CAP_CHAIN 1 << 3
#define CAP_EEPROM 1 << 4
#define CAP_SELFTEST 1 << 5
#define CAP_UPDATE_MAN 1 << 6
#define CAP_UPDATE_AUTO 1 << 7
#define CAP_EXPANSION 1 << 8

#define LED_FIXED_COLOR 1 << 0
#define LED_POWER 1 << 1
#define LED_RGB 1 << 2
#define LED_SET 1 << 3
#define LED_GET 1 << 4
#define LED_FINE 1 << 5

#define EEPROM_READ 1 << 0
#define EEPROM_WRITE 1 << 1
#define EEPROM_CHANNEL 1 << 2 // Save channel information to trigger from EEPROM
#define EEPROM_CUE 1 << 3 // Save cues to EEPROM
#define EEPROM_OPERATION 1 << 4 // Capability for this device to operate with commands off EEPROM without a host

#define CMD_SEND 1
#define CMD_SN 2
#define CMD_RESET 3
#define CMD_IDENT 4
#define CMD_INIT 5
#define CMD_CAP 6
#define CMD_LEDINFO 7
#define CMD_LEDOP 8
#define CMD_EEPROMINFO 9
#define CMD_EEPROMOP 10
#define CMD_SELFTEST 11
#define CMD_UPDATE_MAN 12

#include "device.h"

void(* resetFunc) (void) = 0;

int universeSize;

RS485Class *rs485[UNIVERSE_COUNT];
DMXClass *dmx[UNIVERSE_COUNT];

int code = 0;

void setup() {
  EEPROM.begin();

  // Setup output ports
  for(int i = 0; i < UNIVERSE_COUNT; i++){
    rs485[i] = new RS485Class(serialPorts[i], outputPins[i*3], outputPins[i*3+1], outputPins[i*3+2]);
    dmx[i] = new DMXClass(*rs485[i]);
  }
  
  Serial.begin(115200);
  #if (CAP) & (CAP_EXPANSION) || DEBUG_PORT
    #pragma message ( "Compiling with DMX support" )
    // Start auxilliary serial port to receive commands from addons
    expansionSerial.begin(115200);
  #endif
  
  #ifdef DEBUG_PORT
    expansionSerial.println("Started controller!");
  #endif
}
int count = 0;
void loop() {
  //TODO: Implement code path for receiving commands from expansion serial
  if (Serial.available()) {
    int cmd = Serial.read();
    #ifdef DEBUG_PORT
      expansionSerial.print("Received command with code ");
      expansionSerial.print(cmd);
      expansionSerial.println();
    #endif
    if (CMD_SEND) {
      #if (CAP) & (CAP_DMX)
        #pragma message ( "Compiling with DMX support" )
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
              expansionSerial.println("Timed out waiting for send command!");
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
          expansionSerial.print("Received send command for universe ");
          expansionSerial.print(universe);
          expansionSerial.print(" channel ");
          expansionSerial.print(channel);
          expansionSerial.print(" value ");
          expansionSerial.print(value);
          expansionSerial.println();
        #endif
        if (universe <= UNIVERSE_COUNT){
          int index = universe-1;
          dmx[index]->beginTransmission();
          dmx[index]->write(channel,value);
          dmx[index]->endTransmission();
        }else{
          #ifdef DEBUG_PORT
            expansionSerial.print("Requested send to invalid universe");
            expansionSerial.print(universe);
            expansionSerial.println("!");
          #endif
        }
        Serial.write(CMD_SEND);
      #endif
    } else if (CMD_SN) {
      // SN command
      Serial.write(CMD_SN);
      for (uint8_t i = 0; i < 8; i++) {
        Serial.write((uint8_t) (SERIAL_NUMBER >> (i*8)));
      }
    } else if (CMD_RESET) {
      // Reset command
      Serial.write(CMD_RESET);
      Serial.flush();
      resetFunc();
    } else if (CMD_IDENT) {
      // Identify command
      Serial.write(CMD_IDENT);
      Serial.write(0xFF); // Magic number
      Serial.write(PROTOCOL_REV); // Protocol rev
      Serial.write(VERSION_MAJOR); // Software major version
      Serial.write(VERSION_MINOR); // Software minor version
      Serial.write(HARDWARE_VERSION); // Hardware revision
      Serial.write(UNIVERSE_COUNT); // 2 universes
    } else if (CMD_INIT) {
      #if (CAP) & (CAP_DMX)
        // Initialize command
        int count = 0;
        while (Serial.available() < 1) {
          delay(1);
          count++;
          if (count >= 500) {
            while (Serial.available())
              Serial.read();
            #ifdef DEBUG_PORT
              expansionSerial.println("Timed out waiting for initialize command!");
            #endif
            return;
          }
        }
        universeSize = Serial.read() | (((int) Serial.read()) << 8); // Universe size is sent as 2 bytes
        // This isn't in the setup function because the serial stuff isn't necessarily connected during the setup phase and errors need to be reported (so its not just broken mysteriously)
        Serial.write(CMD_INIT);
        // initialize the DMX library with the universe size
        code = 0;
        for(int i = 0; i < UNIVERSE_COUNT; i++){
          if(!dmx[i]->begin(universeSize)){
            code = i+1;
            break;
          }
        }
        #ifdef DEBUG_PORT
          expansionSerial.print("Received initialize command! Current code is ");
          expansionSerial.print(code);
          expansionSerial.println();
        #endif
        
        Serial.write(code);
      #endif
    } else if (CMD_CAP){
      // Capabilities command
      
      Serial.write(CMD_CAP); // Acknowledge command
      Serial.write((byte) CAP);
      Serial.write((byte) (CAP >> 8));
      Serial.write((byte) (CAP >> 16));
      Serial.write((byte) (CAP >> 24));
      
    } else if (CMD_LEDINFO){
      #if (CAP) & (CAP_LED)
        #pragma message ( "Compiling with LED support" )
        // LED info command
        Serial.write(CMD_LEDINFO); // Acknowledge command
        Serial.write(LED_COUNT);
        Serial.write(LED_USABLE);
        Serial.write(LED_CAP); // Supported operations
        for(int i = 0; i < LED_COUNT; i++){
          for(int i1 = 0; i1 < 6; i1++){
            Serial.write(leds[i*6+i1]);
          }
        }
      #endif
    } else if (CMD_LEDOP){
      #if (CAP) & (CAP_LED)
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
              expansionSerial.println("Timed out waiting for LED get/set command!");
            #endif
            return;
          }
        }
        Serial.write(CMD_LEDOP);
        int operation = Serial.read();
        int index = Serial.read();
        int red = Serial.read();
        int green = Serial.read();
        int blue = Serial.read();
        int mode = Serial.read();
        int pin = 0;
        // Index starts at 1 because 0 is the power LED which cannot be changed
        if(index <= LED_COUNT){
          pin = ledPins[index-1];
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
      #endif
    } else if (CMD_EEPROMINFO){
      #if (CAP) & (CAP_EEPROM)
        #pragma message ( "Compiling with EEPROM support" )
        // This controller advertizes EEPROM support to its host so support it
        // EEPROM info command
        // Note that the start of the EEPROM is always 0 even if it doesn't physically, this is because stuff may be stored in the EEPROM
        // For the controller to use so silently add an offset if needed and take it out of the size
        Serial.write(CMD_EEPROMINFO);
        
        Serial.write((byte) EEPROM_SIZE);
        Serial.write((byte) (EEPROM_SIZE >> 8));
        Serial.write((byte) (EEPROM_SIZE >> 16));
        Serial.write((byte) (EEPROM_SIZE >> 24));
        
        Serial.write(EEPROM_READ | EEPROM_WRITE);
      #endif
    } else if (CMD_EEPROMOP){
      #if (CAP) & (CAP_EEPROM)
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
              expansionSerial.println("Timed out waiting for EEPROM read/write command!");
            #endif
            return;
          }
        }
        Serial.write(CMD_EEPROMOP);
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
      #endif
    } else if (CMD_SELFTEST){
      #if (CAP) & (CAP_SELFTEST)
        #pragma message ( "Compiling with SELFTEST support" )
        Serial.write(CMD_SELFTEST);
        dmx[0]->end();
        dmx[1]->end();
        rs485[0]->begin(250000); // Use same baud rate as DMX to make it more realistic
        rs485[1]->begin(250000);
        
        rs485[0]->receive();
        int magic = 0xBEEF;
        rs485[0]->beginTransmission();
        rs485[0]->write((byte) magic);
        rs485[0]->write((byte) (magic >> 8));
        rs485[0]->endTransmission();
        int count = 0;
        while(!rs485[1]->available() < 2){
          count++;
          delay(1);
          if(count > 500){
            while (Serial.available())
              Serial.read();
            #ifdef DEBUG_PORT
              expansionSerial.println("Timed out waiting for EEPROM read/write command!");
            #endif
          }
        }
        int newMagic = rs485[1]->read() | (((int) rs485[1]->read()) << 8);
        if(newMagic != magic){
          #ifdef DEBUG_PORT
            expansionSerial.print("Incorrect magic number for self test! Expected ");
            expansionSerial.print(magic);
            expansionSerial.print(" but got ");
            expansionSerial.print(newMagic);
            expansionSerial.println();
          #endif
          rs485[0]->noReceive();
          rs485[0]->end();
          rs485[1]->end();
          Serial.write(1); // Failed
        }else{
          rs485[0]->noReceive();
          rs485[0]->end();
          rs485[0]->end();
          Serial.write(0); // Passed
        }
      #endif
    }else if(cmd == CMD_UPDATE_MAN){
      #if (CAP) & (CAP_UPDATE_MAN)
        #pragma message ( "Compiling with UPDATE_MAN support" )
        Serial.write(CMD_UPDATE_MAN);
        int len = strlen(UPDATE_URL);
        Serial.write(len);
        Serial.print(UPDATE_URL);
      #endif
    }
  }
  count++;
  if (count >= 100) {
    count = 0;
    for(int i = 0; i < UNIVERSE_COUNT; i++){
      dmx[i]->beginTransmission();
      dmx[i]->endTransmission();
    }
  }
  delay(1);
}
