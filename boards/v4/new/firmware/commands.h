// This file is part of the Waggle Platform.Please see the file
// LICENSE.waggle.txt for the legal details of the copyright and software
// license.For more details on the Waggle project, visit:
// http://www.wa8.gl
#include <Arduino.h>

struct Command {
    const char *name;
    byte (*func)(byte, const char **);
};

byte commandRTC(byte argc, const char **argv);
byte commandStart(byte argc, const char **argv);
byte commandStop(byte argc, const char **argv);
byte commandReset(byte argc, const char **argv);
byte commandPing(byte argc, const char **argv);
byte commandID(byte argc, const char **argv);
byte commandEEDump(byte argc, const char **argv);
byte commandDate(byte argc, const char **argv);
byte commandCurrent(byte argc, const char **argv);
byte commandHeartbeat(byte argc, const char **argv);
byte commandThermistor(byte argc, const char **argv);
byte commandEnvironment(byte argc, const char **argv);
byte commandBootMedia(byte argc, const char **argv);
byte commandFailCount(byte argc, const char **argv);
byte commandLog(byte argc, const char **argv);
byte commandBootFlags(byte argc, const char **argv);
byte commandUptime(byte argc, const char **argv);
byte commandEnable(byte argc, const char **argv);
byte commandDisable(byte argc, const char **argv);
byte commandWatch(byte argc, const char **argv);
byte commandResetEEPROM(byte argc, const char **argv);
byte commandWatch(byte argc, const char **argv);
byte commandBoots(byte argc, const char **argv);
byte commandVersion(byte argc, const char **argv);
byte commandBLFlag(byte argc, const char **argv);
