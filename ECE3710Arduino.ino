//NRF      ARDUINO
//1 GND    GND
//2 VCC    3.3V
//3 CE     4  out
//4 CSN    5  out
//5 SCK    13 out
//6 MOSI   11 out
//7 MISO   12 in
//8 IRQ    2  in

#include "FunctionData.h"

void setup()
{
  //set up the SPI and define pins
  nrfInit();
}

void loop()
{
  //ping random byte out
  nrfPing();
}

