#include "config.cpp"
#include "./libs/HTU21D/HTU21D.h"
#include "./libs/MCP342X/MCP342X.h"
#include <Wire.h>

float htu21d_humd, htu21d_temp;

HTU21D myHumidity;
MCP342X mcp3428_1;
MCP342X mcp3428_2;


void turn_on_port_1 (void)
{
    digitalWrite(NC_LAT_CLK, LOW);
    digitalWrite(NC_LAT_D, HIGH);
    delay(100);
    digitalWrite(NC_LAT_CLK, HIGH);
    delay(100);
    digitalWrite(NC_LAT_CLK, LOW);
    digitalWrite(NC_LAT_D, LOW);
    delay(100);
}


void turn_off_port_1 (void)
{
    digitalWrite(NC_LAT_CLK, LOW);
    digitalWrite(NC_LAT_D, LOW);
    delay(100);
    digitalWrite(NC_LAT_CLK, HIGH);
    delay(100);
    digitalWrite(NC_LAT_CLK, LOW);
    digitalWrite(NC_LAT_D, LOW);
    delay(100);
}

void turn_on_port_2 (void)
{
    digitalWrite(EP_LAT_CLK, LOW);
    digitalWrite(EP_LAT_D, HIGH);
    delay(100);
    digitalWrite(EP_LAT_CLK, HIGH);
    delay(100);
    digitalWrite(EP_LAT_CLK, LOW);
    digitalWrite(EP_LAT_D, LOW);
    delay(100);
}


void turn_off_port_2 (void)
{
    digitalWrite(EP_LAT_CLK, LOW);
    digitalWrite(EP_LAT_D, LOW);
    delay(100);
    digitalWrite(EP_LAT_CLK, HIGH);
    delay(100);
    digitalWrite(EP_LAT_CLK, LOW);
    digitalWrite(EP_LAT_D, LOW);
    delay(100);
}


void turn_on_port_3 (void)
{
    digitalWrite(CS_LAT_CLK, LOW);
    digitalWrite(CS_LAT_D, HIGH);
    delay(100);
    digitalWrite(CS_LAT_CLK, HIGH);
    delay(100);
    digitalWrite(CS_LAT_CLK, LOW);
    digitalWrite(CS_LAT_D, LOW);
    delay(100);
}


void turn_off_port_3 (void)
{
    digitalWrite(CS_LAT_CLK, LOW);
    digitalWrite(CS_LAT_D, LOW);
    delay(100);
    digitalWrite(CS_LAT_CLK, HIGH);
    delay(100);
    digitalWrite(CS_LAT_CLK, LOW);
    digitalWrite(CS_LAT_D, LOW);
    delay(100);
}


void turn_on_port_4 (void)
{
    digitalWrite(P4_LAT_CLK, LOW);
    digitalWrite(P4_LAT_D, HIGH);
    delay(100);
    digitalWrite(P4_LAT_CLK, HIGH);
    delay(100);
    digitalWrite(P4_LAT_CLK, LOW);
    digitalWrite(P4_LAT_D, LOW);
    delay(100);
}


void turn_off_port_4 (void)
{
    digitalWrite(P4_LAT_CLK, LOW);
    digitalWrite(P4_LAT_D, LOW);
    delay(100);
    digitalWrite(P4_LAT_CLK, HIGH);
    delay(100);
    digitalWrite(P4_LAT_CLK, LOW);
    digitalWrite(P4_LAT_D, LOW);
    delay(100);
}



void turn_on_port_5 (void)
{
    digitalWrite(P5_LAT_CLK, LOW);
    digitalWrite(P5_LAT_D, HIGH);
    delay(100);
    digitalWrite(P5_LAT_CLK, HIGH);
    delay(100);
    digitalWrite(P5_LAT_CLK, LOW);
    digitalWrite(P5_LAT_D, LOW);
    delay(100);
}


void turn_off_port_5 (void)
{
    digitalWrite(P5_LAT_CLK, LOW);
    digitalWrite(P5_LAT_D, LOW);
    delay(100);
    digitalWrite(P5_LAT_CLK, HIGH);
    delay(100);
    digitalWrite(P5_LAT_CLK, LOW);
    digitalWrite(P5_LAT_D, LOW);
    delay(100);
}


void setup() {
    delay(2000);

    pinMode(NC_AUTO_DISABLE, OUTPUT);
    digitalWrite(NC_AUTO_DISABLE, LOW);

    SerialUSB.begin(115200);

    Wire.begin(); // look for the alternative pins
    myHumidity.begin();
    mcp3428_1.init(MCP342X::H, MCP342X::H); //U15 on the schematic
    mcp3428_2.init(MCP342X::L, MCP342X::H); //U17 on the schematic

    analogReadResolution(12);
    pinMode(NC_LAT_CLK, OUTPUT);
    pinMode(NC_LAT_D, OUTPUT);
    pinMode(EP_LAT_CLK, OUTPUT);
    pinMode(EP_LAT_D, OUTPUT);
    pinMode(CS_LAT_CLK, OUTPUT);
    pinMode(CS_LAT_D, OUTPUT);
    pinMode(P4_LAT_D, OUTPUT);
    pinMode(P4_LAT_CLK, OUTPUT);
    pinMode(P5_LAT_CLK, OUTPUT);
    pinMode(P5_LAT_D, OUTPUT);
}

