//---------- C O N S T A N T S ------------------------------------------------




//---------- G L O B A L S ----------------------------------------------------




//---------- B O O T _ G N ----------------------------------------------------
/*
   Boots the guest nodes.

   :rtype: none
*/
void boot_gn()
{
  // Guest node 1 booted successfully?
  if(boot_gn1())
    // Mark guest node as operational
    GN1_booted = true;

  Serial.print("GN1 boot: ");
  Serial.println(GN1_booted);

  // Guest node 2 booted successfully?
  if (boot_gn2())
    // Mark guest node as operational
    GN2_booted = true;

  Serial.print("GN2 boot: ");
  Serial.println(GN2_booted);

  // Guest node 3 booted successfully?
  if (boot_gn3())
    // Mark guest node as operational
    GN3_booted = true;

  Serial.print("GN3 boot: ");
  Serial.println(GN3_booted);
}



//---------- B O O T _ G N 1 --------------------------------------------------
/*
   Boots guest node 1.  Checks temperature, power draw, and heartbeat.
   If something goes wrong, the node controller is notified.

   Return TRUE: guest node booted successfully.
   Return FALSE: guest node did not boot successfully.

   :rtype: boolean
*/
boolean boot_gn1()
{
  // Is guest node 1 enabled and expected to be present?
  if(eeprom_read_byte(&E_GN1_ENABLED) && eeprom_read_byte(&E_PRESENT_GN1))
  {
    // Is the guest node's temperature outside of safe parameters?
    if(!check_temp_gn(1))
    {
      // Giving the guest node one more chance...

      // Wait for things to settle down, perhaps
      delay(BOOT_BAD_ENVIRON_WAIT_TIME * 1000);

      // Is the guest node's temperature outside of safe parameters?
      if(!check_temp_gn(1))
      {
        // Inform node controller of failure
        send_problem(PROBLEM_BOOT_GN1_TEMP);

        // Exit with failure
        return false;
      }
    }

    // Turn on the guest node
    digitalWrite(PIN_RELAY_GN1, HIGH);

    // Give the guest node time to boot
    delay(eeprom_read_word(&E_BOOT_TIME_GN1) * 1000);

    // Is the guest node not drawing an expected amount of power?
    if(!check_power_gn(1))
    {
      // Giving the guest node one more chance...

      // Power cycle the guest node
      power_cycle(PIN_RELAY_GN1);

      // Give the guest node time to boot
      delay(eeprom_read_word(&E_BOOT_TIME_GN1) * 1000);

      // Is the guest node not drawing an expected amount of power?
      if(!check_power_gn(1))
      {
        // Inform node controller of failure
        send_problem(PROBLEM_BOOT_GN1_POWER);

        // Exit with failure
        return false;
      }
    }

    // Is the guest node alive (sending a "heartbeat")?
    if(!check_heartbeat_odroid(PIN_HEARTBEAT_GN1))
    {
      byte boot_attempts = 0;
      boolean _heartbeat_detected = false;

      // Try to get a heartbeat from the GN as many times as allowed
      while (boot_attempts < eeprom_read_byte(&E_MAX_NUM_SUBSYSTEM_BOOT_ATTEMPTS))
      {
        // Is "heartbeat" not detected?
        if(!check_heartbeat_odroid(PIN_HEARTBEAT_GN1))
        {
          // Power cycle the guest node
          power_cycle(PIN_RELAY_GN1);

          // Give the guest node time to boot
          delay(eeprom_read_word(&E_BOOT_TIME_GN1) * 1000);
        }
        else
          // Indicate that a heartbeat was detected
          _heartbeat_detected = true;

        // Increment counter for number of attempts
        boot_attempts++;
      }

      // Still no heartbeat detected?
      if(!_heartbeat_detected)
      {
        // Turn off the guest node
        //digitalWrite(PIN_RELAY_GN1, LOW);

        // Mark GN as dead
        eeprom_update_byte(&E_GN1_ENABLED, 0);

        // Inform node controller of failure
        send_problem(PROBLEM_BOOT_GN1_HEARTBEAT);

        // Exit with failure
        return false;
      }
    }

    // Exit with success
    return true;
  }
  else
    // Exit with failure
    return false;
}



