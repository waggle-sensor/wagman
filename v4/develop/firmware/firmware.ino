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

static time_t setupTime;

template <class streamT>
struct stream_writer : public writer {
  streamT &stream;

  stream_writer(streamT &stream) : stream(stream) {}

  int write(const byte *s, int n) {
    int maxn = stream.availableForWrite();

    if (n > maxn) {
      n = maxn;
    }

    return stream.write(s, n);
  }
};

#define REQ_WAGMAN_ID 0xc000
#define REQ_WAGMAN_CU 0xc001
#define REQ_WAGMAN_HB 0xc002
#define REQ_WAGMAN_TH 0xc003
#define REQ_WAGMAN_UPTIME 0xc004
#define REQ_WAGMAN_BOOTS 0xc005
#define REQ_WAGMAN_FC 0xc006
#define REQ_WAGMAN_START 0xc007
#define REQ_WAGMAN_STOP 0xc008
#define REQ_WAGMAN_RESET 0xc009
#define REQ_WAGMAN_EERESET 0xc00a
#define REQ_WAGMAN_DEVICE_STATE 0xc00b
#define REQ_WAGMAN_DEVICE_ENABLE 0xc00c
#define REQ_WAGMAN_GET_MEDIA_SELECT 0xc00d
#define REQ_WAGMAN_SET_MEDIA_SELECT 0xc00e

#define PUB_WAGMAN_ID 0xff1a
#define PUB_WAGMAN_CU 0xff06
#define PUB_WAGMAN_HB 0xff09
#define PUB_WAGMAN_BOOTS 0xff1a
#define PUB_WAGMAN_UPTIME 0xff14
#define PUB_WAGMAN_FC 0xff05
#define PUB_WAGMAN_START 0xff0b
#define PUB_WAGMAN_STOP 0xff0a
#define PUB_WAGMAN_EERESET 0xc00a
#define PUB_WAGMAN_RESET 0xc009
#define PUB_WAGMAN_PING 0xff1e
#define PUB_WAGMAN_VOLTAGE 0xff08
#define PUB_WAGMAN_TH 0xff10
#define PUB_WAGMAN_DEVICE_STATE 0xff1f
#define PUB_WAGMAN_DEVICE_ENABLE 0xff20
#define PUB_WAGMAN_GET_MEDIA_SELECT 0xff21
#define PUB_WAGMAN_SET_MEDIA_SELECT 0xff22

template <class T>
void basicResp(writer &w, int id, int sub_id, T value) {
  sensorgram_encoder<64> e(w);
  e.info.id = id;
  e.info.sub_id = sub_id;
  e.info.inst = 0;
  e.info.source_id = 1;
  e.info.source_inst = 0;
  e.encode_uint(value);
  e.encode();
}

