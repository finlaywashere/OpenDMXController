# Configuring an ATmega2560

## Setting clock speed

A brand new ATmega2560 must have its fuses set for proper operation

The command to do so is `avrdude -c stk500v1 -p ATmega2560 -P /dev/ttyUSB0 -b 19200 -U efuse:w:0xff:m -U hfuse:w:0x99:m -U lfuse:w:0xff:m`
