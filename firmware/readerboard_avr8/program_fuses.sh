#!/bin/bash

avrdude -P usb -c avrispmkii -p at90usb162 -B 10 -U efuse:w:0xff:m -U hfuse:w:0xd8:m -U lfuse:w:0xde:m

