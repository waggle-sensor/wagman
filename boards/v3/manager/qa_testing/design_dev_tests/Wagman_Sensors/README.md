# Test Scope and Coverage

This test evaluates the following aspects of Wagman system design. </br>
*   The proper wiring of on-board sensors.
    -   HTU21D</br>
    <img src="./resources/HTU21.jpg" width="320">
    -   HIH4030</br>
    <img src="./resources/HIH4030.jpg" width="320">
    -   Five Thermistors, four of which are sampled using MCP3428 A/D converter</br>
    <img src="./resources/Thermistor.jpg" width="320">
    -   Optoresistor</br>
    <img src="./resources/LightSensor.jpg" width="320">
*    The functioning of the on-board sensors.
</br>

# Test Setup Procedure

## Test Events Timeline
* Wagman is loaded with firmware and the sensor test is conducted.

## Electrical Connection
*  Connect the two leads of the thermistors to PINS 5 and 6 of J5, J6, J7, J8 and J9 as shown below.
<img src="./resources/Thermistor_wiring.jpg" width="320">
*  Connect micro-USB cable between Wagman's J3 and QA computer.
*  Connect 5V DC power to Wagman.

__Please make sure only the Wagman under test is connected to the QA computer. No other Wagman or Arduino Micro/derivatives can be connected at this time.__

## Testing Procedure
*   Start the test by issuing the command *make test* in the Wagman subfolder
    and follow the on-screen prompts.

    __To exit the screen session created for the test, press Control+a, followed by k and y.__

## Test Log
This test is evaluated and scored on the __Wagman QA Scoresheet__.

# Success Criteria
The test is considered __PASSED__ if all the subtests under the Environmental Sensors Test pass.



