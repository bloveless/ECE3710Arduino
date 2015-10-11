//NRF24L01+ Test Code
// Kevin Darrah
// V2
//www.kevindarrah.com


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
#include <SPI.h>
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

  //pipe 0-5, bytes 1-32
  nrfSetRXPayload(0, 3);

  //Get Reg7 Status, 1=print screen
  nrfGetAddress(7, 1);

  //register#, bit#, and value 0 or 1, ::  0,0,1 RX Mode
  nrfSetAddressBit(0, 0);

  //register, bit, and value 0,1,1 PowerUP
  nrfSetAddressBit(0, 1);

  //RT Mask turns off the RT interrupt
  nrfSetAddressBit(0, 4);

  //TX Mask turns off the TX interrupt
  nrfSetAddressBit(0, 5);
  
  nrfFlushRX();
  nrfFlushTX();

  //clears any interrupts
  nrfClearInterrupts();
  delay(100);
  
  //kick things off by attaching the IRQ interrupt
  attachInterrupt(0, nrfGetData, FALLING);
}

void nrfTransmit(byte data1, byte data2, byte data3)
{
  nrfFlushTX();
  
  digitalWrite(CSN_pin, LOW);
  // load TX payload
  data_in[0] = SPI.transfer(B10100000);
  // load three bytes of data
  data_in[1] = SPI.transfer(data1);
  data_in[2] = SPI.transfer(data2);
  data_in[3] = SPI.transfer(data3);
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
  data_in[0] = SPI.transfer(B11100010);
  digitalWrite(CSN_pin, HIGH);
}

void nrfFlushTX()
{
  // Flush TX
  digitalWrite(CSN_pin, LOW);
  data_in[0] = SPI.transfer(B11100001);
  digitalWrite(CSN_pin, HIGH);
}

