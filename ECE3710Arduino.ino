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

bool isMaster = true;

void setup()
{
  //set up the SPI and define pins
  nrfInit();
}

void loop()
{
  if(isMaster) {
    //ping random byte out
    nrfPing();
  }
  // isSlave
  else {
    if(nrfHasData()) {
      
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
  
  // nrfPing();
}

