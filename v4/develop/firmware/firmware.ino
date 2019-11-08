// This file is part of the Waggle Platform.Please see the file
// LICENSE.waggle.txt for the legal details of the copyright and software
// license.For more details on the Waggle project, visit:
// http://www.wa8.gl
#include "Device.h"
#include "EEPROM.h"
#include "Error.h"
#include "Logger.h"
#include "MCP79412RTC.h"
#include "Record.h"
#include "Timer.h"
#include "Wagman.h"
#include "buildinfo.cpp"
#include "commands.h"
#include "verinfo.cpp"

#include "DueTimer.h"

#include <SD.h>
#include <SPI.h>
#include "waggle.h"
#define SD_CS 42

#warning "Using mocked out MCUSR register."
int MCUSR = 0;

const int WDRF = 0;
const int BORF = 1;
const int EXTRF = 2;
const int PORF = 3;

constexpr int _BV(int x) { return 1 << x; }

void watchdogSetup() {
  // I moved this to setup() just so sequence of events is more clear.
  // Definiing this function is enough to override the default behavior,
  // you don't actually need to do watchdogEnable here.

  // watchdogReset();
  // watchdogEnable(8000); // 8s watchdog
  // watchdogReset();
}

// TODO Look into watchdog.
// TODO Define EEPROM file which has same interface, but talks to EEPROM IC over
// bus. write small test case for this.

void setupDevices();
void checkSensors();
void checkCurrentSensors();
void checkThermistors();
unsigned long meanBootDelta(const Record::BootLog &bootLog, byte maxSamples);

void printDate(const DateTime &dt);
void printID(byte mac[8]);

static const byte DEVICE_COUNT = 5;
static const byte BUFFER_SIZE = 80;
static const byte MAX_ARGC = 8;

byte bootflags = 0;
bool shouldResetSystem = false;
unsigned long shouldResetTimeout = 0;
DurationTimer shouldResetTimer;
bool logging = true;
byte deviceWantsStart = 255;

Device devices[DEVICE_COUNT];

DurationTimer startTimer;
static DurationTimer statusTimer;

static char buffer[BUFFER_SIZE];
static byte bufferSize = 0;

static time_t setupTime;

struct : public writer {
  int write(const byte *s, int n) {
    int maxn = SerialUSB.availableForWrite();

    if (n > maxn) {
      n = maxn;
    }

    return SerialUSB.write(s, n);
  }
} serial_writer;

void printDate(const DateTime &dt) {
  SerialUSB.print(dt.year);
  SerialUSB.print(' ');
  SerialUSB.print(dt.month);
  SerialUSB.print(' ');
  SerialUSB.print(dt.day);
  SerialUSB.print(' ');
  SerialUSB.print(dt.hour);
  SerialUSB.print(' ');
  SerialUSB.print(dt.minute);
  SerialUSB.print(' ');
  SerialUSB.print(dt.second);
}

void printID(byte id[8]) {
  for (byte i = 2; i < 8; i++) {
    SerialUSB.print(id[i] >> 4, HEX);
    SerialUSB.print(id[i] & 0x0F, HEX);
  }
}

#define COMMAND_ID 1
#define COMMAND_CU 2
#define COMMAND_HB 3
#define COMMAND_BOOTS 4
#define COMMAND_UPTIME 7
#define COMMAND_FC 10
#define COMMAND_START 5
#define COMMAND_STOP 6
#define COMMAND_ENABLE 40
#define COMMAND_EERESET 30
#define COMMAND_RESET 31
#define COMMAND_PING 32
#define COMMAND_VOLTAGE 33
#define COMMAND_TH 34
#define COMMAND_STATE 50

void basicResp(writer &w, int id, int err) {
  sensorgram_encoder<64> e(w);
  e.info.id = id;
  e.encode_uint(err);
  e.encode();
}

/*
Command:
Send Ping

Description:
Sends an "external" heartbeat signal for a device.

Examples:
# resets heartbeat timeout on device 1
$ wagman-client ping 1
*/
byte commandPing(writer &w, int port) {
  if (Wagman::validPort(port)) {
    devices[port].sendExternalHeartbeat();
    basicResp(w, COMMAND_PING, 0);
  } else {
    basicResp(w, COMMAND_PING, 1);
  }

  return 0;
}

