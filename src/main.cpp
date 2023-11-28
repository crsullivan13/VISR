#include <Arduino.h>
#include <BleKeyboard.h>

//USE_NIMBLE defined in BleKeyboard.h -> drasticly decreases size of BLE libraries
//Heap size is 363700, free heap is 284764, sketch size is 499440 -> nimble on
//Heap size is 343588, free heap is 245716, sketch size is 883296 -> nimble off

BleKeyboard bleKeyboard;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  bleKeyboard.begin();
}

uint8_t count = 0;

void loop() {
  // uint32_t heapSize = ESP.getHeapSize();
  // uint32_t freeHeap = ESP.getFreeHeap();
  // uint32_t sketchSize = ESP.getSketchSize();

  // Serial.printf("Heap size is %d, free heap is %d, sketch size is %d\n", heapSize, freeHeap, sketchSize);

  if ( bleKeyboard.isConnected() ) {
    count++;

    if ( count == 1 ) {
      Serial.println("Sending F5");
      bleKeyboard.write(KEY_F5);

      delay(1000);
    }

    Serial.println("Sending Right Arrow key...");
    bleKeyboard.write(KEY_RIGHT_ARROW);

    delay(1000);

    Serial.println("Sending Left Arrow key...");
    bleKeyboard.write(KEY_LEFT_ARROW);

    delay(1000);

    if ( count == 8 ) {
      Serial.println("Sending esc key");

      count = 0;
      bleKeyboard.write(KEY_ESC);

      delay(1000);
    } 
  }

  Serial.println("Waiting 5 seconds...");
  delay(5000);
}