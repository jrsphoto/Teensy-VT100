
// A FAST subset-VT100 serial terminal with 40 lines by 40 chars

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <EEPROM.h>

/**  
  VT-100 code Copyright: Martin K. Schröder (info@fortmax.se) 2014/10/27
  Conversion to Arduino, additional codes and this page by Peter Scargill 2016
*/

#include "ili9340.h"
#include "vt100.h"

extern char new_br[8]; // baud-rate string - if non-zero will update screen
uint32_t charCounter=0;
uint32_t charShadow=0;
uint8_t  charStart=1;

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);  
  ili9340_init();
  ili9340_setRotation(0);  
}

#define PURPLE_ON_BLACK "\e[35;40m"
#define GREEN_ON_BLACK "\e[32;40m"

void loop() {
  auto respond = [=](char *str){ Serial.print(str); }; 
  vt100_init(respond);
  sei();
 
  // reset terminal and clear screen..
  vt100_puts("\e[c");   // terminal ok
  vt100_puts("\e[2J");  // erase entire screen
  vt100_puts("\e[?6l"); // absolute origin
  // print some fixed purple text top and bottom
  vt100_puts(PURPLE_ON_BLACK);   
  vt100_puts("\e[2;1HSerial HC2016 Terminal 1.0"); 
  vt100_puts("\e[39;1HBaud: 115200");
  vt100_puts("\e[39;15HChars:");
  // 4 LEDS in the top corner initially set to OFF
  ili9340_drawRect(186,6,10,10,ILI9340_RED,ILI9340_BLACK);
  ili9340_drawRect(200,6,10,10,ILI9340_RED,ILI9340_BLACK);
  ili9340_drawRect(214,6,10,10,ILI9340_RED,ILI9340_BLACK);
  ili9340_drawRect(228,6,10,10,ILI9340_RED,ILI9340_BLACK);
  // delimit fixed areas
  ili9340_drawFastHLine(0,20, 240, ILI9340_BLUE);
  ili9340_drawFastHLine(0,300, 240, ILI9340_RED);
  vt100_puts("\e[4;38r"); // set the scrolling region
  vt100_puts(GREEN_ON_BLACK);
  vt100_puts("\e[37;1H"); // Set up at line 37, char position 1
  vt100_puts("\e[0q"); // All top corner LEDs off

  if ((EEPROM.read(BAUD_STORE)^EEPROM.read(BAUD_STORE+1))==0xff)
  {
    char bStr[12];
    sprintf(bStr,"\e[%dX",EEPROM.read(BAUD_STORE));
    vt100_puts(bStr);
  }
  else vt100_puts("\e[6X"); // baud rate 6 - i.e. 115200
 
  while(1){
    char data;
    data=Serial.read();
    if(data == -1) 
      {
      data=Serial1.read();
      if(data == -1) 
          {   
          //if nothing coming in serial - check for baud rate message
          if (new_br[0]) 
              { 
                vt100_puts("\e7\e[39;7H"); // save cursor and attribs
                vt100_puts(PURPLE_ON_BLACK);
                vt100_puts(new_br); 
                vt100_puts("\e8"); // restore cursor and attribs
                new_br[0]=0; 
               } 
          //or character count update
          if ((charCounter!=charShadow) || (charStart))
            {
              charShadow=charCounter; charStart=0;
              vt100_puts("\e7");  //save cursor and color
              vt100_puts(PURPLE_ON_BLACK);
              char numbers[12]; sprintf(numbers,"%ld",charCounter); 
              vt100_puts("\e[39;22H"); vt100_puts(numbers);          
              vt100_puts("\e8");  //restore cursor and attribs                
            }
            continue;
          }
      }
    charCounter++;
    vt100_putc(data);  
  }
}