/*
Command:
Start Device

Description:
Starts a device.

Examples:
# start the node controller
$ wagman-client start 0

# start the guest node
$ wagman-client start 1
*/
byte commandStart(writer &w, int port) {
  if (Wagman::validPort(port)) {
    deviceWantsStart = port;
    basicResp(w, COMMAND_START, 0);
  } else {
    basicResp(w, COMMAND_START, 1);
  }

  return 0;
}

/*
Command:
Stop Device

Description:
Stops a device with an optional delay.

Examples:
# stop guest node after 30 seconds
$ wagman-client stop 1 30

# stop guest node immediately
$ wagman-client stop 1 0
*/
byte commandStop(writer &w, int port, int dur) {
  if (Wagman::validPort(port)) {
    devices[port].setStopTimeout((unsigned long)dur * 1000);
    devices[port].stop();
    basicResp(w, COMMAND_STOP, 0);
  } else {
    basicResp(w, COMMAND_STOP, 1);
  }

  return 0;
}

/*
Command:
Reset Wagman

Description:
Resets the Wagman with an optional delay.

Examples:
# resets the wagman immediately
$ wagman-client reset

# resets the wagman after 90 seconds
$ wagman-client reset 90
*/
byte commandReset(writer &w) {
  shouldResetSystem = true;
  shouldResetTimer.reset();
  shouldResetTimeout = 0;
  basicResp(w, COMMAND_RESET, 0);
  return 0;
}

/*
Command:
Get Wagman ID

Description:
Gets the onboard Wagman ID.

Examples:
$ wagman-client id
*/
byte commandID(writer &w) {
  byte id[8];

  Wagman::getID(id);

  sensorgram_encoder<64> e(w);
  e.info.id = COMMAND_ID;
  e.encode_bytes(id, 8);
  e.encode();

  return 0;
}

/*
Command:
Get / Set Date

Description:
Gets / sets the RTC date.

Examples:
# gets the date
$ wagman-client date

# sets the date
$ wagman-client date 2016 03 15 13 00 00
*/
byte commandDate(byte argc, const char **argv) {
  if (argc != 1 && argc != 7) return ERROR_INVALID_ARGC;

  if (argc == 1) {
    DateTime dt;
    Wagman::getDateTime(dt);
    printDate(dt);
    SerialUSB.println();
  } else if (argc == 7) {
    DateTime dt;

    dt.year = atoi(argv[1]);
    dt.month = atoi(argv[2]);
    dt.day = atoi(argv[3]);
    dt.hour = atoi(argv[4]);
    dt.minute = atoi(argv[5]);
    dt.second = atoi(argv[6]);

    Wagman::setDateTime(dt);
  }

  return 0;
}

/*
Command:
Get Current Values

Description:
Gets the current from the system and all devices. The outputs are formatted as:
System Device0 Device1 ... Device4

Examples:
$ wagman-client cu
*/
byte commandCurrent(writer &w) {
  sensorgram_encoder<64> e(w);
  e.info.id = COMMAND_CU;
  e.encode_uint(Wagman::getCurrent());
  e.encode_uint(Wagman::getCurrent(0));
  e.encode_uint(Wagman::getCurrent(1));
  e.encode_uint(Wagman::getCurrent(2));
  e.encode_uint(Wagman::getCurrent(3));
  e.encode_uint(Wagman::getCurrent(4));
  e.encode();
  return 0;
}

byte commandVoltage(writer &w) {
  sensorgram_encoder<64> e(w);
  e.info.id = COMMAND_VOLTAGE;
  e.encode_uint(Wagman::getVoltage(0));
  e.encode_uint(Wagman::getVoltage(1));
  e.encode_uint(Wagman::getVoltage(2));
  e.encode_uint(Wagman::getVoltage(3));
  e.encode_uint(Wagman::getVoltage(4));
  e.encode();
  return 0;
}

/*
Command:
Get Heartbeats

Description:
Gets the time since last seeing a heartbeat for each device. The outputs are
formatted as: Device0 Device1
...
Device4

Examples:
$ wagman-client hb
*/
byte commandHeartbeat(writer &w) {
  sensorgram_encoder<64> e(w);
  e.info.id = COMMAND_HB;
  e.encode_uint(devices[0].timeSinceHeartbeat());
  e.encode_uint(devices[1].timeSinceHeartbeat());
  e.encode_uint(devices[2].timeSinceHeartbeat());
  e.encode_uint(devices[3].timeSinceHeartbeat());
  e.encode_uint(devices[4].timeSinceHeartbeat());
  e.encode();
  return 0;
}