void loop() {
    turn_off_port_1();
    turn_off_port_2();
    turn_off_port_3();
    turn_off_port_4();
    turn_off_port_5();
    delay(5000);
    SerialUSB.print("Photo Resistor Value: ");
    SerialUSB.println(analogRead(PHOTORESIST));
    SerialUSB.println("<------------------------------------->");

    SerialUSB.print("Thermistor_1 Value: ");
    SerialUSB.println(analogRead(P1_THM));
    SerialUSB.println("<------------------------------------->");

    SerialUSB.print("Thermistor_2 Value: ");
    SerialUSB.println(analogRead(P2_THM));
    SerialUSB.println("<------------------------------------->");

    SerialUSB.print("Thermistor_3 Value: ");
    SerialUSB.println(analogRead(P3_THM));
    SerialUSB.println("<------------------------------------->");

    SerialUSB.print("Thermistor_4 Value: ");
    SerialUSB.println(analogRead(P4_THM));
    SerialUSB.println("<------------------------------------->");

    SerialUSB.print("Thermistor_5 Value: ");
    SerialUSB.println(analogRead(P5_THM));
    SerialUSB.println("<------------------------------------->");

    SerialUSB.print("OP1 Value: ");
    SerialUSB.println(analogRead(OP1_ADC));
    SerialUSB.println("<------------------------------------->");

    SerialUSB.print("OP2 Value: ");
    SerialUSB.println(analogRead(OP2_ADC));
    SerialUSB.println("<------------------------------------->");

    SerialUSB.print("OP3 Value: ");
    SerialUSB.println(analogRead(OP3_ADC));
    SerialUSB.println("<------------------------------------->");

    SerialUSB.print("OP4 Value: ");
    SerialUSB.println(analogRead(OP4_ADC));
    SerialUSB.println("<------------------------------------->");

    SerialUSB.print("OP5 Value: ");
    SerialUSB.println(analogRead(OP5_ADC));
    SerialUSB.println("<------------------------------------->");

    SerialUSB.print("ADC_5V_IN value: ");
    SerialUSB.println(analogRead(ADC_5V_IN));
    SerialUSB.println("<------------------------------------->");

    htu21D_report();

    SerialUSB.println("<---------------------------");
    SerialUSB.println("Starting the current sensing");
    // the values below are from the sensor U15 as listed on the schematic
    SerialUSB.print("Current Sense 5V: ");
    mcp3428_1.selectChannel(MCP342X::CHANNEL_0, MCP342X::GAIN_1);
    SerialUSB.println(mcp3428_1.readADC()>>5);

    SerialUSB.print("Current Sense 1: ");
    mcp3428_1.selectChannel(MCP342X::CHANNEL_3, MCP342X::GAIN_1);
    SerialUSB.println(mcp3428_1.readADC()>>5);

    SerialUSB.print("Current Sense 2: ");
    mcp3428_1.selectChannel(MCP342X::CHANNEL_1, MCP342X::GAIN_1);
    SerialUSB.println(mcp3428_1.readADC()>>5);
    SerialUSB.print("Current Sense 3: ");

    mcp3428_1.selectChannel(MCP342X::CHANNEL_2, MCP342X::GAIN_1);
    SerialUSB.println(mcp3428_1.readADC()>>5);

    SerialUSB.print("Current Sense 4: ");
    mcp3428_2.selectChannel(MCP342X::CHANNEL_0, MCP342X::GAIN_1);
    SerialUSB.println(mcp3428_2.readADC()>>5);

    SerialUSB.print("Current Sense 5: ");
    mcp3428_2.selectChannel(MCP342X::CHANNEL_3, MCP342X::GAIN_1);
    SerialUSB.println(mcp3428_2.readADC()>>5);
    SerialUSB.println("<-------------------------->");

    SerialUSB.println("Now printing thermistor value");
    SerialUSB.print("Thermistor 0: ");
    mcp3428_2.selectChannel(MCP342X::CHANNEL_1, MCP342X::GAIN_1);
    SerialUSB.println(mcp3428_2.readADC()>>5);
    SerialUSB.print("LOPS: ");
    mcp3428_2.selectChannel(MCP342X::CHANNEL_2, MCP342X::GAIN_1);
    SerialUSB.println(mcp3428_2.readADC()>>5);
    delay(1000);

    turn_on_port_1();
    turn_on_port_2();
    turn_on_port_3();
    turn_on_port_4();
    turn_on_port_5();

    delay(5000);


    SerialUSB.print("Photo Resistor Value: ");
    SerialUSB.println(analogRead(PHOTORESIST));
    SerialUSB.println("<------------------------------------->");

    SerialUSB.print("Thermistor_1 Value: ");
    SerialUSB.println(analogRead(P1_THM));
    SerialUSB.println("<------------------------------------->");

    SerialUSB.print("Thermistor_2 Value: ");
    SerialUSB.println(analogRead(P2_THM));
    SerialUSB.println("<------------------------------------->");

    SerialUSB.print("Thermistor_3 Value: ");
    SerialUSB.println(analogRead(P3_THM));
    SerialUSB.println("<------------------------------------->");

    SerialUSB.print("Thermistor_4 Value: ");
    SerialUSB.println(analogRead(P4_THM));
    SerialUSB.println("<------------------------------------->");

    SerialUSB.print("Thermistor_5 Value: ");
    SerialUSB.println(analogRead(P5_THM));
    SerialUSB.println("<------------------------------------->");

    SerialUSB.print("OP1 Value: ");
    SerialUSB.println(analogRead(OP1_ADC));
    SerialUSB.println("<------------------------------------->");

    SerialUSB.print("OP2 Value: ");
    SerialUSB.println(analogRead(OP2_ADC));
    SerialUSB.println("<------------------------------------->");

    SerialUSB.print("OP3 Value: ");
    SerialUSB.println(analogRead(OP3_ADC));
    SerialUSB.println("<------------------------------------->");

    SerialUSB.print("OP4 Value: ");
    SerialUSB.println(analogRead(OP4_ADC));
    SerialUSB.println("<------------------------------------->");

    SerialUSB.print("OP5 Value: ");
    SerialUSB.println(analogRead(OP5_ADC));
    SerialUSB.println("<------------------------------------->");

    SerialUSB.print("ADC_5V_IN value: ");
    SerialUSB.println(analogRead(ADC_5V_IN));
    SerialUSB.println("<------------------------------------->");

    htu21D_report();

    SerialUSB.println("<---------------------------");
    SerialUSB.println("Starting the current sensing");
    // the values below are from the sensor U15 as listed on the schematic
    SerialUSB.print("Current Sense 5V: ");
    mcp3428_1.selectChannel(MCP342X::CHANNEL_0, MCP342X::GAIN_1);
    SerialUSB.println(mcp3428_1.readADC()>>5);

    SerialUSB.print("Current Sense 1: ");
    mcp3428_1.selectChannel(MCP342X::CHANNEL_3, MCP342X::GAIN_1);
    SerialUSB.println(mcp3428_1.readADC()>>5);

    SerialUSB.print("Current Sense 2: ");
    mcp3428_1.selectChannel(MCP342X::CHANNEL_1, MCP342X::GAIN_1);
    SerialUSB.println(mcp3428_1.readADC()>>5);
    SerialUSB.print("Current Sense 3: ");

    mcp3428_1.selectChannel(MCP342X::CHANNEL_2, MCP342X::GAIN_1);
    SerialUSB.println(mcp3428_1.readADC()>>5);

    SerialUSB.print("Current Sense 4: ");
    mcp3428_2.selectChannel(MCP342X::CHANNEL_0, MCP342X::GAIN_1);
    SerialUSB.println(mcp3428_2.readADC()>>5);

    SerialUSB.print("Current Sense 5: ");
    mcp3428_2.selectChannel(MCP342X::CHANNEL_3, MCP342X::GAIN_1);
    SerialUSB.println(mcp3428_2.readADC()>>5);
    SerialUSB.println("<-------------------------->");

    SerialUSB.println("Now printing thermistor value");
    SerialUSB.print("Thermistor 0: ");
    mcp3428_2.selectChannel(MCP342X::CHANNEL_1, MCP342X::GAIN_1);
    SerialUSB.println(mcp3428_2.readADC()>>5);
    SerialUSB.print("LOPS: ");
    mcp3428_2.selectChannel(MCP342X::CHANNEL_2, MCP342X::GAIN_1);
    SerialUSB.println(mcp3428_2.readADC()>>5);
    delay(1000);
}

void htu21D_report()
{
    htu21d_humd = myHumidity.readHumidity();
    htu21d_temp = myHumidity.readTemperature();
    SerialUSB.print("HTU21 Sensor: ");
    SerialUSB.print(" Temperature:");
    SerialUSB.print(htu21d_temp, 1);
    SerialUSB.print("C");
    SerialUSB.print(" Humidity:");
    SerialUSB.print(htu21d_humd, 1);
    SerialUSB.println("%");
    delay(10);
    return;
}