//---------- B O O T _ G N 2 --------------------------------------------------
/*
   Boots guest node 2.  Checks temperature, power draw, and heartbeat.
   If something goes wrong, the node controller is notified.

   Return TRUE: guest node booted successfully.
   Return FALSE: guest node did not boot successfully.

   :rtype: boolean
*/
boolean boot_gn2()
{
  // Is guest node 2 enabled and expected to be present?
  if(eeprom_read_byte(&E_GN2_ENABLED) && eeprom_read_byte(&E_PRESENT_GN2))
  {
    // Is the guest node's temperature outside of safe parameters?
    if(!check_temp_gn(2))
    {
      // Giving the guest node one more chance...

      // Wait for things to settle down, perhaps
      delay(BOOT_BAD_ENVIRON_WAIT_TIME * 1000);

      // Is the guest node's temperature outside of safe parameters?
      if(!check_temp_gn(2))
      {
        // Inform node controller of failure
        send_problem(PROBLEM_BOOT_GN2_TEMP);

        // Exit with failure
        return false;
      }
    }

    // Turn on the guest node
    digitalWrite(PIN_RELAY_GN2, HIGH);

    // Give the guest node time to boot
    delay(eeprom_read_word(&E_BOOT_TIME_GN2) * 1000);

    // Is the guest node not drawing an expected amount of power?
    if(!check_power_gn(2))
    {
      // Giving the guest node one more chance...

      // Power cycle the guest node
      power_cycle(PIN_RELAY_GN2);

      // Give the guest node time to boot
      delay(eeprom_read_word(&E_BOOT_TIME_GN2) * 1000);

      // Is the guest node not drawing an expected amount of power?
      if(!check_power_gn(2))
      {
        // Inform node controller of failure
        send_problem(PROBLEM_BOOT_GN2_POWER);

        // Exit with failure
        return false;
      }
    }

    // Is the guest node alive (sending a "heartbeat")?
    if(!check_heartbeat_odroid(PIN_HEARTBEAT_GN2))
    {
      byte boot_attempts = 0;
      boolean _heartbeat_detected = false;

      // Try to get a heartbeat from the GN as many times as allowed
      while (boot_attempts < eeprom_read_byte(&E_MAX_NUM_SUBSYSTEM_BOOT_ATTEMPTS))
      {
        // Is "heartbeat" not detected?
        if(!check_heartbeat_odroid(PIN_HEARTBEAT_GN2))
        {
          // Power cycle the guest node
          power_cycle(PIN_RELAY_GN2);

          // Give the guest node time to boot
          delay(eeprom_read_word(&E_BOOT_TIME_GN2) * 1000);
        }
        else
          // Indicate that a heartbeat was detected
          _heartbeat_detected = true;

        // Increment counter for number of attempts
        boot_attempts++;
      }

      // Still no heartbeat detected?
      if(!_heartbeat_detected)
      {
        // Turn off the guest node
        digitalWrite(PIN_RELAY_GN2, LOW);

        // Mark GN as dead
        eeprom_update_byte(&E_GN2_ENABLED, 0);

        // Inform node controller of failure
        send_problem(PROBLEM_BOOT_GN2_HEARTBEAT);

        // Exit with failure
        return false;
      }
    }

    // Exit with success
    return true;
  }
  else
    // Exit with failure
    return false;
}



//---------- B O O T _ G N 3 --------------------------------------------------
/*
   Boots guest node 3.  Checks temperature, power draw, and heartbeat.
   If something goes wrong, the node controller is notified.

   Return TRUE: guest node booted successfully.
   Return FALSE: guest node did not boot successfully.

   :rtype: boolean
*/
boolean boot_gn3()
{
  // Is guest node 3 enabled and expected to be present?
  if(eeprom_read_byte(&E_GN3_ENABLED) && eeprom_read_byte(&E_PRESENT_GN3))
  {
    // Is the guest node's temperature outside of safe parameters?
    if(!check_temp_gn(3))
    {
      // Giving the guest node one more chance...

      // Wait for things to settle down, perhaps
      delay(BOOT_BAD_ENVIRON_WAIT_TIME * 1000);

      // Is the guest node's temperature outside of safe parameters?
      if(!check_temp_gn(3))
      {
        // Inform node controller of failure
        send_problem(PROBLEM_BOOT_GN3_TEMP);

        // Exit with failure
        return false;
      }
    }

    // Turn on the guest node
    digitalWrite(PIN_RELAY_GN3, HIGH);

    // Give the guest node time to boot
    delay(eeprom_read_word(&E_BOOT_TIME_GN3) * 1000);

    // Is the guest node not drawing an expected amount of power?
    if(!check_power_gn(3))
    {
      // Giving the guest node one more chance...

      // Power cycle the guest node
      power_cycle(PIN_RELAY_GN3);

      // Give the guest node time to boot
      delay(eeprom_read_word(&E_BOOT_TIME_GN3) * 1000);

      // Is the guest node not drawing an expected amount of power?
      if(!check_power_gn(3))
      {
        // Inform node controller of failure
        send_problem(PROBLEM_BOOT_GN3_POWER);

        // Exit with failure
        return false;
      }
    }

    // Is the guest node alive (sending a "heartbeat")?
    if(!check_heartbeat_odroid(PIN_HEARTBEAT_GN3))
    {
      byte boot_attempts = 0;
      boolean _heartbeat_detected = false;

      // Try to get a heartbeat from the GN as many times as allowed
      while (boot_attempts < eeprom_read_byte(&E_MAX_NUM_SUBSYSTEM_BOOT_ATTEMPTS))
      {
        // Is "heartbeat" not detected?
        if(!check_heartbeat_odroid(PIN_HEARTBEAT_GN3))
        {
          // Power cycle the guest node
          power_cycle(PIN_RELAY_GN3);

          // Give the guest node time to boot
          delay(eeprom_read_word(&E_BOOT_TIME_GN3) * 1000);
        }
        else
          // Indicate that a heartbeat was detected
          _heartbeat_detected = true;

        // Increment counter for number of attempts
        boot_attempts++;
      }

      // Still no heartbeat detected?
      if(!_heartbeat_detected)
      {
        // Turn off the guest node
        digitalWrite(PIN_RELAY_GN3, LOW);

        // Mark GN as dead
        eeprom_update_byte(&E_GN3_ENABLED, 0);

        // Inform node controller of failure
        send_problem(PROBLEM_BOOT_GN3_HEARTBEAT);

        // Exit with failure
        return false;
      }
    }

    // Exit with success
    return true;
  }
  else
    // Exit with failure
    return false;
}