/*
Command:
Get Fail Counts

Description:
Gets the number of device failures for each device. Currently, this only
includes heartbeat timeouts. Device0 Device1
...
Device4

Examples:
$ wagman-client fc
*/
byte commandFailCount(writer &w) {
  sensorgram_encoder<64> e(w);
  e.info.id = COMMAND_FC;
  e.encode_uint(Record::getBootFailures(0));
  e.encode_uint(Record::getBootFailures(1));
  e.encode_uint(Record::getBootFailures(2));
  e.encode_uint(Record::getBootFailures(3));
  e.encode_uint(Record::getBootFailures(4));
  e.encode();
  return 0;
}

/*
Command:
Get Thermistor Values

Description:
Gets the thermistor values for each device. The outputs are formatted as:
Device0
Device1
...
Device4

Examples:
$ wagman-client th
*/
byte commandThermistor(writer &w) {
  sensorgram_encoder<64> e(w);
  e.info.id = COMMAND_TH;
  e.encode_uint(Wagman::getThermistor(0));
  e.encode_uint(Wagman::getThermistor(1));
  e.encode_uint(Wagman::getThermistor(2));
  e.encode_uint(Wagman::getThermistor(3));
  e.encode_uint(Wagman::getThermistor(4));
  e.encode();
  return 0;
}

/*
Command:
Get Environment Sensor Values

Description:
Gets the onboard temperature and humidity sensor values.

Examples:
$ wagman-client env
*/
byte commandEnvironment(writer &w) {
  // split into individual sensors
  {
    unsigned int raw;
    float hrf;
    bool ok = Wagman::getTemperature(&raw, &hrf);

    if (ok) {
      sensorgram_encoder<64> e(w);
      e.info.id = 31;
      e.encode_uint(raw);
      e.encode();
    }
  }

  {
    unsigned int raw;
    float hrf;
    bool ok = Wagman::getHumidity(&raw, &hrf);

    if (ok) {
      sensorgram_encoder<64> e(w);
      e.info.id = 32;
      e.encode_uint(raw);
      e.encode();
    }
  }

  {
    unsigned int raw;
    bool ok = Wagman::getLight(&raw);

    if (ok) {
      sensorgram_encoder<64> e(w);
      e.info.id = 33;
      e.encode_uint(raw);
      e.encode();
    }
  }

  return 0;
}

/*
Command:
Get / Set Device Boot Media

Description:
Gets / sets the next boot media for a device. The possible boot media are `sd`
and `emmc`.

Examples:
# gets the selected boot media for the node controller.
$ wagman-client bs 0

# set the selected boot media for the guest node to sd
$ wagman-client bs 1 sd

# set the selected boot media for the guest node to emmc
$ wagman-client bs 1 emmc
*/
// TODO split as get vs set
byte commandBootMedia(writer &w) {
  // if (argc != 2 && argc != 3) return ERROR_INVALID_ARGC;

  // byte index = atoi(argv[1]);

  // if (!Wagman::validPort(index)) return ERROR_INVALID_PORT;

  // if (argc == 3) {
  //   if (strcmp(argv[2], "sd") == 0) {
  //     devices[index].setNextBootMedia(MEDIA_SD);
  //     SerialUSB.println("set sd");
  //   } else if (strcmp(argv[2], "emmc") == 0) {
  //     devices[index].setNextBootMedia(MEDIA_EMMC);
  //     SerialUSB.println("set emmc");
  //   } else {
  //     SerialUSB.println("invalid media");
  //   }
  // } else if (argc == 2) {
  //   byte bootMedia = devices[index].getNextBootMedia();

  //   if (bootMedia == MEDIA_SD) {
  //     SerialUSB.println("sd");
  //   } else if (bootMedia == MEDIA_EMMC) {
  //     SerialUSB.println("emmc");
  //   } else {
  //     SerialUSB.println("invalid media");
  //   }
  // }

  return 0;
}

/*
Command:
Get Boot Flags

Description:
Gets the system boot flags. The possible results are:
* WDRF: Reset by watchdog.
* EXTF: Reset by external reset line.
* BORF: Detected brownout.
* PORF: Detected power on. (Should always be set.)

Examples:
$ wagman-client bf
*/
// byte commandBootFlags(__attribute__((unused)) byte argc,
//                       __attribute__((unused)) const char **argv) {
//   if (bootflags & _BV(WDRF)) SerialUSB.println("WDRF");
//   if (bootflags & _BV(BORF)) SerialUSB.println("BORF");
//   if (bootflags & _BV(EXTRF)) SerialUSB.println("EXTRF");
//   if (bootflags & _BV(PORF)) SerialUSB.println("PORF");
//   return 0;
// }

