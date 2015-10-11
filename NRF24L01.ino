// NRF24L01+ Test Code
// Kevin Darrah
// V2
//www.kevindarrah.com

#include <SPI.h>

//NRF      ARDUINO
//1 GND    GND
//2 VCC    3.3V
//3 CE     4  out
//4 CSN    5  out
//5 SCK    13 out
//6 MOSI   11 out
//7 MISO   12 in
//8 IRQ    2  in

// Define your pins here!
#define CE_pin 4
#define CSN_pin 5
#define IRQ_pin 2
#define MOSI_pin 11
#define MISO_pin 12
#define SCK_pin 13 

//The global variables used by everyone
byte data_in[5], data2, data3;

void nrfInit()
{
  // chip enable set as output
  pinMode(CE_pin, OUTPUT);
  
  // chip select pin as output
  pinMode(CSN_pin, OUTPUT);

  // SPI data out
  pinMode(MOSI_pin, OUTPUT);

  // SPI data in
  pinMode(MISO_pin, INPUT);

  // SPI clock out
  pinMode(SCK_pin, OUTPUT);
  Serial.println("NRF Pins Initialized");

  // SPI Most Significant Bit First
  SPI.setBitOrder(MSBFIRST);

  // Mode 0 Rising edge of data, keep clock low
  SPI.setDataMode(SPI_MODE0);

  //Run the data in at 16MHz/2 - 8MHz
  SPI.setClockDivider(SPI_CLOCK_DIV2);

  //RX mode
  digitalWrite(CE_pin, HIGH);

  //SPI idle
  digitalWrite(CSN_pin, HIGH);

  //start up the SPI library
  SPI.begin();
  Serial.println("NRF Ready");

  Serial.begin(115200);//start Serial
  Serial.println("Setting Up");
  delay(100);

  // pipe 0-5, bytes 1-32
  nrfSetRXPayload(0, 3);

  // register#, bit#, and value 0 or 1, ::  0,0,1 RX Mode
  nrfSetAddressBit(0, 0);

  // register, bit, and value 0,1,1 PowerUP
  nrfSetAddressBit(0, 1);

  // RT Mask turns off the RT interrupt
  nrfSetAddressBit(0, 4);

  // TX Mask turns off the TX interrupt
  nrfSetAddressBit(0, 5);
  
  nrfFlushRX();
  nrfFlushTX();

  // clears any interrupts
  nrfClearInterrupts();
  delay(100);
  
  //kick things off by attaching the IRQ interrupt
  attachInterrupt(0, nrfGetData, FALLING);
}

void nrfTransmit(functionData instruction)
{
  nrfFlushTX();

  functionData returnInstruction;
  
  digitalWrite(CSN_pin, LOW);
  // load TX payload
  byte statusRegister = SPI.transfer(B10100000);
  // load three bytes of data
  returnInstruction.function = SPI.transfer(instruction.function);
  returnInstruction.data1    = SPI.transfer(instruction.data1);
  returnInstruction.data2    = SPI.transfer(instruction.data2);
  digitalWrite(CSN_pin, HIGH);

  //pull CE pin LOW
  digitalWrite(CE_pin, LOW);
  
  //small delay
  delay(1);
  
  //go into TX mode
  nrfClearAddressBit(0, 0);
  
  //small delay
  delay(1);
  digitalWrite(CE_pin, HIGH);
  
  //this is the time CE pin must be HIGH for before going back into RX mode
  //delay(1) seems to work best for this.  any longer or shorter doesn't work as well
  delay(1);
  
  //go back into RX mode
  nrfSetAddressBit(0, 0);
}

void nrfFlushRX()
{
  // Flush RX
  digitalWrite(CSN_pin, LOW);
  byte statusRegister = SPI.transfer(B11100010);
  digitalWrite(CSN_pin, HIGH);
}

void nrfFlushTX()
{
  // Flush TX
  digitalWrite(CSN_pin, LOW);
  byte statusRegister = SPI.transfer(B11100001);
  digitalWrite(CSN_pin, HIGH);
}

