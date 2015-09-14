/**
 * BAC_Display
 * Author: Shawn Hymel
 * Date: September 1, 2015
 * 
 * Receives serial data from Particle Photon. Displays BAC.
 *
 * Format: 0.000
 * 4 bytes: SOF | 2 byte payload | EOF
 *         0xA5 |      xxx       | 0x5A
 * 
 * There's really only one appropriate license for this code:
 * https://en.wikipedia.org/wiki/Beerware
 */

#include <MicroView.h>

// Constants
#define PAYLOAD_SIZE      4     // Bytes
#define RX_TIMEOUT        200  // ms
#define SOF               0xA5
#define EOF               0x5A

// Global variables
byte in_msg[PAYLOAD_SIZE];
unsigned long timestamp;
bool has_msg;
bool do_recv;
uint8_t idx;
uint16_t bac;

void setup() {
  
  // Initialize MicroView
  uView.begin();
  uView.clear(ALL);
  uView.display();
  delay(500);
  uView.clear(PAGE);
  
  // Set up UART
  Serial.begin(9600);
  
  // Display default (0.000) BAC
  displayBAC(0);

}

void loop() {
  
  // Clear buffer
  memset(in_msg, 0, PAYLOAD_SIZE);
  
  // Receive BAC
  timestamp = millis();
  has_msg = false;
  do_recv = true;
  idx = 0;
  while ( do_recv ) {
    
    // Check if we have anything in our buffer
    if ( Serial.available() > 0 ) {
      in_msg[idx] = Serial.read();
      idx++;
      timestamp = millis();
    }
    
    // See if we have filled our message buffer
    if ( idx >= PAYLOAD_SIZE ) {
      do_recv = false;
      has_msg = true;
    }
    
    // If we timeout, exit receiver loop
    if ( (millis() - timestamp) >= RX_TIMEOUT ) {
      do_recv = false;
    }
  }
  
  // If we have a message, parse it and display BAC
  if ( has_msg ) {
    if ( (in_msg[0] == SOF) && 
         (in_msg[PAYLOAD_SIZE - 1] == EOF) ) {
      bac = in_msg[1] << 8;
      bac |= in_msg[2];
      displayBAC(bac);
    }
  }
}

void displayBAC(uint16_t num) {

  // Clear OLED
  uView.clear(PAGE);
  uView.setFontType(3);
  uView.setCursor(0, 0);
  
  // If we are over max display, just show 9.999
  if ( num > 999 ) {
    uView.print("0.999");
    uView.display();
    return;
  }
  
  // Set initial "0."
  uView.print("0");
  uView.print(" ");
  uView.circleFill(18, 45, 2);
  
  // Set unused digits to '0'
  if ( num < 100 ) {
    uView.print("0");
  }
  if ( num < 10 ) {
    uView.print("0");
  }
  
  // Print integer weight
  uView.print(num);
  
  uView.display();
}