/*
Command:
Get Uptime

Description:
Gets the system uptime in seconds.

Examples:
$ wagman-client up
*/
byte commandUptime(writer &w) {
  sensorgram_encoder<64> e(w);
  e.info.id = COMMAND_UPTIME;
  e.encode_uint(millis() / 1000);
  e.encode();
  return 0;
}

/*
Command:
Enable Device

Description:
Enables a device to be powered on automatically. Note that this doesn't
immediately start the device.

Examples:
# enable the coresense
$ wagman-client enable 2
*/
byte commandEnable(writer &w, int port) {
  if (Wagman::validPort(port)) {
    devices[port].enable();
    basicResp(w, COMMAND_ENABLE, 0);
  } else {
    basicResp(w, COMMAND_ENABLE, 1);
  }

  return 0;
}

byte commandState(writer &w) {
  sensorgram_encoder<64> e(w);
  e.info.id = COMMAND_STATE;
  e.encode_uint(devices[0].getState());
  e.encode_uint(devices[1].getState());
  e.encode_uint(devices[2].getState());
  e.encode_uint(devices[3].getState());
  e.encode_uint(devices[4].getState());
  e.encode();
  return 0;
}

/*
Command:
Get Wagman Boot Count

Description:
Gets the number of times the system has booted.

Examples:
$ wagman-client boots
*/
byte commandBoots(writer &w) {
  unsigned long count;
  Record::getBootCount(count);

  sensorgram_encoder<64> e(w);
  e.info.id = COMMAND_BOOTS;
  e.encode_uint(count);
  e.encode();
  return 0;
}

/*
Command:
Get Wagman Version

Description:
Gets the hardware / firmware versions of the system.

Examples:
$ wagman-client ver
*/
byte commandVersion(byte argc, const char **argv) {
  SerialUSB.print("hw ");
  SerialUSB.print(WAGMAN_HW_VER_MAJ);
  SerialUSB.print('.');
  SerialUSB.println(WAGMAN_HW_VER_MIN);

  SerialUSB.print("ker ");
  SerialUSB.print(WAGMAN_KERNEL_MAJ);
  SerialUSB.print('.');
  SerialUSB.print(WAGMAN_KERNEL_MIN);
  SerialUSB.print('.');
  SerialUSB.println(WAGMAN_KERNEL_SUB);

  SerialUSB.print("time ");
  SerialUSB.println(BUILD_TIME);

  SerialUSB.print("git ");
  SerialUSB.println(BUILD_GIT);

  return 0;
}

/*
Command:
Get Wagman Bootloader Flag

Description:
Gets the bootloader stage boot / media flag for the node controller. The output
indicates whether the node controller will be started in the bootloader stage
and if so, with which boot media.

Examples:
$ wagman-client blf
*/
byte commandBLFlag(byte argc, const char **argv) {
  if (argc == 1) {
    byte mode = Record::getBootloaderNodeController();

    if (mode == 0) {
      SerialUSB.println("off");
    } else if (mode == 1) {
      SerialUSB.println("sd");
    } else if (mode == 2) {
      SerialUSB.println("emmc");
    } else {
      SerialUSB.println("?");
    }
  } else if (argc == 2) {
    if (strcmp(argv[1], "off") == 0) {
      Record::setBootloaderNodeController(0);
    } else if (strcmp(argv[1], "sd") == 0) {
      Record::setBootloaderNodeController(1);
    } else if (strcmp(argv[1], "emmc") == 0) {
      Record::setBootloaderNodeController(2);
    } else {
      SerialUSB.println("invalid mode");
    }
  }

  return 0;
}

/*
Command:
Reset EEPROM

Description:
Requests that the system resets its persistent EEPROM on the *next* reboot.

Examples:
# request a eeprom reset
$ wagman-client eereset
# reset the wagman
$ wagman-client reset
*/
byte commandResetEEPROM(writer &w) {
  Record::clearMagic();
  basicResp(w, COMMAND_EERESET, 0);
  return 0;
}