//---------- C H E C K _ T E M P _ G N ----------------------------------------
/*
   Reads the thermistor for the specified guest node.

   Return TRUE: temperature is acceptable.
   Return FALSE: temperature is unacceptable.

   :param byte gn: which guest node's temperature to check (1, 2 or 3)

   :rtype: boolean
*/
boolean check_temp_gn(byte gn)
{
  unsigned int temp_ADC;

  // Which guest node is being checked?
  switch (gn) {
    case 1:
      // Get ADC result from thermistor
      temp_ADC = analogRead(PIN_THERMISTOR_GN1);

      Serial.println(temp_ADC);

      // Is measured temperature acceptable?
      if((eeprom_read_word(&E_TEMP_MIN_GN1) < temp_ADC)
          && (temp_ADC < eeprom_read_word(&E_TEMP_MAX_GN1)))
      {
        // Exit with success
        return true;
      }
      else
        // Exit with failure
        return false;

    case 2:
      // Get ADC result from thermistor
      temp_ADC = analogRead(PIN_THERMISTOR_GN2);

      // Is measured temperature acceptable?
      if((eeprom_read_word(&E_TEMP_MIN_GN2) < temp_ADC)
          && (temp_ADC < eeprom_read_word(&E_TEMP_MAX_GN2)))
      {
        // Exit with success
        return true;
      }
      else
        // Exit with failure
        return false;

    case 3:
      // Get ADC result from thermistor
      temp_ADC = analogRead(PIN_THERMISTOR_GN3);

      // Is measured temperature acceptable?
      if((eeprom_read_word(&E_TEMP_MIN_GN3) < temp_ADC)
          && (temp_ADC < eeprom_read_word(&E_TEMP_MAX_GN3)))
      {
        // Exit with success
        return true;
      }
      else
        // Exit with failure
        return false;

    default:
      // Invalid guest node, so exit with failure
      return false;
  }
}



//---------- C H E C K _ P O W E R _ G N --------------------------------------
/*
   Reads the specified guest node's current sensor.

   Return TRUE: guest node is drawing expected current.
   Return FALSE: guest node is drawing no/too much current.

   :param byte gn: which guest node's power draw to check (1, 2 or 3)

   :rtype: boolean
*/
boolean check_power_gn(byte gn)
{
  byte msb, csb, lsb;
  int addr;

  // Which guest node is being checked?
  switch (gn) {
    case 1:
      // Assign the correct I2C address
      addr = ADDR_CURRENT_SENSOR_GN1;
      break;

    case 2:
      // Assign the correct I2C address
      addr = ADDR_CURRENT_SENSOR_GN2;
      break;

    case 3:
      // Assign the correct I2C address
      addr = ADDR_CURRENT_SENSOR_GN3;
      break;

    default:
      // Invalid guest node, so exit with failure
      return false;
  }

  // Start I2C transaction with current sensor
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
  }
  else
    // Exit with failure
    return false;

  // End I2C transaction (with stop bit)
  Wire.endTransmission(1);

  // Calculate milliamps from raw sensor data
  unsigned int milliamps = ((csb << 8) | lsb) * MILLIAMPS_PER_STEP;

  // Which guest node is being checked?
  switch (gn) {
    case 1:
      // Is measured current below allowed maximum?
      if(milliamps < eeprom_read_word(&E_AMP_MAX_GN1))
        // Exit with success
        return true;

    case 2:
      // Is measured current below allowed maximum?
      if(milliamps < eeprom_read_word(&E_AMP_MAX_GN2))
        // Exit with success
        return true;

    case 3:
      // Is measured current below allowed maximum?
      if(milliamps < eeprom_read_word(&E_AMP_MAX_GN3))
        // Exit with success
        return true;

    default:
      // Invalid guest node, so exit with failure
      return false;
  }

  // Exit with failure
  return false;
}
