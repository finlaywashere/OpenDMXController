# Configuring an ATmega2560

## Setting clock speed

A brand new ATmega2560 must have its fuses set for proper operation

The command to do so is (using an ArduinoISP on /dev/ttyUSB0) `avrdude -c stk500v1 -p ATmega2560 -P /dev/ttyUSB0 -b 19200 -U efuse:w:0xff:m -U hfuse:w:0xd9:m -U lfuse:w:0xf7:m`
