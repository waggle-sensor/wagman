#include "config.cpp"
void setup(){
  SerialUSB.begin(115200);
  pinMode(EE_RST, INPUT);

}

void loop(){
  int ee_rst_val = digitalRead(EE_RST);
  if(!ee_rst_val){
    SerialUSB.println("You have pressed the button");
  }
}