byte commandSDInfo(byte argc, const char **argv) {
  Sd2Card card;
  SdVolume volume;

  card.init(SPI_HALF_SPEED, SD_CS);

  SerialUSB.print("Card Type : ");

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
      break;
  }

  if (!volume.init(card)) {
    SerialUSB.println(
        "Could not find FAT16/FAT32 partition.\nMake sure you've formatted the "
        "card");
    while (1)
      ;
  }

  SerialUSB.print("Clusters : ");
  SerialUSB.println(volume.clusterCount());
  SerialUSB.print("Blocks x Cluster : ");
  SerialUSB.println(volume.blocksPerCluster());

  SerialUSB.print("Total Blocks : ");
  SerialUSB.println(volume.blocksPerCluster() * volume.clusterCount());
  SerialUSB.println();

  // print the type and size of the first FAT-type volume
  uint32_t volumesize;
  SerialUSB.print("Volume type is:    FAT");
  SerialUSB.println(volume.fatType(), DEC);

  volumesize = volume.blocksPerCluster();  // clusters are collections of blocks
  volumesize *= volume.clusterCount();     // we'll have a lot of clusters
  volumesize /= 2;  // SD card blocks are always 512 bytes (2 blocks are 1KB)
  SerialUSB.print("Volume size (Kb):  ");
  SerialUSB.println(volumesize);
  SerialUSB.print("Volume size (Mb):  ");
  volumesize /= 1024;
  SerialUSB.println(volumesize);
  SerialUSB.print("Volume size (Gb):  ");
  SerialUSB.println((float)volumesize / 1024.0);

  return 0;
}

const int NC_AUTO_DISABLE = 48;
const int MR1 = 30;
const int MR2 = 32;

bool shouldResetAll = false;

byte commandResetAll(byte argc, const char **argv) {
  shouldResetAll = true;
  return 0;
}

bytebuffer<128> msgbuf;

// ah...could even stream this the other way to save on internal data usage...

void processCommand() {
  base64_decoder b64d(msgbuf);
  sensorgram_decoder<64> d(b64d);

  while (d.decode()) {
    base64_encoder b64e(serial_writer);

    switch (d.info.id) {
      case COMMAND_ID: {
        commandID(b64e);
      } break;
      case COMMAND_CU: {
        commandCurrent(b64e);
      } break;
      case COMMAND_HB: {
        commandHeartbeat(b64e);
      } break;
      case COMMAND_BOOTS: {
        commandBoots(b64e);
      } break;
      case COMMAND_FC: {
        commandFailCount(b64e);
      } break;
      case COMMAND_START: {
        int port = d.decode_uint();

        if (!d.err) {
          commandStart(b64e, port);
        } else {
          basicResp(b64e, COMMAND_START, 2);
        }
      } break;
      case COMMAND_STOP: {
        int port = d.decode_uint();
        int after = d.decode_uint();

        if (!d.err) {
          commandStop(b64e, port, after);
        } else {
          basicResp(b64e, COMMAND_STOP, 2);
        }
      } break;
      case COMMAND_UPTIME: {
        commandUptime(b64e);
      } break;
      case COMMAND_EERESET: {
        commandResetEEPROM(b64e);
      } break;
      case COMMAND_RESET: {
        commandReset(b64e);
      } break;
    }

    b64e.close();
    serial_writer.writebyte('\n');
  }
}

void processCommands() {
  int n = SerialUSB.available();

  for (int i = 0; i < n; i++) {
    int c = SerialUSB.read();

    if (c == '\n') {
      processCommand();
      msgbuf.reset();
    } else {
      msgbuf.writebyte(c);
    }
  }
}

void deviceKilled(Device &device) {
  if (meanBootDelta(Record::bootLogs[device.port], 3) < 240) {
    device.setStartDelay(300000);
    Logger::begin("backoff");
    Logger::log("backing off of ");
    Logger::log(device.name);
    Logger::end();
  }
}

volatile bool detectHB[5] = {false, false, false, false, false};

void checkSerialHB() {
  byte heartbeatBuffer[33];

  if (Serial1.available() > 0) {
    int n = Serial1.readBytesUntil('\n', heartbeatBuffer, 32);
    heartbeatBuffer[n] = 0;

    if (HasPrefix((const char *)heartbeatBuffer, "hello")) {
      detectHB[0] = true;
    }
  }

  if (Serial2.available() > 0) {
    int n = Serial2.readBytesUntil('\n', heartbeatBuffer, 32);
    heartbeatBuffer[n] = 0;

    if (HasPrefix((const char *)heartbeatBuffer, "hello")) {
      detectHB[1] = true;
    }
  }
}

