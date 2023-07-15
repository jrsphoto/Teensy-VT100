Fast Mini Serial Terminal

Project September 2016 Peter Scargill - developed from VT-100 non-Arduino code by Martin K. Schröder

Looking for a fast serial terminal on a cheap display powered by Atmega, I discovered Martin's VT100 Atmega328 code and brought it into the Arduino environment, turning it into a simple 40 char by 40 line terminal for debugging my Arduino and ESP8266 projects. Right now I've changed the PORT bits to handle a 1284 chip so it'll need change for a 328 - this needs generalising to be of greater use.

Missing from the code was general line drawing as this was intended as a stand alone serial terminal.  I could not get the serial to operate properly so I've replaced it with the Arduino serial - but you need to read the blog as the Arduino serial buffer is 64 bytes and that's useless really... I've increased mine all the way up to 1K RAM.

So right now I've added in fast VERTICAL line drawing and from there, the ADAFRUIT line drawing routine with only one minor change as their swap routine caused trouble. Now the line drawing works a treat.

Up to now just a few changes to VT100 - commands s and u save the cursor. Commands 7 and 8 in addition should save "attributes" -  I'm taking that to assume foreground and background colour and I've updated that - so now you can use 7 and 8 - go off and write something elsewhere and return to where you were in the colour your were in. Also added a new ESC [ X command to set baud rate which is stored in EEPROM. Speed and character count are updated on the bottom line during idle.

See http://tech.scargill.net/an-arduino-terminal/ for more info.