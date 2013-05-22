#include <cstdlib>
#include <iostream>

#include "../RF24.h"
#define NODE_ID 31        // The ID of this node
unsigned int readingNum = 0;

//
// Hardware configuration
//

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10

//RF24 radio(9,10);
RF24 radio("/dev/spidev0.0",8000000 , 25);  //spi device, speed and CSN,only CSN is NEEDED in RPI


//
// Topology
//

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0D2LL, 0xF0F0F0F0E1LL };

void setup(void)
{
  printf("\n\rSwitch relay in the garage on \n\r");

  //
  // Setup and configure rf radio
  //

  radio.begin();
  radio.setDataRate(RF24_250KBPS);
  // optionally, increase the delay between retries & # of retries
  radio.setRetries(15,15);

  // optionally, reduce the payload size.  seems to
  // improve reliability
  radio.setPayloadSize(5);
  radio.setChannel(24);
  radio.setPALevel(RF24_PA_MAX);
  radio.setAutoAck(true);
  //
  // Open pipes to other nodes for communication
  //

  // This simple sketch opens two pipes for these two nodes to communicate
  // back and forth.
  // Open 'our' pipe for writing
  // Open the 'other' pipe for reading, in position #1 (we can have up to 5 pipes open for reading)

  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1,pipes[1]);

  radio.powerUp();
  radio.printDetails();
}

bool doSendMsg(unsigned int xiData, unsigned int xiSensorNum, unsigned char xiMsgNum)
{
  // Send a message with the following format
  // 5 bit node ID 
  // 5 bit reading number 
  // 5 bit sensor number - I struggled a lot to add this. Seems to work now.
  // 1 bit unused 
  // 16 bit data
  // 8 bit checksum

    // This is a total of 5 unsigned chars
  unsigned char databuf[5];
  unsigned char nodeID = (NODE_ID & 0b11111);
  unsigned char msgNum = (xiMsgNum & 0b11111);
  unsigned char sensorNum = (xiSensorNum & 0b11111);

  databuf[0] = (nodeID << 3) | (msgNum >> 2);
  // databuf[1] = ((msgNum & 0b00000011) << 6 ) | (sensorNum >> 2);
  databuf[1] = (msgNum  << 6  | (sensorNum & 0b00011111) << 1);
  databuf[2] = ((0xFF00 & xiData) >> 8);
  databuf[3] = (0x00FF & xiData);
  databuf[4] = databuf[0] | databuf[1] | databuf[2] | databuf[3];

    printf("Now sending message\r\n");
    bool ok = radio.write( &databuf, sizeof(databuf) );
    return ok;
}


bool sendMsg(unsigned int sensornum, unsigned int data)
{
  readingNum++;
  if (readingNum >= 65535) readingNum = 0;  // Isn't 65535 what you can store in an unsigned int?

  return(doSendMsg(data, sensornum, readingNum));
}




int main(int argc, char** argv)
{
    setup();
    int duration=0;
    if (argc>1) {
	duration=atoi(argv[1]);
    } else {
	printf("no milliseconds given. exiting.\r\n");
	exit (0);
    }

    
    while (!sendMsg(31,duration)) {
      printf("Sending a button press of %i ms failed. Retrying...\n\r",duration);
    } 
    printf("Button press of %i ms succeeded\n\r", duration);

    return 0;
}