volatile int lastHBPinState[5] = {LOW, LOW, LOW, LOW, LOW};

const int CS_HB_PIN = 28;

void checkPinHB() {
  int state = digitalRead(CS_HB_PIN);
  detectHB[2] = state != lastHBPinState[2];
  lastHBPinState[2] = state;

  detectHB[3] = false;
  detectHB[4] = false;
}

// extern MockEEPROM<4096> EEPROM;
extern ExternalEEPROM EEPROM;

void setup() {
  watchdogReset();
  watchdogEnable(16000);
  watchdogReset();

  Wire.begin();

  SerialUSB.begin(57600);
  SerialUSB.setTimeout(100);

  Serial1.begin(115200);
  Serial1.setTimeout(100);

  Serial2.begin(115200);
  Serial2.setTimeout(100);

  Serial3.begin(115200);
  Serial3.setTimeout(100);

  watchdogReset();

  if (!Record::initialized()) {
    Record::init();
    Wagman::init();

    watchdogReset();

    for (byte i = 0; i < 8; i++) {
      Wagman::setLEDs(HIGH);
      delay(40);
      Wagman::setLEDs(LOW);
      delay(40);
    }
  } else {
    Wagman::init();
  }

  Wagman::getTime(setupTime);
  Record::setLastBootTime(setupTime);
  Record::incrementBootCount();

  watchdogReset();
  setupDevices();
  deviceWantsStart = 0;

  bufferSize = 0;
  startTimer.reset();
  statusTimer.reset();

  shouldResetSystem = false;
  shouldResetTimeout = 0;

  if (Wagman::Clock.get() < BUILD_TIME) {
    Wagman::Clock.set(BUILD_TIME);
  }

  watchdogReset();

  pinMode(CS_HB_PIN, INPUT_PULLUP);
  Timer3.attachInterrupt(checkPinHB).setFrequency(10).start();

  devices[0].start();
}

void setupDevices() {
  devices[0].name = "nc";
  devices[0].port = 0;
  devices[0].bootSelector = 0;
  devices[0].primaryMedia = MEDIA_SD;
  devices[0].secondaryMedia = MEDIA_EMMC;
  devices[0].watchHeartbeat = true;
  devices[0].watchCurrent = false;

  devices[1].name = "gn";
  devices[1].port = 1;
  devices[1].bootSelector = 1;
  devices[1].primaryMedia = MEDIA_SD;
  devices[1].secondaryMedia = MEDIA_EMMC;
  devices[1].watchHeartbeat = true;
  devices[1].watchCurrent = false;

  devices[2].name = "cs";
  devices[2].port = 2;
  devices[2].primaryMedia = MEDIA_SD;
  devices[2].secondaryMedia = MEDIA_EMMC;
  devices[2].watchHeartbeat = true;
  devices[2].watchCurrent = false;

  devices[3].name = "x1";
  devices[3].port = 3;
  devices[3].watchHeartbeat = false;
  devices[3].watchCurrent = false;

  devices[4].name = "x2";
  devices[4].port = 4;
  devices[4].watchHeartbeat = false;
  devices[4].watchCurrent = false;

  for (byte i = 0; i < DEVICE_COUNT; i++) {
    devices[i].init();
  }
}

void checkSensors() {
  checkCurrentSensors();
  checkThermistors();
}

void checkCurrentSensors() {
  for (byte i = 0; i < DEVICE_COUNT; i++) {
    byte attempt;

    if (Record::getPortCurrentSensorHealth(i) >= 5) {
      // Printf("post: current sensor %d too many faults\n", i);
      Logger::begin("post");
      Logger::log("current sensor ");
      Logger::log(i);
      Logger::log(" too many faults");
      Logger::end();
      continue;
    }

    for (attempt = 0; attempt < 3; attempt++) {
      Record::setPortCurrentSensorHealth(
          i, Record::getPortCurrentSensorHealth(i) + 1);
      delay(20);
      unsigned int value = Wagman::getCurrent(i);
      delay(50);
      Record::setPortCurrentSensorHealth(
          i, Record::getPortCurrentSensorHealth(i) - 1);
      delay(20);

      if (value <= 5000) break;
    }

    if (attempt == 3) {
      Logger::begin("post");
      Logger::log("current sensor ");
      Logger::log(i);
      Logger::log(" out of range.");
      Logger::end();
    }
  }
}

