//NRF      ARDUINO
//1 GND    GND
//2 VCC    3.3V
//3 CE     4  out
//4 CSN    5  out
//5 SCK    13 out
//6 MOSI   11 out
//7 MISO   12 in
//8 IRQ    2  in

#include "TypesAndDefines.h"

functionData expectedInstruction;
bool isMaster = true, waitingSuccessfulReturn = false;
// Allow the transmit to fail 10 times before skipping that packet
int allowedFails = 10;
// Track the current number of fails
int currentFails = 0;

void setup()
{
  //set up the SPI and define pins
  nrfInit();
}

void loop()
{
  if(isMaster) {

    if(waitingSuccessfulReturn && nrfHasData()) {

      functionData lastReadInstruction = nrfGetLastReadInstruction();

      if(lastReadInstruction.data1 == expectedInstruction.data1) {
        Serial.println(" Ping Successful");
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
      //get a random byte
      int ping = random(256);
      Serial.print("Pinging with     ");
      Serial.print(ping);
      Serial.print(" ");
    
      // send the ping out starting with 6 (basically a function id)
      functionData instruction = {
        .function = PING,
        .data1 = ping,
        .data2 = ping
      };
  
      expectedInstruction = {
        .function = PING_RETURN,
        .data1 = ping,
        .data2 = ping
      };
      
      nrfTransmit(instruction);
  
      waitingSuccessfulReturn = true;  
    }
  }
  // isSlave
  else if(nrfHasData()) {
    
    functionData lastReadInstruction = nrfGetLastReadInstruction();

    // if the last read instruction was a ping
    // then we should return it
    if(lastReadInstruction.function == PING) {
    
      functionData newInstruction = {
        .function = PING_RETURN,
        .data1 = lastReadInstruction.data1,
        .data2 = lastReadInstruction.data2
      };

      nrfTransmit(newInstruction);
    } 
  }
}

