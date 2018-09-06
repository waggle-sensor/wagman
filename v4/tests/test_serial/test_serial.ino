void setup() {
    SerialUSB.begin(9600);
 
    Serial1.begin(9600);
    Serial1.setTimeout(1000);
    delay(1000);

    Serial2.begin(9600);
    Serial2.setTimeout(1000);
    delay(1000);
    
    Serial3.begin(9600);
    Serial3.setTimeout(1000);
    delay(1000);
}

void printBytesWaiting() {
    SerialUSB.print(Serial1.available());
    SerialUSB.print(" ");
    SerialUSB.print(Serial2.available());
    SerialUSB.print(" ");
    SerialUSB.print(Serial3.available());
    SerialUSB.println();
}

// ERROR
//
// 1 2 3 4 5
//
// Writes to Serial1 input buffer. (With no connections!!!)
void loopNone() {
    printBytesWaiting();
    delay(500);
    Serial2.write('X');
    delay(500);
}


// ERROR
// ^ ^
// 1 2 3 4 5
//
// Writes to both Serial1 and *sometimes* Serial2 input buffer.
//
// Haven't worked out why Serial2 write randomly seems to happen.
void loopCross1() {
    printBytesWaiting();
    delay(500);
    Serial1.write('X');
    delay(500);
}

// ERROR
//   ^
// 1 2 3 4 5
//
// Writes to both Serial1 and Serial2 input buffer.
void loopCross2() {
    printBytesWaiting();
    delay(500);
    Serial2.write('X');
    delay(500);
}

void loop() {
//    loopNoWiresConnected();
    loopCross1();
//    loopCross2();
}

