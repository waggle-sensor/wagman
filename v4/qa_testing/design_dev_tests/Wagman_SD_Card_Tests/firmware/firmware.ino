//#include "config.h"
#include <SPI.h>
#include <SD.h>
#define SD_CS 42

// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;
File myFile;
File root1;

void setup() {
  // Open serial communications and wait for port to open:
  SerialUSB.begin(115200);
  while (!SerialUSB) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  // Start the initialization process.
  SerialUSB.print("\nInitializing SD card...");
  if (!card.init(SPI_HALF_SPEED, SD_CS)) {
    SerialUSB.println("initialization failed. Things to check:");
    SerialUSB.println("* is a card inserted?");
    SerialUSB.println("* is your wiring correct?");
    SerialUSB.println("* did you change the chipSelect pin to match your shield or module?");
    while (1);
  } 
  else {
    SerialUSB.println("Wiring is correct and a card is present.");
  }

  // print the type of card
  SerialUSB.println();
  SerialUSB.print("Card type:         ");
  switch (card.type()) {
    case SD_CARD_TYPE_SD1:
      SerialUSB.println("SD1");
      break;
    case SD_CARD_TYPE_SD2:
      SerialUSB.println("SD2");
      break;
    case SD_CARD_TYPE_SDHC:
      SerialUSB.println("SDHC");
      break;
    default:
      SerialUSB.println("Unknown");
  }

  // Open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!volume.init(card)) {
    SerialUSB.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    while (1);
  }

  SerialUSB.print("Clusters:          ");
  SerialUSB.println(volume.clusterCount());
  SerialUSB.print("Blocks x Cluster:  ");
  SerialUSB.println(volume.blocksPerCluster());

  SerialUSB.print("Total Blocks:      ");
  SerialUSB.println(volume.blocksPerCluster() * volume.clusterCount());
  SerialUSB.println();

  // print the type and size of the first FAT-type volume
  uint32_t volumesize;
  SerialUSB.print("Volume type is:    FAT");
  SerialUSB.println(volume.fatType(), DEC);

  volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
  volumesize *= volume.clusterCount();       // we'll have a lot of clusters
  volumesize /= 2;                           // SD card blocks are always 512 bytes (2 blocks are 1KB)
  SerialUSB.print("Volume size (Kb):  ");
  SerialUSB.println(volumesize);
  SerialUSB.print("Volume size (Mb):  ");
  volumesize /= 1024;
  SerialUSB.println(volumesize);
  SerialUSB.print("Volume size (Gb):  ");
  SerialUSB.println((float)volumesize / 1024.0);
  SerialUSB.println("SD card initialization finished");
  if (!SD.begin(SD_CS)) {
    SerialUSB.println("initialization failed!");
    return;
  }
  SerialUSB.println("< ------------------------------------- >");
  delay(1000);
  SerialUSB.println("Starting listing files.");
  root1 = SD.open("/");
  printDirectory(root1, 0);
  SerialUSB.println("done!");
  SerialUSB.println("Now moving on to the Read & Write Tests");
  SerialUSB.println("< ------------------------------------- >");
  SerialUSB.println("Test Writing to a file.");
  myFile = SD.open("test.txt", FILE_WRITE);
  if (myFile) {
    SerialUSB.print("Writing to test.txt...");
    myFile.println("testing 1, 2, 3.");
    SerialUSB.print("closing the file ");
    myFile.close();
    SerialUSB.println("done.");
  } else {
    SerialUSB.println("error opening test.txt");
  }
  SerialUSB.println("finished writing test");
  SerialUSB.println("Starting the reading test");
  myFile = SD.open("test.txt");
  if (myFile) {
    SerialUSB.println("test.txt:");

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      SerialUSB.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    SerialUSB.println("error opening test.txt");
  }
  SerialUSB.println("finished reading test.");
  

  
}

void loop(void) {
}

void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      SerialUSB.print('\t');
    }
    SerialUSB.print(entry.name());
    if (entry.isDirectory()) {
      SerialUSB.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      SerialUSB.print("\t\t");
      SerialUSB.println(entry.size(), DEC);
    }
    entry.close();
  }
}

