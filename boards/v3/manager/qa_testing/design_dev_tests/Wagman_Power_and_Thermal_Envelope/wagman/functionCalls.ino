
void set_up_pinmodes ()
{
    //Setup PIN modes
    pinMode(PIN_Debug_L, OUTPUT); // L LED
    pinMode(PIN_Debug_L1, OUTPUT);  // L1 LED


    pinMode(PIN_NC_POW_State_Latch, OUTPUT);    //CLK for the Latch that saves NC power state across Wagman reboots
    pinMode(PIN_BootSelect_NC, OUTPUT);         //High to boot from eMMC, Low to boot from uSD
    pinMode(PIN_NC_POW_State, OUTPUT);          //High to ON, Low to OFF - when the CLK changes from Low to High
    pinMode(PIN_BootSelect_GM, OUTPUT);         //High to boot eMMC, Low to boot uSD

    pinMode(PIN_POW_2, OUTPUT);  // Power Set 2
    pinMode(PIN_POW_3, OUTPUT);  // Power Set 3
    pinMode(PIN_POW_4, OUTPUT);  // Power Set 4
    pinMode(PIN_POW_5, OUTPUT);  // Power Set 5



    pinMode(PIN_HBT1, INPUT);   // Heartbeat 1
    pinMode(PIN_HBT2, INPUT);   // Heartbeat 2
    pinMode(PIN_HBT3, INPUT);   // Heartbeat 3
    pinMode(PIN_HBT4, INPUT);   // Heartbeat 4
    pinMode(PIN_HBT5, INPUT);   // Heartbeat 5
    return;
}

void power_on_all ()
{
    turnON_POW1();
    delay(200);
    turnON_POW2();
    delay(200);
    turnON_POW3();
    delay(200);
    turnON_POW4();
    delay(200);
    turnON_POW5();
    return;
}

void power_off_all ()
{
    turnOFF_POW1();
    delay(200);
    turnOFF_POW2();
    delay(200);
    turnOFF_POW3();
    delay(200);
    turnOFF_POW4();
    delay(200);
    turnOFF_POW5();
    return;
}

void boot_pow_check()
{

    Serial.println("We will power ON all ports one by one. You should hear 5 seperate clicks, and see the current usage increase.");
    currentusage_report();

    Serial.print("Click 1 ");
    turnON_POW1();
    delay(2000);

    Serial.print("Click 2 ");
    turnON_POW2();
    delay(2000);

    Serial.print("Click 3 ");
    turnON_POW3();
    delay(2000);

    Serial.print("Click 4 ");
    turnON_POW4();
    delay(2000);

    Serial.println("Click 5");
    turnON_POW5();
    delay(2000);

    currentusage_report();
    delay(2000);

    Serial.println("Powering off all relays. The current usage reported now should all be under 120.");
    power_off_all();
    delay(1000);
    currentusage_report();
    delay(1000);
    return;
}


void turnON_POW1 ()
{
    digitalWrite(PIN_NC_POW_State_Latch, LOW);
    //Setting state to ON
    digitalWrite(PIN_NC_POW_State, HIGH);
    delay(2);
    // giving raising clock edge
    digitalWrite(PIN_NC_POW_State_Latch, HIGH);
    delay(2);
    // lowering clock edge
    digitalWrite(PIN_NC_POW_State_Latch, LOW);
    delay(2);
    return;
}

void turnON_POW2()
{
    digitalWrite(PIN_POW_2, HIGH);
    return;
}

void turnON_POW3()
{
    digitalWrite(PIN_POW_3, HIGH);
    return;
}
void turnON_POW4()
{
    digitalWrite(PIN_POW_4, HIGH);
    return;
}
void turnON_POW5()
{
    digitalWrite(PIN_POW_5, HIGH);
    return;
}

void turnOFF_POW1 () {
    digitalWrite(PIN_NC_POW_State_Latch, LOW);
    //Setting state to ON
    digitalWrite(PIN_NC_POW_State, LOW);
    delay(2);
    // giving raising clock edge
    digitalWrite(PIN_NC_POW_State_Latch, HIGH);
    delay(2);
    // lowering clock edge
    digitalWrite(PIN_NC_POW_State_Latch, LOW);
    delay(2);
    return;
}

void WagID_print()
{
    byte k;
    RTC.idRead(WagID);
    Serial.print("Unique Board ID # ");
    for (k = 0; k<0x08; k++)
    {
        Serial.print(WagID[k],HEX);
        if (k<0x07)
        {
            Serial.print(":");
        }
    }
    Serial.println("");
}


void turnOFF_POW2()
{
    digitalWrite(PIN_POW_2, LOW);
    return;
}

void turnOFF_POW3()
{
    digitalWrite(PIN_POW_3, LOW);
    return;
}
void turnOFF_POW4()
{
    digitalWrite(PIN_POW_4, LOW);
    return;
}
void turnOFF_POW5()
{
    digitalWrite(PIN_POW_5, LOW);
    return;
}


void thermistor_report()
{
    Serial.print("Thermistor Values:");
    Serial.print(analogRead(PIN_Therm_NC));
    Serial.print(',');
    mcp3428_1.selectChannel(MCP342X::CHANNEL_0, MCP342X::GAIN_1);
    Serial.print(mcp3428_1.readADC()>>5);
    Serial.print(',');
    mcp3428_1.selectChannel(MCP342X::CHANNEL_1, MCP342X::GAIN_1);
    Serial.print(mcp3428_1.readADC()>>5);
    Serial.print(',');
    mcp3428_1.selectChannel(MCP342X::CHANNEL_2, MCP342X::GAIN_1);
    Serial.print(mcp3428_1.readADC()>>5);
    Serial.print(',');
    mcp3428_1.selectChannel(MCP342X::CHANNEL_3, MCP342X::GAIN_1);
    Serial.println(mcp3428_1.readADC()>>5);
    return;
}


int read_current(int addr)
{
    byte msb,csb,lsb, logged;
    // Start I2C transaction with current sensor

    logged = 0;
    do{

        Wire.beginTransmission(addr);
        // Tell sensor we want to read "data" register
        Wire.write(0);
        // Sensor expects restart condition, so end I2C transaction (no stop bit)
        Wire.endTransmission(0);
        // Ask sensor for data
        Wire.requestFrom(addr, 3);

        // Read the 3 bytes that the sensor returns
        if(Wire.available())
        {
            msb = Wire.read();
            // We only care about the data, so the mask hides the SYNC flag
            csb = Wire.read() & 0x01;
            lsb = Wire.read();
            logged = 1;
        }
        else
        {
            delay(500);
        }
    }while(logged == 0);

    // End I2C transaction (with stop bit)
    Wire.endTransmission(1);

    // Calculate milliamps from raw sensor data
    unsigned int milliamps = ((csb << 8) | lsb) * MILLIAMPS_PER_STEP;
    return milliamps;
}

void currentusage_report(void)
{
    Serial.print("Current consumption:");
    Serial.print(read_current(ADDR_CURRENT_SYS));
    Serial.print(';');
    delay(5);
    Serial.print(read_current(ADDR_CURRENT_POW1));
    Serial.print(',');
    delay(5);
    Serial.print(read_current(ADDR_CURRENT_POW2));
    Serial.print(',');
    delay(5);
    Serial.print(read_current(ADDR_CURRENT_POW3));
    Serial.print(',');
    delay(5);
    Serial.print(read_current(ADDR_CURRENT_POW4));
    Serial.print(',');
    delay(5);
    Serial.println(read_current(ADDR_CURRENT_POW5));
    return;
}
