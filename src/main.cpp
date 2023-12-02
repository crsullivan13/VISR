#include <Arduino.h>
#include <USBHIDKeyboard.h>
#include <BleKeyboard.h>
#include <string>
uint8_t count = 0;
bool bleEnabled;

USBHIDKeyboard usb;
BleKeyboard ble;

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Test");

  Serial.println("Enable Bluetooth?");
  bleEnabled = Serial.read();
  Serial.println("You set bluetooth to:" + bleEnabled);
}

template <typename T>
void command(int cmd, T& input) {
  switch (cmd) {
    case 1:
      Serial.println("[CMD] F5");
      input.pressRaw(KEY_F5);
      break;
    case 2:
      Serial.println("[CMD] Right Arrow");
      keyboard.pressRaw(KEY_RIGHT_ARROW);
      break;
    case 3:
      Serial.println("[CMD] Left Arrow");
      keyboard.pressRaw(KEY_LEFT_ARROW);
      break;
    case 4:
      Serial.println("[CMD] Esc");
      keyboard.pressRaw(KEY_ESC);
      break;
     default:
      Serial.println("[USB] Err invalid option.");
      break;
  }
}

void loop() {
  if (count >5) {count = 1;}
  bleEnabled ? command(count, ble) : command(count, usb);
  count++;
  Serial.println("Done.");
  delay(5000);
}