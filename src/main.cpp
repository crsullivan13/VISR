#include <Arduino.h>
#include <BleKeyboard.h>
#include <I2S.h>
#include <string>
#include <USBHIDKeyboard.h>
#include <VISR_Presentation_Controls_inferencing.h>

#define SAMPLE_RATE 16000U
#define SAMPLE_BITS 16
#define USER_LED 21

typedef struct {
  uint32_t n_samples;
  uint32_t buf_count;
  uint16_t* buffer;
  uint8_t buf_ready;
} inference_t;

uint8_t count = 0;
bool bleEnabled = false;
bool interfaceChosen = false;

unsigned long prevTimeHolder = 0;

USBHIDKeyboard usb;
BleKeyboard ble;

unsigned long TimeSince(unsigned long start);

template <typename T>
void command(int cmd, T& input);

void setup() {
  Serial.begin(115200);
  while(!Serial);

  pinMode(USER_LED, OUTPUT);
  digitalWrite(USER_LED, HIGH);

  I2S.setAllPins(-1, 42, 41, -1, -1);
  if ( !I2S.begin(PDM_MONO_MODE, SAMPLE_RATE, SAMPLE_BITS) )
  {
    Serial.println("I2S failed to init!");
    while(1);
  }
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

unsigned long TimeSince(unsigned long start) {
  return millis() - start;
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