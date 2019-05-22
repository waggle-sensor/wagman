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

Command commands[] = {
    {"rtc", commandRTC},
    {"ping", commandPing},
    {"start", commandStart},
    {"stop", commandStop},
    {"reset", commandReset},
    {"id", commandID},
    {"cu", commandCurrent},
    {"hb", commandHeartbeat},
    {"env", commandEnvironment},
    {"bs", commandBootMedia},
    {"th", commandThermistor},
    {"date", commandDate},
    {"bf", commandBootFlags},
    {"fc", commandFailCount},
    {"up", commandUptime},
    {"enable", commandEnable},
    {"disable", commandDisable},
    {"watch", commandWatch},
    {"log", commandLog},
    {"eereset", commandResetEEPROM},
    {"boots", commandBoots},
    {"ver", commandVersion},
    {"blf", commandBLFlag},
    {"sdinfo", commandSDInfo},
    {"state", commandState},
    {"resetall", commandResetAll},
    {NULL, NULL},
};

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

/*
Command:
Get RTC

Description:
Gets the milliseconds since epoch from the RTC.

Examples:
$ wagman-client rtc
*/
byte commandRTC(byte argc, const char **argv) {
  SerialUSB.println(Wagman::Clock.get());
  return 0;
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
byte commandPing(byte argc, const char **argv) {
  if (argc != 2) return ERROR_INVALID_ARGC;

  byte port = atoi(argv[1]);

  if (!Wagman::validPort(port)) return ERROR_INVALID_PORT;

  devices[port].sendExternalHeartbeat();

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
byte commandStart(byte argc, const char **argv) {
  if (argc != 2) return ERROR_INVALID_ARGC;

  byte port = atoi(argv[1]);

  if (!Wagman::validPort(port)) return ERROR_INVALID_PORT;

  deviceWantsStart = port;

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
byte commandStop(byte argc, const char **argv) {
  if (argc <= 1) return ERROR_INVALID_ARGC;

  byte port = atoi(argv[1]);

  if (!Wagman::validPort(port)) return ERROR_INVALID_PORT;

  if (argc == 2) {
    devices[port].setStopTimeout(60000);
    devices[port].stop();
  } else if (argc == 3) {
    devices[port].setStopTimeout((unsigned long)atoi(argv[2]) * 1000);
    devices[port].stop();
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
byte commandReset(byte argc, const char **argv) {
  shouldResetSystem = true;
  shouldResetTimer.reset();

  if (argc == 1) {
    shouldResetTimeout = 0;
  } else if (argc == 2) {
    shouldResetTimeout = (unsigned long)atoi(argv[1]) * 1000;
  }

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
byte commandID(__attribute__((unused)) byte argc,
               __attribute__((unused)) const char **argv) {
  byte id[8];

  Wagman::getID(id);
  printID(id);
  SerialUSB.println();

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
byte commandCurrent(__attribute__((unused)) byte argc,
                    __attribute__((unused)) const char **argv) {
  SerialUSB.print(Wagman::getCurrent());

  for (byte i = 0; i < DEVICE_COUNT; i++) {
    SerialUSB.print(' ');
    SerialUSB.print(Wagman::getCurrent(i));
  }

  SerialUSB.println();

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
byte commandHeartbeat(__attribute__((unused)) byte argc,
                      __attribute__((unused)) const char **argv) {
  for (byte i = 0; i < DEVICE_COUNT; i++) {
    SerialUSB.println(devices[i].timeSinceHeartbeat());
  }

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
byte commandFailCount(__attribute__((unused)) byte argc,
                      __attribute__((unused)) const char **argv) {
  for (byte i = 0; i < DEVICE_COUNT; i++) {
    SerialUSB.println(Record::getBootFailures(i));
  }

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
byte commandThermistor(__attribute__((unused)) byte argc,
                       __attribute__((unused)) const char **argv) {
  for (byte i = 0; i < DEVICE_COUNT; i++) {
    SerialUSB.println(Wagman::getThermistor(i));
  }

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
byte commandEnvironment(__attribute__((unused)) byte argc,
                        __attribute__((unused)) const char **argv) {
  unsigned int rawTemperature, rawHumidity, rawLight;
  float temperature, humidity;
  bool temperatureOK = Wagman::getTemperature(&rawTemperature, &temperature);
  bool humidityOK = Wagman::getHumidity(&rawHumidity, &humidity);
  bool lightOK = Wagman::getLight(&rawLight);

  SerialUSB.print("temperature ");
  SerialUSB.print(rawTemperature);
  SerialUSB.print(" ");
  SerialUSB.print(temperature);

  if (!temperatureOK) {
    SerialUSB.println(" err");
  }

  SerialUSB.println();

  SerialUSB.print("humidity ");
  SerialUSB.print(rawHumidity);
  SerialUSB.print(" ");
  SerialUSB.print(humidity);

  if (!humidityOK) {
    SerialUSB.println(" err");
  }

  SerialUSB.println();

  SerialUSB.print("light ");
  SerialUSB.print(rawLight);

  if (!humidityOK) {
    SerialUSB.println(" err");
  }

  SerialUSB.println();

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
byte commandBootMedia(byte argc, const char **argv) {
  if (argc != 2 && argc != 3) return ERROR_INVALID_ARGC;

  byte index = atoi(argv[1]);

  if (!Wagman::validPort(index)) return ERROR_INVALID_PORT;

  if (argc == 3) {
    if (strcmp(argv[2], "sd") == 0) {
      devices[index].setNextBootMedia(MEDIA_SD);
      SerialUSB.println("set sd");
    } else if (strcmp(argv[2], "emmc") == 0) {
      devices[index].setNextBootMedia(MEDIA_EMMC);
      SerialUSB.println("set emmc");
    } else {
      SerialUSB.println("invalid media");
    }
  } else if (argc == 2) {
    byte bootMedia = devices[index].getNextBootMedia();

    if (bootMedia == MEDIA_SD) {
      SerialUSB.println("sd");
    } else if (bootMedia == MEDIA_EMMC) {
      SerialUSB.println("emmc");
    } else {
      SerialUSB.println("invalid media");
    }
  }

  return 0;
}

byte commandLog(byte argc, const char **argv) {
  if (argc == 2) {
    if (strcmp(argv[1], "on") == 0) {
      logging = true;
    } else if (strcmp(argv[1], "off") == 0) {
      logging = false;
    }
  }

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
byte commandBootFlags(__attribute__((unused)) byte argc,
                      __attribute__((unused)) const char **argv) {
  if (bootflags & _BV(WDRF)) SerialUSB.println("WDRF");
  if (bootflags & _BV(BORF)) SerialUSB.println("BORF");
  if (bootflags & _BV(EXTRF)) SerialUSB.println("EXTRF");
  if (bootflags & _BV(PORF)) SerialUSB.println("PORF");
  return 0;
}

/*
Command:
Get Uptime

Description:
Gets the system uptime in seconds.

Examples:
$ wagman-client up
*/
byte commandUptime(__attribute__((unused)) byte argc,
                   __attribute__((unused)) const char **argv) {
  unsigned long uptimeSeconds = millis() / 1000;
  SerialUSB.println(uptimeSeconds);
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
byte commandEnable(byte argc, const char **argv) {
  for (byte i = 1; i < argc; i++) {
    byte port = atoi(argv[i]);

    if (!Wagman::validPort(port)) {
      continue;
    }

    devices[port].enable();
  }

  return 0;
}

byte commandState(byte argc, const char **argv) {
  for (byte i = 0; i < 5; i++) {
    SerialUSB.print(devices[i].getState());
    SerialUSB.print(' ');
  }

  SerialUSB.println();

  return 0;
}

/*
Command:
Disable Device

Description:
Disables a device from being powered on automatically. Note that this doesn't
immediately stop a device.

Examples:
# disable the coresense
$ wagman-client disable 2
*/
byte commandDisable(byte argc, const char **argv) {
  for (byte i = 1; i < argc; i++) {
    byte port = atoi(argv[i]);

    if (!Wagman::validPort(port)) {
      continue;
    }

    devices[port].disable();
  }

  return 0;
}

/*
Command:
Change Device Timeout Behavior

Description:
Change the timeout behavior for the heartbeat and current. Currently, only
heartbeat timeouts are supported.

Examples:
# disable watching the coresense heartbeat
$ wagman-client watch 2 hb f

# enable watching the coresense heartbeat
$ wagman-client watch 2 hb t
*/
byte commandWatch(byte argc, const char **argv) {
  //    if (argc != 4)
  //        return ERROR_USAGE;

  if (argc == 4) {
    byte index = atoi(argv[1]);
    if (Wagman::validPort(index)) {
      bool mode = strcmp(argv[3], "t") == 0;

      if (strcmp(argv[2], "hb") == 0) {
        devices[index].watchHeartbeat = mode;
      } else if (strcmp(argv[2], "cu")) {
        devices[index].watchCurrent = mode;
      }
    }
  }

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
byte commandBoots(byte argc, const char **argv) {
  unsigned long count;
  Record::getBootCount(count);
  SerialUSB.println(count);
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
byte commandResetEEPROM(byte argc, const char **argv) {
  Record::clearMagic();
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

void executeCommand(const char *sid, byte argc, const char **argv) {
  byte (*func)(byte, const char **) = NULL;

  // search for command and execute if found
  for (byte i = 0; commands[i].name != NULL; i++) {
    if (strcmp(commands[i].name, argv[0]) == 0) {
      func = commands[i].func;
      break;
    }
  }

  // marks the beginning of a response packet.
  SerialUSB.print("<<<- sid=");
  SerialUSB.print(sid);
  SerialUSB.print(' ');
  SerialUSB.println(argv[0]);

  if (func != NULL) {
    func(argc, argv);
  } else {
    SerialUSB.println("invalid command");
  }

  // marks the end of a response packet.
  SerialUSB.println("->>>");
}

void processCommand() {
  byte argc = 0;
  const char *argv[MAX_ARGC];

  char *s = buffer;

  while (argc < MAX_ARGC) {
    // skip whitespace
    while (isspace(*s)) {
      s++;
    }

    // check if next character is an argument
    if (isgraph(*s)) {
      // mark start of argument.
      argv[argc++] = s;

      // scan remainder of argument.
      while (isgraph(*s)) {
        s++;
      }
    }

    // end of buffer?
    if (*s == '\0') {
      break;
    }

    // mark end of argument.
    *s++ = '\0';
  }

  if (argc > 0) {
    // this sid case was suggested by wolfgang to help identify the source of
    // certain messages.
    if (argv[0][0] == '@') {
      executeCommand(argv[0] + 1, argc - 1, argv + 1);
    } else {
      executeCommand("0", argc, argv);
    }
  }
}

void processCommands() {
  byte dataread = 0;  // using this instead of a timer to reduce delay in
                      // processing byte stream

  while (SerialUSB.available() > 0 && dataread < 240) {
    char c = SerialUSB.read();

    buffer[bufferSize++] = c;
    dataread++;

    if (bufferSize >= BUFFER_SIZE) {  // exceeds buffer! dangerous!
      bufferSize = 0;

      // flush remainder of line.
      while (SerialUSB.available() > 0 && dataread < 240) {
        if (SerialUSB.read() == '\n') break;
        dataread++;
      }
    } else if (c == '\n') {
      buffer[bufferSize] = '\0';
      bufferSize = 0;
      processCommand();
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

  // NOTE We should have already started the NC during setup. But, just in case,
  // we do it again here.
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
  byte id[8];
  Wagman::getID(id);

  Logger::begin("id");
  printID(id);
  Logger::end();

  delay(50);

  DateTime dt;
  Wagman::getDateTime(dt);

  Logger::begin("date");
  printDate(dt);
  SerialUSB.println();

  delay(50);

  Logger::begin("cu");

  Logger::log(Wagman::getCurrent());

  for (byte i = 0; i < 5; i++) {
    Logger::log(' ');
    Logger::log(Wagman::getCurrent(i));
  }

  Logger::end();

  delay(50);

  Logger::begin("vdc");

  for (byte i = 0; i < 5; i++) {
    Logger::log(Wagman::getVoltage(i));
    Logger::log(' ');
  }

  Logger::end();

  delay(50);

  Logger::begin("th");

  for (byte i = 0; i < 5; i++) {
    Logger::log(Wagman::getThermistor(i));
    Logger::log(' ');
  }

  Logger::end();

  delay(50);

  unsigned int rawTemperature, rawHumidity;
  float temperature, humidity;
  bool temperatureOK = Wagman::getTemperature(&rawTemperature, &temperature);
  bool humidityOK = Wagman::getHumidity(&rawHumidity, &humidity);

  Logger::begin("temperature");
  Logger::log(rawTemperature);
  Logger::log(" ");
  Logger::log(temperature);

  if (!temperatureOK) {
    Logger::log(" err");
  }

  Logger::end();

  Logger::begin("humidity");
  Logger::log(rawHumidity);
  Logger::log(" ");
  Logger::log(humidity);

  if (!humidityOK) {
    Logger::log(" err");
  }

  Logger::end();

  unsigned int light;
  bool lightOK = Wagman::getLight(&light);

  Logger::begin("light");
  Logger::log(light);

  if (!lightOK) {
    Logger::log(" err");
  }

  Logger::end();

  Logger::end();

  delay(50);

  Logger::begin("fails");

  for (byte i = 0; i < 5; i++) {
    Logger::log(Record::getBootFailures(i));
    Logger::log(' ');
  }

  Logger::end();

  delay(50);

  Logger::begin("enabled");

  for (byte i = 0; i < 5; i++) {
    Logger::log(Record::getDeviceEnabled(i));
    Logger::log(' ');
  }

  Logger::end();

  Logger::begin("state");

  for (byte i = 0; i < 5; i++) {
    Logger::log(devices[i].getState());
    Logger::log(' ');
  }

  Logger::end();

  delay(50);

  Logger::begin("media");

  for (byte i = 0; i < 2; i++) {
    byte bootMedia = devices[i].getNextBootMedia();

    if (bootMedia == MEDIA_SD) {
      Logger::log("sd ");
    } else if (bootMedia == MEDIA_EMMC) {
      Logger::log("emmc ");
    } else {
      Logger::log("? ");
    }
  }

  Logger::end();
}
