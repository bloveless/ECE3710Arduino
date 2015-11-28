#include <Adafruit_NeoPixel.h>
#include "TypesAndDefines.h"


//NRF      ARDUINO
//1 GND     GND
//2 VCC     3.3V
//3 CE      4  out
//4 CSN     5  out
//5 SCK     13 out
//6 MOSI    11 out
//7 MISO    12 in
//8 IRQ     2  in
// Relay 1  A5 out
// Relay 2  A0 out
// Lights   A1 out


// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1
#define PIN            A1

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      24

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

functionData expectedInstruction;
bool isMaster = false, waitingSuccessfulReturn = false;
// Allow the transmit to fail 10 times before skipping that packet
int allowedFails = 10;
// Track the current number of fails
int currentFails = 0;

void setup()
{
  //set up the SPI and define pins
  nrfInit();

  if(!isMaster) {
    pinMode(A0, OUTPUT);
    pinMode(A1, OUTPUT);
    pinMode(A5, OUTPUT);

    pixels.begin();
  }
}

String inString = "";
void loop()
{
  if(isMaster) {

    if(waitingSuccessfulReturn && nrfHasData()) {

      functionData lastReadInstruction = nrfGetLastReadInstruction();

      if(lastReadInstruction.data1 == (expectedInstruction.data1 + 1)) {
        Serial.println("Ping Successful");      
        Serial.print("Received: ");
        Serial.println((uint8_t) lastReadInstruction.data1);
        waitingSuccessfulReturn = false;
        currentFails = 0;
      } else {
        currentFails++;
        // Give it a millisecond
        delay(1);
      }

      // If we failed too many times then just skip this
      // transmission. Something much better than this
      // should happen here (error handling)
      if(currentFails >= allowedFails) {
        Serial.println("Ping Failed");
        waitingSuccessfulReturn = false;
        currentFails = 0;
      }
      
    } else if(!waitingSuccessfulReturn) {

      if(Serial.available() > 0) {
        char inChar = Serial.read();
        if(isDigit(inChar)) {
          inString += (char) inChar;
        }
  
        if((inChar == '\n') && (inString != "")) {
          //get a random byte
          int ping = inString.toInt();
          Serial.print("Pinging with     ");
          Serial.println(ping);
        
          // send the ping out starting with 6 (basically a function id)
          functionData instruction = {
            .function = PING,
            .data1 = ping
          };
      
          expectedInstruction = {
            .function = PING_RETURN,
            .data1 = ping
          };
          
          nrfTransmit(instruction);
      
          waitingSuccessfulReturn = true;  
  
          // Get ready for new input
          inString = "";
        }
      }
    }
  }
  // isSlave
  else if(nrfHasData()) {

    functionData lastReadInstruction = nrfGetLastReadInstruction();

    Serial.print("Received: ");
    Serial.println((uint8_t) lastReadInstruction.data1);

    // if the last read instruction was a ping
    // then we should return it
    if(lastReadInstruction.function == PING) {

      // Return the instruction and increment the data by 1
      // just to see if it is actually working correctly
      functionData newInstruction = {
        .function = PING_RETURN,
        .data1 = (lastReadInstruction.data1 + 1)
      };

      // Turn on relay 1
      if(lastReadInstruction.data1 == 1) {
        digitalWrite(A5, HIGH);
      }
      // Turn off relay 1
      else if(lastReadInstruction.data1 == 2) {
        digitalWrite(A5, LOW);
      }
      // Turn on relay 2
      else if(lastReadInstruction.data1 == 3) {
        digitalWrite(A0, HIGH);
      }
      // Turn off relay 2
      else if(lastReadInstruction.data1 == 4) {
        digitalWrite(A0, LOW);
      }
      // Turn on white lights
      else if(lastReadInstruction.data1 == 5) {
        for(int i=0;i<NUMPIXELS;i++){
          // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
          pixels.setPixelColor(i, pixels.Color(128, 128, 128));
          
          pixels.show(); // This sends the updated pixel color to the hardware.
        }
      }
      // Turn on blue/white lights
      else if(lastReadInstruction.data1 == 6) {
        for(int i=0;i<NUMPIXELS;i++){
          // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
          pixels.setPixelColor(i, pixels.Color(64, 64, 128));
          
          pixels.show(); // This sends the updated pixel color to the hardware.
        }
      }
      // Turn on blue lights only
      else if(lastReadInstruction.data1 == 7) {
        for(int i=0;i<NUMPIXELS;i++){
          // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
          pixels.setPixelColor(i, pixels.Color(0, 0, 128));
          
          pixels.show(); // This sends the updated pixel color to the hardware.
        }
      }
      // Turn off all lights
      else if(lastReadInstruction.data1 == 8) {
        for(int i=0;i<NUMPIXELS;i++){
          // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
          pixels.setPixelColor(i, pixels.Color(0, 0, 0));
          
          pixels.show(); // This sends the updated pixel color to the hardware.
        }
      }

      // Send a validation ping back
      nrfTransmit(newInstruction);
    }

    nrfClearHasData();
  }

  if(!isMaster) {

/*
    delay(1000);
    
    digitalWrite(A0, HIGH);
    digitalWrite(A5, HIGH);

    delay(1000);

    digitalWrite(A0, LOW);
    digitalWrite(A5, LOW);

    for(int i=0;i<NUMPIXELS;i++){
    
      // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
      pixels.setPixelColor(i, pixels.Color(255, 255, 255)); // Moderately bright green color.
      
      pixels.show(); // This sends the updated pixel color to the hardware.
      
      delay(500); // Delay for a period of time (in milliseconds).
      
    }

        */
    
  }
}