// this routine is called when the IRQ pin is pulled LOW by the NRF
void nrfGetData()
{
  int i;
  digitalWrite(CSN_pin, LOW);
  data_in[0] = SPI.transfer(B01100001);//read the payload
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
    nrfTransmit(3,data_in[2],data_in[3]);//send the information back for verification
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
    nrfTransmit(3,data_in[2],data_in[3]);
  }
  
  //echo back used to verify the right data was sent
  else if(data_in[1]==3) {
    data2 = data_in[2];
    data3 = data_in[3];
  }
  
  //4 is used to do digital reads
  else if(data_in[1]==4) {
    delay(10);
    nrfTransmit(3,data_in[2],digitalRead(data_in[2]));//everything comes back with the echo
  }
  
  //not yet implemented, will be for analog reads probably
  else if(data_in[1]==5) {}
  
  //ping transmit
  else if(data_in[1]==6) {
    delay(10);
    //send ping back
    nrfTransmit(3,data_in[2],data_in[3]);
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
  data_in[0] = SPI.transfer(address);

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
  nrfGetAddress(address, 0);

  //if we want to write a one to the bit then set the bit in the register we read
  if(val==1) {
    bitSet(data_in[1], bit_address);
  } else {
    //clear it if not
    bitClear(data_in[1], bit_address);
  }

  //now we'll write the modified data back in
  digitalWrite(CSN_pin, LOW);
  //a write to a register adds 32
  data_in[0] = SPI.transfer(32+address);
  //write the modified register
  data_in[1] = SPI.transfer(data_in[1]);
  digitalWrite(CSN_pin, HIGH);
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
  nrfTransmit(6, ping, ping);

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

//     MASSIVE ROUTINE HERE - PUT ALL ROUTINES ABOVE THIS

void nrfGetAddress(byte address, byte info)
{
  //send the address and either a 1 or 0 if you want to do a serial print of the address
  //after a call to this routine, data_in[1] will equal the address you called

  digitalWrite(CSN_pin, LOW);
  data_in[0] = SPI.transfer(address);
  data_in[1] = SPI.transfer(B00000000);
  digitalWrite(CSN_pin, HIGH);

  // if the user wanted it, you will get a print out of the register - good fo debugging
  if(info==1){
    Serial.print("R");
    Serial.print(address);
    
    switch (address) {
      case 0:
        Serial.print(" CONFIG REGISTER =");
        Serial.println(data_in[1]);
        Serial.print("PRIM_RX = ");
   
        if(bitRead(data_in[1],0)) {
          Serial.println("PRX");
        } else {
          Serial.println("PTX");
        }
   
        Serial.print("PWR_UP = ");
        if(bitRead(data_in[1],1)) {
          Serial.println("POWER UP");
        } else {
          Serial.println("POWER DOWN");
        }
   
        Serial.print("CRCO = ");
        if(bitRead(data_in[1],2)) {
          Serial.println("2Bytes");
        } else {
          Serial.println("1Byte");
        }
   
        Serial.print("EN_CRC = ");
        if(bitRead(data_in[1],3)) {
          Serial.println("Enabled");
        } else {
          Serial.println("Disabled");
        }
 
        Serial.print("MASK_MAX_RT = ");
        if(bitRead(data_in[1],4)) {
          Serial.println("Interrupt not reflected on the IRQ pin");
        } else {
          Serial.println("Reflect MAX_RT as active low interrupt on the IRQ pin");
        }
 
        Serial.print("MASK_TX_DS = ");
        if(bitRead(data_in[1],5)) {
          Serial.println("Interrupt not reflected on the IRQ pin");
        } else {
          Serial.println("Reflect TX_DS as active low interrupt on the IRQ pin");
        }
   
        Serial.print("MASK_RX_DR = ");
        if(bitRead(data_in[1],6)) {
          Serial.println("Interrupt not reflected on the IRQ pin");
        } else {
          Serial.println("Reflect RX_DR as active low interrupt on the IRQ pin");
        }
        
        break; // 0
        
      case 1:
        Serial.print(" EN_AA REGISTER Enhanced ShockBurst =");
        Serial.println(data_in[1]);
        break; // 1
        
      case 2:
        Serial.print(" EN_RXADDR REGISTER Enabled RX Addresses =");
        Serial.println(data_in[1]);
        break; // 2
        
      case 3:
        Serial.print(" SETUP_AW REGISTER Setup of Address Widths =");
        Serial.println(data_in[1]);
        break; // 3
        
      case 4:
        Serial.print(" SETUP_RETR REGISTER Setup of Automatic Retransmission =");
        Serial.println(data_in[1]);
        break; // 4
        
      case 5:
        Serial.print(" RF_CH REGISTER RF Channel =");
        Serial.println(data_in[1]);
        break; // 5
        
      case 6:
        Serial.print(" RF_SETUP REGISTER RF Setup Register =");
        Serial.println(data_in[1]);
        Serial.print("RF Power = ");
        Serial.print(bitRead(data_in[1],2));
        Serial.println(bitRead(data_in[1],1));
        Serial.print("RF_DR_HIGH = ");
        Serial.println(bitRead(data_in[1],3));
        Serial.print("PLL_LOCK = ");
        Serial.println(bitRead(data_in[1],4));
        Serial.print("RF_DR_LOW = ");
        Serial.println(bitRead(data_in[1],5));  
        Serial.print("CONT_WAVE = ");
        Serial.println(bitRead(data_in[1],7));   
        break; // 6
        
      case 7:
        Serial.print(" STATUS REGISTER  =");
        Serial.println(data_in[1]);
        
        Serial.print("TX_FULL = ");
        if(bitRead(data_in[1],0)) {
          Serial.println("TX FIFO full");
        } else {
          Serial.println("TX FIFO Not full");
        }
   
        Serial.print("RX_P_NO = ");
        if(bitRead(data_in[1],1)&&(data_in[1],2)&&(data_in[1],3)) {
          Serial.println("RX FIFO Empty");
        } else {
          Serial.println(bitRead(data_in[1],1)+(bitRead(data_in[1],2)<<1)+(bitRead(data_in[1],2)<<2));
        }
        
        Serial.print("MAX_RT Interrupt = ");
        Serial.println(bitRead(data_in[1],4));
        Serial.print("TX_DS Interrupt = ");
        Serial.println(bitRead(data_in[1],5));
        Serial.print("RX_DR Interrupt = ");
        Serial.println(bitRead(data_in[1],6));
        break; // 7
        
      case 8:
        Serial.print(" OBSERVE_TX REGISTER Transmit observe register  =");
        Serial.println(data_in[1]);
        Serial.print("ARC_CNT = "); 
        Serial.println(bitRead(data_in[1],0)+(bitRead(data_in[1],1)<<1)+(bitRead(data_in[1],2)<<2)+(bitRead(data_in[1],3)<<3));
        Serial.print("PLOS_CNT = "); 
        Serial.println(bitRead(data_in[1],4)+(bitRead(data_in[1],5)<<1)+(bitRead(data_in[1],6)<<2)+(bitRead(data_in[1],7)<<3));
        break; // 8
        
      case 9:
        Serial.print(" RPD REGISTER Received Power Detector =");
        Serial.println(bitRead(data_in[1],0));
        break; // 9
        
      case 10:
        Serial.print(" RX_ADDR_P0 LSB =");
        Serial.println(data_in[1]);
        break; // 10
        
      case 11:
        Serial.print(" RX_ADDR_P1 LSB =");
        Serial.println(data_in[1]);
        break; // 11
        
      case 12:
        Serial.print(" RX_ADDR_P2 LSB =");
        Serial.println(data_in[1]);
        break; // 12
        
      case 13:
        Serial.print(" RX_ADDR_P3 LSB =");
        Serial.println(data_in[1]);
        break; // 13
        
      case 14:
        Serial.print(" RX_ADDR_P4 LSB =");
        Serial.println(data_in[1]);
        break; // 14
        
      case 15:
        Serial.print(" RX_ADDR_P5 LSB =");
        Serial.println(data_in[1]);
        break; // 15

      case 16:
        Serial.print(" TX_ADDR LSB =");
        Serial.println(data_in[1]);
        break; // 16
        
      case 17:
        Serial.print(" RX_PW_P0 RX payload =");
        Serial.println(data_in[1]);
        break; // 17
        
      case 18:
        Serial.print(" RX_PW_P1 RX payload =");
        Serial.println(data_in[1]);
        break; // 18
        
      case 19:
        Serial.print(" RX_PW_P2 RX payload =");
        Serial.println(data_in[1]);
        break; // 19
        
      case 20:
        Serial.print(" RX_PW_P3 RX payload =");
        Serial.println(data_in[1]);
        break; // 20
        
      case 21:
        Serial.print(" RX_PW_P4 RX payload =");
        Serial.println(data_in[1]);
        break; // 21

      case 22:
        Serial.print(" RX_PW_P5 RX payload =");
        Serial.println(data_in[1]);
        break; // 22

      case 23:
        Serial.print(" FIFO_STATUS Register =");
        Serial.println(data_in[1]);
        
        Serial.print("RX_EMPTY = ");
        if(bitRead(data_in[1],0)) {
          Serial.println("RX FIFO empty");
        } else {
          Serial.println("Data in RX FIFO");
        }
   
        Serial.print("RX_EMPTY = ");
        if(bitRead(data_in[1],1)) {
          Serial.println("RX FIFO full");
        } else {
          Serial.println("Available locations in RX FIFO");
        }
 
        Serial.print("TX_EMPTY = ");
        if(bitRead(data_in[1],4)) {
          Serial.println("TX FIFO empty");
        } else {
          Serial.println("Data in TX FIFO");
        }
  
        Serial.print("TX_FULL = ");
        if(bitRead(data_in[1],5)) {
          Serial.println("TX FIFO full");
        } else {
          Serial.println("Available locations in TX FIFO");
        }
        
        Serial.print("TX_REUSE = ");
        Serial.println(bitRead(data_in[1],6));
        break; // 23
        
    } // switch
  } // if 1
}