void checkThermistors() {
  for (byte i = 0; i < DEVICE_COUNT; i++) {
    byte attempt;

    if (Record::getThermistorSensorHealth(i) >= 5) {
      Logger::begin("post");
      Logger::log("thermistor ");
      Logger::log(i);
      Logger::log(" too many faults");
      Logger::end();
      continue;
    }

    for (attempt = 0; attempt < 3; attempt++) {
      Record::setThermistorSensorHealth(
          i, Record::getThermistorSensorHealth(i) + 1);
      delay(20);
      unsigned int value = Wagman::getThermistor(i);
      delay(50);
      Record::setThermistorSensorHealth(
          i, Record::getThermistorSensorHealth(i) - 1);
      delay(20);

      if (value <= 10000) break;
    }

    if (attempt == 3) {
      Logger::begin("post");
      Logger::log("thermistor ");
      Logger::log(i);
      Logger::log(" out of range.");
      Logger::end();
    }
  }
}

unsigned long meanBootDelta(const Record::BootLog &bootLog, byte maxSamples) {
  byte count = min(maxSamples, bootLog.getCount());
  unsigned long total = 0;

  for (byte i = 1; i < count; i++) {
    total += bootLog.getEntry(i) - bootLog.getEntry(i - 1);
  }

  return total / count;
}

void showBootLog(const Record::BootLog &bootLog) {
  Logger::begin("bootlog");
  for (byte i = 0; i < bootLog.getCount(); i++) {
    Logger::log(" ");
    Logger::log(bootLog.getEntry(i));
  }
  Logger::end();

  if (bootLog.getCount() > 1) {
    Logger::begin("bootdt");
    for (byte i = 1; i < min(4, bootLog.getCount()); i++) {
      unsigned long bootdt = bootLog.getEntry(i) - bootLog.getEntry(i - 1);
      Logger::log(" ");
      Logger::log(bootdt);
    }

    if (meanBootDelta(bootLog, 3) < 200) {
      Logger::log(" !");
    }

    Logger::end();
  }
}

// should probably tie current -> current LEDs
// can also blink them as needed.

void startNextDevice() {
  // if we've asked for a specific device, start that device.
  if (Wagman::validPort(deviceWantsStart)) {
    startTimer.reset();
    devices[deviceWantsStart].start();
    deviceWantsStart = 255;
    return;
  }

  // NOTE We should have already started the NC during setup. But, just in
  // case, we do it again here.
  if (startTimer.exceeds(60000)) {
    for (byte i = 0; i < DEVICE_COUNT; i++) {
      if (devices[i].canStart()) {  // include !started in canStart() call.
        startTimer.reset();
        devices[i].start();
        showBootLog(Record::bootLogs[i]);
        break;
      }
    }
  }
}

bool HasPrefix(const char *s, const char *prefix) {
  while (*prefix) {
    if (*s != *prefix) {
      return false;
    }

    s++;
    prefix++;
  }

  return true;
}

void doResetBlink() {
  for (int i = 0; i < 3; i++) {
    Wagman::setLED(i, HIGH);
  }

  delay(200);

  for (int i = 0; i < 3; i++) {
    Wagman::setLED(i, LOW);
  }

  delay(200);
}

void doResetAll() {
  for (int i = 0; i < 6; i++) {
    watchdogReset();

    Logger::begin("nc");
    Logger::log("stopping");
    Logger::end();

    Logger::begin("gn");
    Logger::log("stopping");
    Logger::end();

    doResetBlink();

    delay(10000);
  }

  pinMode(NC_AUTO_DISABLE, OUTPUT);

  for (int i = 0; i < 5; i++) {
    watchdogReset();
    Wagman::setRelay(i, false);
    doResetBlink();
    delay(5000);
  }

  for (int i = 0; i < 20; i++) {
    watchdogReset();
    doResetBlink();
    delay(1000);
  }

  pinMode(MR1, OUTPUT);
  pinMode(MR2, OUTPUT);
  digitalWrite(MR1, LOW);
  digitalWrite(MR2, LOW);

  for (;;) {
    doResetBlink();
  }
}

