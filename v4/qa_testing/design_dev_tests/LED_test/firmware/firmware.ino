#include <SPI.h>
#include <SD.h>
#include "config.cpp"
// PIN MAPPING
#define NUM_OF_LED 10

const int LED_ARR[NUM_OF_LED] = {LED0, LED1, LED2, LED3, LED4, LED5, LED6, LED7, LED8, LED9};
// the setup function runs once when you press reset or power the board

void setup() {
  Serial.begin(115200);
  for(int i = 0; i < NUM_OF_LED; i++)
  {
     pinMode(LED_ARR[i], OUTPUT);  
  }
}

void loop() {
  for(int i = 0; i < NUM_OF_LED; i++)
  {
     digitalWrite(LED_ARR[i], HIGH);
     delay(1000);
     digitalWrite(LED_ARR[i], LOW);  
  }
}
