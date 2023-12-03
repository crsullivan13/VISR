#include <Arduino.h>
#include <USBHIDKeyboard.h>
#include <BleKeyboard.h>
#include <string>

uint8_t count = 0;
bool bleEnabled = false;
bool interfaceChosen = false;

unsigned long prevTimeHolder = 0;

USBHIDKeyboard usb;
BleKeyboard ble;

void setup() {
  Serial.begin(115200);
}

template <typename T>
void command(int cmd, T& input) {
  switch (cmd) {
    case 1:
      Serial.println("[CMD] F5");
      input.write(KEY_F5);
      break;
    case 2:
      Serial.println("[CMD] Right Arrow");
      input.write(KEY_RIGHT_ARROW);
      break;
    case 3:
      Serial.println("[CMD] Left Arrow");
      input.write(KEY_LEFT_ARROW);
      break;
    case 4:
      Serial.println("[CMD] Esc");
      input.write(KEY_ESC);
      break;
     default:
      Serial.println("[CMD] Err invalid option.");
      break;
  }
}

unsigned long TimeSince(unsigned long start) {
  return millis() - start;
}

void loop() {
  if ( !interfaceChosen )
  {
    if ( TimeSince(prevTimeHolder) >= 1000 )
    {
      Serial.println("Enable Bluetooth? [y/n]");
      prevTimeHolder = millis();
    }

    //Only read from serial if there is actually something in the buffer to read
    if ( Serial.available() > 0 )
    {
      uint8_t choice = Serial.read(); //This overload of serial read gets a single byte
      bleEnabled = choice == 'y' ? true : false;

      Serial.println("You set interface to: " + choice);
      interfaceChosen = true;

      if ( bleEnabled )
      {
        ble.begin();
        prevTimeHolder = millis();
      }
      else
      {
        usb.begin();
      }
    }
  }
  else if ( interfaceChosen )
  {
    if ( count > 5 ) 
    {
      count = 1;
    }

    if ( bleEnabled && ble.isConnected() )
    {
      command(count, ble);
      prevTimeHolder = millis(); //Update the BLE reset timer everytime we confirm a connection
    }
    else if ( !bleEnabled )
    {
      command(count, usb);
    }
    else if ( bleEnabled && !ble.isConnected() )
    {
      //If we haven't got a ble connection for 30 seconds, then turn BLE off and reset
      if ( TimeSince(prevTimeHolder) >= 30000 )
      {
        Serial.println("No BLE connection after 30 seconds, returning to interface selection");
        ble.end();
        interfaceChosen = false;
      }
    }

    count++;
  }

  Serial.println("Done.");
  delay(5000);
}