void setup() {
    Serial.begin(115200);
    SerialUSB.begin(115200);
}

void loop() {
    if (Serial.available()) {      // If anything comes in Serial (USB),
        SerialUSB.write(Serial.read());   // read it and send it out Serial1 (pins 0 & 1)
    }

    if (SerialUSB.available()) {     // If anything comes in Serial1 (pins 0 & 1)
        Serial.write(SerialUSB.read());   // read it and send it out Serial (USB)
    }
}