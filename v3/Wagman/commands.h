#include <Arduino.h>

struct Command {
    const char *name;
    uint8_t (*func)(uint8_t, const char **);
};

uint8_t commandStart(uint8_t argc, const char **argv);
uint8_t commandStop(uint8_t argc, const char **argv);
uint8_t commandKill(uint8_t argc, const char **argv);
uint8_t commandReset(uint8_t argc, const char **argv);
uint8_t commandPing(uint8_t argc, const char **argv);
uint8_t commandID(uint8_t argc, const char **argv);
uint8_t commandEEDump(uint8_t argc, const char **argv);
uint8_t commandDate(uint8_t argc, const char **argv);
uint8_t commandCurrent(uint8_t argc, const char **argv);
uint8_t commandHeartbeat(uint8_t argc, const char **argv);
uint8_t commandThermistor(uint8_t argc, const char **argv);
uint8_t commandEnvironment(uint8_t argc, const char **argv);
uint8_t commandBootMedia(uint8_t argc, const char **argv);
uint8_t commandFailCount(uint8_t argc, const char **argv);
uint8_t commandLog(uint8_t argc, const char **argv);
uint8_t commandBootFlags(uint8_t argc, const char **argv);
uint8_t commandUptime(uint8_t argc, const char **argv);
uint8_t commandEnable(uint8_t argc, const char **argv);
uint8_t commandDisable(uint8_t argc, const char **argv);
uint8_t commandWatch(uint8_t argc, const char **argv);
uint8_t commandResetEEPROM(uint8_t argc, const char **argv);