void basicResp(writer &w, int id, int sub_id, const byte *value, int n) {
  sensorgram_encoder<64> e(w);
  e.info.id = id;
  e.info.sub_id = sub_id;
  e.info.inst = 0;
  e.info.source_id = 1;
  e.info.source_inst = 0;
  e.encode_bytes(value, n);
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
int commandPingMain(int device) {
  int ok = Wagman::validPort(device);

  if (ok) {
    Wagman::setLED(device + 1, LOW);
    devices[device].sendExternalHeartbeat();
    delay(50);
    Wagman::setLED(device + 1, HIGH);
  }

  return ok;
}

void commandPing(writer &w, int sub_id) {
  basicResp(w, PUB_WAGMAN_PING, sub_id, commandPingMain(sub_id - 1));
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
int commandStartMain(int port) {
  int ok = Wagman::validPort(port);

  if (ok) {
    deviceWantsStart = port;
  }

  return ok;
}

void commandStart(writer &w, int sub_id) {
  basicResp(w, PUB_WAGMAN_START, sub_id, commandStartMain(sub_id - 1));
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
int commandStopMain(int port, int after) {
  int ok = Wagman::validPort(port);

  if (ok) {
    devices[port].setStopTimeout((unsigned long)after * 1000);
    devices[port].stop();
  }

  return ok;
}

void commandStop(writer &w, int sub_id, int after) {
  basicResp(w, PUB_WAGMAN_STOP, sub_id, commandStopMain(sub_id - 1, after));
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
void commandReset(writer &w) {
  shouldResetSystem = true;
  shouldResetTimer.reset();
  shouldResetTimeout = 0;
  basicResp(w, PUB_WAGMAN_RESET, 1, 1);
}

/*
Command:
Get Wagman ID

Description:
Gets the onboard Wagman ID.

Examples:
$ wagman-client id
*/
void commandID(writer &w) {
  byte wagman_id[8];
  Wagman::getID(wagman_id);
  basicResp(w, PUB_WAGMAN_ID, 1, wagman_id, 8);
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
// void commandDate(byte argc, const char **argv) {
//   if (argc != 1 && argc != 7) return ERROR_INVALID_ARGC;

//   if (argc == 1) {
//     DateTime dt;
//     Wagman::getDateTime(dt);
//     // printDate(dt);
//     SerialUSB.println();
//   } else if (argc == 7) {
//     DateTime dt;

//     dt.year = atoi(argv[1]);
//     dt.month = atoi(argv[2]);
//     dt.day = atoi(argv[3]);
//     dt.hour = atoi(argv[4]);
//     dt.minute = atoi(argv[5]);
//     dt.second = atoi(argv[6]);

//     Wagman::setDateTime(dt);
//   }

//   return 0;
// }

/*
Command:
Get Current Values

Description:
Gets the current from the system and all devices. The outputs are formatted as:
System Device0 Device1 ... Device4

Examples:
$ wagman-client cu
*/
unsigned int commandCurrentMain(int port) {
  // get wagman current
  if (port == 0) {
    return Wagman::getCurrent();
  }

  // get attached device current
  if (Wagman::validPort(port - 1)) {
    return Wagman::getCurrent(port - 1);
  }

  return 0;
}

void commandCurrent(writer &w, int sub_id) {
  basicResp(w, PUB_WAGMAN_CU, sub_id, commandCurrentMain(sub_id - 1));
}

unsigned int commandVoltageMain(int port) {
  if (Wagman::validPort(port)) {
    return Wagman::getVoltage(port);
  }

  return 0;
}

void commandVoltage(writer &w, int sub_id) {
  basicResp(w, PUB_WAGMAN_VOLTAGE, sub_id, commandVoltageMain(sub_id - 1));
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
unsigned int commandHeartbeatMain(int port) {
  if (Wagman::validPort(port)) {
    return devices[port].timeSinceHeartbeat();
  }

  return 0;
}

void commandHeartbeat(writer &w, int sub_id) {
  basicResp(w, PUB_WAGMAN_HB, sub_id, commandHeartbeatMain(sub_id - 1));
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
unsigned int commandFailCountMain(int port) {
  if (Wagman::validPort(port)) {
    return Record::getBootFailures(port);
  }

  return 0;
}

void commandFailCount(writer &w, int sub_id) {
  basicResp(w, PUB_WAGMAN_FC, sub_id, commandFailCountMain(sub_id));
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
int commandThermistorMain(int port) {
  if (Wagman::validPort(port)) {
    return Wagman::getThermistor(port);
  }

  return 0;
}

void commandThermistor(writer &w, int sub_id) {
  basicResp(w, PUB_WAGMAN_TH, sub_id, commandThermistorMain(sub_id));
}

/*
Command:
Get Environment Sensor Values

Description:
Gets the onboard temperature and humidity sensor values.

Examples:
$ wagman-client env
*/
void commandEnvironment(writer &w) {
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
int commandSetMediaSelectMain(int port, int media) {
  if (port != 0 && port != 1) {
    return 0;
  }

  if (media == 0) {
    devices[port].setNextBootMedia(MEDIA_SD);
  } else if (media == 1) {
    devices[port].setNextBootMedia(MEDIA_EMMC);
  } else {
    return 0;
  }

  return 1;
}

void commandSetMediaSelect(writer &w, int sub_id, int media) {
  basicResp(w, PUB_WAGMAN_SET_MEDIA_SELECT, sub_id,
            commandSetMediaSelectMain(sub_id - 1, media));
}

int commandGetMediaSelectMain(int port) {
  if (port == 0 || port == 1) {
    return devices[port].getNextBootMedia();
  }

  return 255;
}

void commandGetMediaSelect(writer &w, int sub_id) {
  basicResp(w, PUB_WAGMAN_GET_MEDIA_SELECT, sub_id,
            commandGetMediaSelectMain(sub_id - 1));
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
void commandUptime(writer &w) {
  basicResp(w, PUB_WAGMAN_UPTIME, 1, millis() / 1000);
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
int commandEnableMain(int port) {
  int ok = Wagman::validPort(port);

  if (ok) {
    devices[port].enable();
  }

  return ok;
}

void commandEnable(writer &w, int sub_id) {
  basicResp(w, PUB_WAGMAN_DEVICE_ENABLE, sub_id, commandEnableMain(sub_id - 1));
}

int commandStateMain(int port) {
  if (Wagman::validPort(port)) {
    return devices[port].getState();
  }

  return 0;
}

void commandState(writer &w, int sub_id) {
  basicResp(w, PUB_WAGMAN_DEVICE_STATE, sub_id, commandStateMain(sub_id - 1));
}

/*
Command:
Get Wagman Boot Count

Description:
Gets the number of times the system has booted.

Examples:
$ wagman-client boots
*/
void commandBoots(writer &w) {
  unsigned long count;
  Record::getBootCount(count);
  basicResp(w, PUB_WAGMAN_BOOTS, 1, count);
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
void commandResetEEPROM(writer &w) {
  Record::clearMagic();
  basicResp(w, PUB_WAGMAN_EERESET, 1, 1);
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
bytebuffer<128> msgbuf0;
bytebuffer<128> msgbuf1;
bytebuffer<128> msgbuf2;
bytebuffer<128> msgbuf3;

template <class bufferT, class writerT>
void processCommand(bufferT &buffer, writerT &wout, bool isadmin, int port) {
  base64_decoder b64d(buffer);
  sensorgram_decoder<64> d(b64d);

  while (d.decode()) {
    base64_encoder b64e(wout);

    switch (d.info.id) {
      case REQ_WAGMAN_ID: {
        commandID(b64e);
      } break;
      case REQ_WAGMAN_CU: {
        commandCurrent(b64e, d.info.sub_id);
      } break;
      case REQ_WAGMAN_HB: {
        commandHeartbeat(b64e, d.info.sub_id);
      } break;
      case REQ_WAGMAN_BOOTS: {
        commandBoots(b64e);
      } break;
      case REQ_WAGMAN_FC: {
        commandFailCount(b64e, d.info.sub_id);
      } break;
      case REQ_WAGMAN_TH: {
        commandThermistor(b64e, d.info.sub_id);
      } break;
      case REQ_WAGMAN_START: {
        if (isadmin) {
          commandStart(b64e, d.info.sub_id);
        }
      } break;
      case REQ_WAGMAN_STOP: {
        if (isadmin) {
          int after = d.decode_uint();

          if (!d.err) {
            commandStop(b64e, d.info.sub_id, after);
          } else {
            basicResp(b64e, PUB_WAGMAN_STOP, d.info.sub_id, 0);
          }
        }
      } break;
      case REQ_WAGMAN_DEVICE_ENABLE: {
        if (isadmin) {
          commandEnable(b64e, d.info.sub_id);
        }
      } break;
      case REQ_WAGMAN_DEVICE_STATE: {
        commandState(b64e, d.info.sub_id);
      } break;
      case REQ_WAGMAN_GET_MEDIA_SELECT: {
        commandGetMediaSelect(b64e, d.info.sub_id);
      } break;
      case REQ_WAGMAN_SET_MEDIA_SELECT: {
        if (isadmin) {
          int media = d.decode_uint();

          if (!d.err) {
            commandSetMediaSelect(b64e, d.info.sub_id, media);
          } else {
            basicResp(b64e, PUB_WAGMAN_SET_MEDIA_SELECT, d.info.sub_id, 0);
          }
        }
      } break;
      case REQ_WAGMAN_UPTIME: {
        commandUptime(b64e);
      } break;
      case REQ_WAGMAN_EERESET: {
        if (isadmin) {
          commandResetEEPROM(b64e);
        }
      } break;
      case REQ_WAGMAN_RESET: {
        if (isadmin) {
          commandReset(b64e);
        }
      } break;
      case PUB_WAGMAN_PING: {
        if (isadmin || (port == (d.info.sub_id - 1))) {
          commandPing(b64e, d.info.sub_id);
        }
      } break;
    }

    b64e.close();
    wout.writebyte('\n');
  }
}

template <class streamT, class bufferT>
void processCommands(streamT &stream, bufferT &buffer, bool isadmin, int port) {
  int n = stream.available();

  for (int i = 0; i < n; i++) {
    int c = stream.read();

    if (c == '\n') {
      stream_writer<streamT> sw(stream);
      processCommand(buffer, sw, isadmin, port);
      buffer.reset();
    } else {
      buffer.writebyte(c);
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

volatile int lastHBPinState[5] = {LOW, LOW, LOW, LOW, LOW};

const int CS_HB_PIN = 28;

void checkPinHB() {
  int state = digitalRead(CS_HB_PIN);
  detectHB[2] = state != lastHBPinState[2];
  lastHBPinState[2] = state;

  detectHB[3] = false;
  detectHB[4] = false;
}

extern ExternalEEPROM EEPROM;

void setup() {
  watchdogReset();
  watchdogEnable(16000);
  watchdogReset();

  Wire.begin();

  SerialUSB.begin(115200);
  SerialUSB.setTimeout(100);

  Serial.begin(115200);
  Serial.setTimeout(100);

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
        // showBootLog(Record::bootLogs[i]);
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

  processCommands(SerialUSB, msgbuf, true, 0);
  processCommands(Serial1, msgbuf1, true, 0);
  processCommands(Serial2, msgbuf2, false, 1);
  processCommands(Serial3, msgbuf3, false, 2);
  processCommands(Serial, msgbuf0, false, 0);

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
  // send batched status sensorgrams
  stream_writer<typeof(SerialUSB)> w(SerialUSB);
  base64_encoder b64e(w);
  commandID(b64e);

  for (int port = 1; port <= 6; port++) {
    commandCurrent(b64e, port);
  }

  for (int port = 1; port <= 5; port++) {
    commandFailCount(b64e, port);
  }

  for (int port = 1; port <= 5; port++) {
    commandVoltage(b64e, port);
  }

  for (int port = 1; port <= 5; port++) {
    commandThermistor(b64e, port);
  }

  b64e.close();
  w.writebyte('\n');
}