void loop() {
  watchdogReset();

  // don't bother starting any new devices once we've decided to reset
  if (!shouldResetSystem) {
    startNextDevice();
  }

  watchdogReset();

  for (byte i = 0; i < DEVICE_COUNT; i++) {
    devices[i].update();
  }

  processCommands();

  if (shouldResetAll) {
    doResetAll();
  }

  if (statusTimer.exceeds(60000)) {
    statusTimer.reset();
    watchdogReset();
    logStatus();
  }

  if (shouldResetSystem && shouldResetTimer.exceeds(shouldResetTimeout)) {
    resetSystem();
  }

  watchdogReset();
  checkSerialHB();
  watchdogReset();

  bool devicePowered[5];

  unsigned int currentLevel[5];
  const unsigned int currentBaseline[5] = {200, 200, 150, 200, 200};

  for (int i = 0; i < 5; i++) {
    currentLevel[i] = Wagman::getCurrent(i);
  }

  for (int i = 0; i < 5; i++) {
    devicePowered[i] = currentLevel[i] >= currentBaseline[i];
  }

  bool shouldBlink[5] = {false, false, false, false, false};

  for (int i = 0; i < 5; i++) {
    if (detectHB[i]) {
      detectHB[i] = false;
      shouldBlink[i] = true;
      heartbeatCounters[i]++;
    }
  }

  Wagman::setLED(0, LOW);

  for (int i = 0; i < 5; i++) {
    if (shouldBlink[i]) {
      Wagman::setLED(i + 1, LOW);
    }
  }

  delay(50);

  Wagman::setLED(0, HIGH);

  for (int i = 0; i < 5; i++) {
    if (shouldBlink[i]) {
      Wagman::setLED(i + 1, HIGH);
    }
  }

  for (int i = 0; i < 5; i++) {
    Wagman::setLED(i + 1, devicePowered[i] ? HIGH : LOW);
  }
}

void resetSystem() {
  watchdogReset();

  for (;;) {
    for (byte i = 0; i < 5; i++) {
      Wagman::setLEDs(HIGH);
      delay(80);
      Wagman::setLEDs(LOW);
      delay(80);
    }

    Wagman::setLEDs(HIGH);
    delay(200);
    Wagman::setLEDs(LOW);
    delay(200);
  }
}

void logStatus() {
  SerialUSB.println("testing async logging");

  // send batched status sensorgrams
  base64_encoder b64e(serial_writer);
  commandID(b64e);
  commandCurrent(b64e);
  commandFailCount(b64e);
  commandVoltage(b64e);
  commandThermistor(b64e);
  b64e.close();
  serial_writer.writebyte('\n');

  // unsigned int rawTemperature, rawHumidity;
  // float temperature, humidity;
  // bool temperatureOK = Wagman::getTemperature(&rawTemperature, &temperature);
  // bool humidityOK = Wagman::getHumidity(&rawHumidity, &humidity);

  // Logger::begin("temperature");
  // Logger::log(rawTemperature);
  // Logger::log(" ");
  // Logger::log(temperature);

  // if (!temperatureOK) {
  //   Logger::log(" err");
  // }

  // Logger::end();

  // Logger::begin("humidity");
  // Logger::log(rawHumidity);
  // Logger::log(" ");
  // Logger::log(humidity);

  // if (!humidityOK) {
  //   Logger::log(" err");
  // }

  // Logger::end();

  // unsigned int light;
  // bool lightOK = Wagman::getLight(&light);

  // Logger::begin("light");
  // Logger::log(light);

  // if (!lightOK) {
  //   Logger::log(" err");
  // }

  // Logger::end();

  // Logger::end();

  // Logger::begin("enabled");

  // for (byte i = 0; i < 5; i++) {
  //   Logger::log(Record::getDeviceEnabled(i));
  //   Logger::log(' ');
  // }

  // Logger::end();

  // Logger::begin("state");

  // for (byte i = 0; i < 5; i++) {
  //   Logger::log(devices[i].getState());
  //   Logger::log(' ');
  // }

  // Logger::end();

  // delay(50);

  // Logger::begin("media");

  // for (byte i = 0; i < 2; i++) {
  //   byte bootMedia = devices[i].getNextBootMedia();

  //   if (bootMedia == MEDIA_SD) {
  //     Logger::log("sd ");
  //   } else if (bootMedia == MEDIA_EMMC) {
  //     Logger::log("emmc ");
  //   } else {
  //     Logger::log("? ");
  //   }
  // }

  // Logger::end();
}
