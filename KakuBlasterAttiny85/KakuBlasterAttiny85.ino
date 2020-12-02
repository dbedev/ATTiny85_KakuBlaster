/* KakuBlasterAttiny85.ino
    Copyright (C) 2020 - D. Berendse

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * Program to use an Digispark Attiny85+433.92TX module as a
 * PC/SBC usb cdc acm controlled Kaku command sender.
 *
 * Expected command string (including space at the end!)
 * "Kaku <AdressDecimal> <portDecimal> <valDecimal> "
 * --Port 0..15 are on/Off ports  (val 0 is off)
 * --Port 16 is the group         (val 0 is off)
 * --Port 17..32 are dimmed items  (val 0..15 for dim setting)
 *
 *  Based on libraries
 *  - NewRemoteTransmitter  (NewRemoteSwitch library 1.0.0) https://github.com/koffienl/attiny-kaku/tree/master/libraries/NewRemoteSwitch
 *  - DigiCDC  (Spark DigiCDC library, https://github.com/digistump/DigistumpArduino/tree/master/digistump-avr/libraries/DigisparkCDC)
 */

// Kaku 433.92 Tx lib
#include <NewRemoteTransmitter.h>
// usb cdc serial for digispark
#include <DigiCDC.h>
// Note that the CDC device seems to get stuck and halt the linux ttyACM driver if the
// host closes the port too soon,so use a >100ms sleep at the host before closing the port on linux systems!

// Pin mapping:
#define LED_PIN (1)
#define OTH_PIN (0)

// By default the LED pin is used for the 433 mhz pwr, so the led is on
// when the tx module is powered.
// In case one wants the led to flicker like a remote does, just
// swap the pin ID here and reconnect the wires accordingly.
#define ON_OFF_PIN (LED_PIN)
#define RF_TX_PIN (OTH_PIN)
/*
                    ATtiny 85
                      _____
  RESET      A0, D5 -|  A  |- VCC
  USB        A3, D3 -|  T  |- A1, D2  RSCK
  USB        A2, D4 -|  t  |- D1      MISO    LED / 433.92 TX module PWR
                GND -|_____|- D0      MOSI    433.92 RF_TX
*/

static uint8_t mRfTimer;
// globals for uart receive
#define BUFFSIZE 32
static char m_buff[BUFFSIZE];
static uint8_t mbufcnt =0;

static void resetCmdBuffer(void){
  uint8_t cnt =0;
  mbufcnt = 0;
  for(cnt = 0; cnt < BUFFSIZE; cnt++){
     m_buff[cnt] = 0;
  }
}

static void setup() {
    pinMode(LED_PIN, OUTPUT);      // LED on Model A to indicate boot
    digitalWrite(LED_PIN, HIGH);
    pinMode(OTH_PIN, OUTPUT);      // LED on Model A to indicate boot
    digitalWrite(OTH_PIN, LOW);
    SerialUSB.begin();
    resetCmdBuffer();
    digitalWrite(LED_PIN, LOW);
    mRfTimer = 2;
}

static void transmitData(){
  char * cmd = m_buff;
  uint8_t i=5;
  unsigned long adress =0;
  uint8_t port = 255;
  uint8_t val =0;
  uint8_t pos = 5;
  // parse the string
  // as the expected message is "Kaku xxxxxxxx xx yy "

  // next space
  while(i<BUFFSIZE-2 && cmd[i]!= ' '){
    i++;
  }
  cmd[i] = 0; // terminate string
  adress = atol(&cmd[pos]); // read the long kaku adress
  pos =i+1; // start of next param
  // next space
  while(  i<BUFFSIZE-2 && cmd[i]!= ' '){
    i++;
  }
  cmd[i] = 0; // terminate string
  port = atol(&cmd[pos]); // use atol as we already need it for the adress cmd
  pos =i+1; // start of next param
  // next space
  while(i<BUFFSIZE-2 && cmd[i]!= ' '){
    i++;
  }
  cmd[i] = 0; // terminate string
  val = atol(&cmd[pos]); // use atol as we already have to use it for adress cmd

  // done parsing the string
  digitalWrite(ON_OFF_PIN, HIGH); // switch on the tx module
  mRfTimer = 150;  // set Rf timer to 150
  // check to see if these are valid values
  if(port < 33 && val < 16){
     // Create a transmitter on for adress, using digital pin to transmit,
     // with a period duration of 260ms (default), repeating the transmitted
     // code 2^3=8 times.
     NewRemoteTransmitter transmitter(adress, RF_TX_PIN, 260, 3);

     if(port < 16){ // unit on/off
         bool onOff = false;
         if(val > 0 ){
          onOff = true;
         }
         transmitter.sendUnit(port, onOff);
      } else if (port == 16) { // group on/off
         bool onOff = false;
         if(val > 0 ){
          onOff = true;
         }
         transmitter.sendGroup(onOff);
      } else if (port > 16){ // dim settings
         transmitter.sendDim(port-17, val &0xf );
      }
  }
}

void loop(){
   uint8_t rcv= SerialUSB.available();

   if (rcv){
        uint8_t cnt;
        // read the data from the ttyAcm Device
        for(cnt = 0; cnt < rcv; cnt++){
           char rc = SerialUSB.read();
           if(rc == 'K'){
             resetCmdBuffer();
             m_buff[0] = 'K';
           } else {
             m_buff[mbufcnt] = rc;
           }
		   // just echo back
           SerialUSB.write(rc);
           mbufcnt = (mbufcnt+1) % (BUFFSIZE-1);
         }
         // look for header in the message
         // as the expected message is "Kaku xxxxxxxx xx yy "
         if(m_buff[0] == 'K' && m_buff[1] == 'a'  &&m_buff[2] == 'k' && m_buff[3] == 'u'){
           uint8_t spacecount = 0;
           // count spaces from pos 4 in the buffer
           for(cnt = 4; cnt < BUFFSIZE; cnt++){
                 if(m_buff[cnt] == ' ') {
                     spacecount++;
                 }
           }
           if( spacecount > 3){ // 3 spaces + Kaku so most likely our message
                 transmitData();
                 resetCmdBuffer();
           }
         }

   }
   
   SerialUSB.refresh();

   if(mRfTimer > 0){ // turn off tx once the time counter expires
      SerialUSB.delay(30);
      mRfTimer--;
      if(mRfTimer < 2){
         mRfTimer=0;
         digitalWrite(ON_OFF_PIN, LOW);
      }
   }
}