// this routine is called when the IRQ pin is pulled LOW by the NRF
void nrfGetData()
{
  int i;
  digitalWrite(CSN_pin, LOW);
  //read the payload
  byte statusRegister = SPI.transfer(B01100001);
  data_in[1] = SPI.transfer(B00000000);
  data_in[2] = SPI.transfer(B00000000);
  data_in[3] = SPI.transfer(B00000000);
  digitalWrite(CSN_pin, HIGH);

  //data starting with '1' sets up the pinmode
  if(data_in[1]==1){

    //data3 is the mode of the pin 0=in 1=out
    if(data_in[3]==0) {
      //data2 is the pin, so set the pin
      pinMode(data_in[2], INPUT);
    }

    if(data_in[3]==1) {
      pinMode(data_in[2], OUTPUT);
    }
    
    delay(10);//very important delay - this lets the transmitter finish
    
    //up what is was doing before sending data back
    //send the information back for verification
    functionData newInstruction = {
      .function = 3,
      .data1 = data_in[2],
      .data2 = data_in[3]
    };
    
    nrfTransmit(newInstruction);
  }
  
  //data starting with '2' sets writes to the pin
  else if(data_in[1]==2) {

    //data3 is the value of the pin, 0=LOW 1=HIGH
    if(data_in[3]==0) {
      digitalWrite(data_in[2], LOW);
    }
        
    if(data_in[3]==1) {
      digitalWrite(data_in[2], HIGH);
    }
        
    delay(10);
    
    //send back for verification
    functionData newInstruction = {
      .function = 3,
      .data1 = data_in[2],
      .data2 = data_in[3]
    };
    
    nrfTransmit(newInstruction);
  }
  
  //echo back used to verify the right data was sent
  else if(data_in[1]==3) {
    data2 = data_in[2];
    data3 = data_in[3];
  }
  
  //not yet implemented, will be for analog reads probably
  else if(data_in[1]==5) {}
  
  //ping transmit
  else if(data_in[1]==6) {
    delay(10);
    //send ping back

    functionData newInstruction = {
      .function = 3,
      .data1 = data_in[2],
      .data2 = data_in[3]
    };
    
    nrfTransmit(newInstruction);
  }
  
  //not yet implemented, will be for dedicated function like temp reads
  else if(data_in[1]==7) {}
  
  //this is printed if a mode was not defined
  else {
    Serial.println("No Mode Byte Identified!");
    
    for(i=1; i<4; i++) {
      //just print out whatever was recieved
      Serial.print(char(data_in[i]));
    }
    
    Serial.println("  ");
  }
  
  nrfFlushRX();

  //clear the RX interrupt flag
  nrfSetAddressBit(7, 6);
}

void nrfSetRXPayload(byte pipe, byte bytes)
{
  // a register write starts at 32, so add on the 1 and 16 to get you to at R17
  byte address=pipe+32+16+1;
  digitalWrite(CSN_pin, LOW);

  // write register 11 RX_PW_P0
  byte statusRegister = SPI.transfer(address);

  // number of bytes in payload
  data_in[1] = SPI.transfer(bytes);
  digitalWrite(CSN_pin, HIGH);
  Serial.print("RX Payload Set RX_PW_P");
  Serial.print(pipe);
  Serial.print(" for ");
  Serial.print(bytes);
  Serial.println(" bytes");
}

void nrfSetAddressBit(byte address, byte bit_address)
{
  nrfWriteAddressBit(address, bit_address, 1);
}

void nrfClearAddressBit(byte address, byte bit_address)
{
  nrfWriteAddressBit(address, bit_address, 0);
}

//This routine writes single bits of a register, without affecting the rest of the register
void nrfWriteAddressBit(byte address, byte bit_address, byte val)
{
  //first read out the register
  byte registerByte = nrfGetAddress(address);

  //if we want to write a one to the bit then set the bit in the register we read
  if(val==1) {
    bitSet(registerByte, bit_address);
  } else {
    //clear it if not
    bitClear(registerByte, bit_address);
  }

  // Write the byte to the module
  nrfWriteByte(32+address, registerByte);
}

//there are three interrupt flags in the NRF.  This routine resets them all no matter what
void nrfClearInterrupts()
{
  // Reset RT interrupt
  nrfSetAddressBit(7, 4);
  
  // Reset TX interrupt
  nrfSetAddressBit(7, 5);

  // Reset RX interrupt
  nrfSetAddressBit(7, 6);
}


void nrfPing()
{
  //get a random byte
  int ping = random(256);
  Serial.print("Pinging with     ");
  Serial.print(ping);

  // send the ping out starting with 6 (basically a function id)
  functionData newInstruction = {
    .function = 6,
    .data1 = ping,
    .data2 = ping
  };
  
  nrfTransmit(newInstruction);

  //give it a little
  delay(15);

  //see if data2 came back with the ping
  if(data2 == ping) {
    Serial.println(" PING Successfull!! ");
  } else {
    Serial.println(" PING FAIL!! ");
  }

  //reset
  data2=0;
}

byte nrfGetAddress(byte address)
{
  return nrfWriteByte(address, B00000000);
}

byte nrfWriteByte(byte address, byte data)
{
  byte returnByte;
  
  //now we'll write the modified data back in
  digitalWrite(CSN_pin, LOW);
  //a write to a register adds 32
  returnByte = SPI.transfer(address);
  //write the modified register
  returnByte = SPI.transfer(data);
  digitalWrite(CSN_pin, HIGH);

  return returnByte;
}



