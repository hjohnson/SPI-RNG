/*
Random_Read: A test protocol for the SPI RNG project
 CC BY/SA Harry Johnson
http://github.com/hjohnson/SPI-RNG
 
 When executed on the Arduino, this program will absolutely flood the Serial interface with 
 random numbers (by default, 200,000 of them) in the range of 0-255, with linefeeds in between. 
 If you capture this stream of numbers to a text file (it'll take a while!) using your favorite Serial Terminal (I like coolterm on the Mac),
 then import them into your favorite data processing tool, (I prefer Matlab), you can do all sorts of fun things, like 
 generate frequency histograms or perform statistical tests. Included in a subdirectory is one such text file, along with its frequency histogram.
 */

#include <SPI.h>

//Hook up the SPI RNG to the Arduino using this pinout:
const unsigned char slaveSelectPin = 10;
const unsigned char MOSIPin = 11;
const unsigned char MISOPin = 12;
const unsigned char SCKPin = 13; 

void setup() {
  unsigned long num_reads = 200000; //number of random numbers to return.
  unsigned long ii = 0; //counter.
  
  Serial.begin(9600);
  digitalWrite(slaveSelectPin, HIGH); //just in case, we don't want to start by selecting the chip.
  pinMode (slaveSelectPin, OUTPUT);   // set the slaveSelectPin as an output:
  pinMode(MOSIPin, OUTPUT); //Master Out, Slave In
  pinMode(MISOPin, INPUT); //Master In, Slave Out
  pinMode(SCKPin, OUTPUT); //SPI Clock

  // initialize SPI:
  SPI.setClockDivider(SPI_CLOCK_DIV32);  //500K BPS, somewhat arbitrary, but still 62.5KsPs
  SPI.setDataMode(0); //various other SPI initializations, these should be default, but let's be sure.
  SPI.setBitOrder(MSBFIRST); 
  SPI.begin(); 

  digitalWrite(slaveSelectPin, LOW); //Slave Select Low activates device.
  SPI.transfer(0); //to let the RNG prep a number in the out buffer.
  delay(1);
  for(ii = 0; ii<num_reads; ii++) { //return a certain number of random values, see above.
    Serial.println(SPI.transfer(0)); //it doesn't matter what the input is, just the response. Print the response.
    delay(1); //Keep this delay: the SPI RNG needs time to load numbers, etc.
  }
  digitalWrite(slaveSelectPin, HIGH); //deselect the SPI RNG.
}

void loop() { //everything handled in setup(). 
}

