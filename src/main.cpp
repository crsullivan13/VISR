#include <Arduino.h>
#include <BleKeyboard.h>
#include <I2S.h>
#include <string>
#include <USBHIDKeyboard.h>
#include <VISR_Presentation_Controls_inferencing.h>

#define SAMPLE_RATE 16000U
#define SAMPLE_BITS 16
#define SAMPLE_BUF_SIZE 2048U
#define USER_LED 21

typedef struct {
  uint32_t n_samples;
  uint32_t buf_count;
  int16_t* buffer;
  uint8_t buf_ready;
} inference_t;

typedef enum {
  FORWARD,
  GO,
  LEFT,
  NOISE,
  STOP,

  NONE
} inferenceLabel;

static inference_t inference;
static int16_t sampleBuffer[SAMPLE_BUF_SIZE];
static bool isRecording = false;

static bool bleEnabled = false;
static bool interfaceChosen = false;

static uint8_t exitCount = 0;
static unsigned long prevTimeHolder = 0;

USBHIDKeyboard usb;
BleKeyboard ble;

unsigned long TimeSince(unsigned long start);

template <typename T>
void command(inferenceLabel cmd, T& input);

inferenceLabel GetLabel();

void SampleCallback(uint32_t nBytes);
void RecordSample(void* arg);
bool BeginSampling(uint32_t nSamples);

int GetAudioData(size_t offset, size_t length, float* out_ptr);

void setup() {
  Serial.begin(115200);
  while(!Serial);

  pinMode(USER_LED, OUTPUT);
  digitalWrite(USER_LED, LOW);

  I2S.setAllPins(-1, 42, 41, -1, -1);
  if ( !I2S.begin(PDM_MONO_MODE, SAMPLE_RATE, SAMPLE_BITS) )
  {
    Serial.println("I2S failed to init!");
    return;
  }

  if ( !BeginSampling(EI_CLASSIFIER_RAW_SAMPLE_COUNT) ) {
    ei_printf("Failed to allocate sampling buffer\n");
    return;
  }

  ble.begin();
  bleEnabled = true;
}

void loop() {
  //Commented out code is for selection between USB/BLE interface, omitting for the demo
  // if ( !interfaceChosen ) {
  //   if ( TimeSince(prevTimeHolder) >= 1000 ) {
  //     Serial.println("Enable Bluetooth? [y/n]");
  //     prevTimeHolder = millis();
  //   }

  //   //Only read from serial if there is actually something in the buffer to read
  //   if ( Serial.available() > 0 ) {
  //     uint8_t choice = Serial.read(); //This overload of serial read gets a single byte
  //     bleEnabled = choice == 'y' ? true : false;

  //     Serial.println("You set interface to: " + choice);
  //     interfaceChosen = true;

  //     if ( bleEnabled ) {
  //       ble.begin();
  //       prevTimeHolder = millis();
  //     } else {
  //       usb.begin();
  //     }
  //   }
  // } else if ( interfaceChosen ) {
    inferenceLabel label = GetLabel();

    if ( bleEnabled && ble.isConnected() ) {
      command(label, ble);
      //prevTimeHolder = millis(); //Update the BLE reset timer everytime we confirm a connection
    } else if ( !bleEnabled ) {
      command(label, usb);
    }
    // else if ( bleEnabled && !ble.isConnected() )
    // {
    //   //If we haven't got a ble connection for 30 seconds, then turn BLE off and reset
    //   if ( TimeSince(prevTimeHolder) >= 30000 ) {
    //     Serial.println("No BLE connection after 30 seconds, returning to interface selection");
    //     ble.end();
    //     interfaceChosen = false;
    //   }
    // }
  // }
}

unsigned long TimeSince(unsigned long start) {
  return millis() - start;
}

template <typename T>
void command(inferenceLabel cmd, T& input) {
  switch (cmd) {
    case GO:
      Serial.println("[CMD] F5");
      input.write(KEY_F5);
      break;
    case FORWARD:
      Serial.println("[CMD] Right Arrow");
      input.write(KEY_RIGHT_ARROW);
      break;
    case LEFT:
      Serial.println("[CMD] Left Arrow");
      input.write(KEY_LEFT_ARROW);
      break;
    case STOP:
      exitCount += 1;
      if ( exitCount != 0 && exitCount < 2) {
        prevTimeHolder = millis();
      } else if ( TimeSince(prevTimeHolder) <= 2500 ) {
        Serial.println("[CMD] Esc");
        input.write(KEY_ESC);
        exitCount = 0;
      } else {
        exitCount = 0;
      }
      break;
     default:
      Serial.println("[CMD] Err invalid option (NOISE).");
      break;
  }
}

inferenceLabel GetLabel() {
    while ( !inference.buf_ready ) {
    delay(10);
  }
  inference.buf_ready = false;

  signal_t signal;
  signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
  signal.get_data = &GetAudioData;
  ei_impulse_result_t result = {0};

  EI_IMPULSE_ERROR classifierError = run_classifier(&signal, &result, false);
  if ( classifierError != EI_IMPULSE_OK ) {
    ei_printf("Classifier failed with error: %d\n", classifierError);
    return NONE;
  }

  int predictionIndex = NONE;
  float predictionValue = 0;
  float tempPrediciton = 0;

  ei_printf("Classifier results (DSP: %d ms, Classification: %d ms, Anomaly: %d, ms)\n", result.timing.dsp, result.timing.classification, result.timing.anomaly);
  for ( size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++ ) {
    tempPrediciton = result.classification[i].value;

    ei_printf("\t%s: ", result.classification[i].label);
    ei_printf_float(tempPrediciton);
    ei_printf("\n");

    if ( tempPrediciton > predictionValue && tempPrediciton > .4 ) {
      predictionIndex = i;
      predictionValue = tempPrediciton;
    }
  }

  return (inferenceLabel)predictionIndex;
}

//Below code is mostly from the KWS lab from class, modified a bit to make it read better
void SampleCallback(uint32_t nBytes) {
  for ( int i = 0; i < nBytes; i++ ) {
    inference.buffer[inference.buf_count++] = sampleBuffer[i];

    if ( inference.buf_count >= inference.n_samples ) {
      inference.buf_ready = true;
      inference.buf_count = 0;
    }
  }
}

void RecordSample(void* arg) {
  const size_t i2sBytesToRead = (size_t)arg;
  size_t bytesRead = 0;

  while ( isRecording ) {
    esp_i2s::i2s_read(esp_i2s::I2S_NUM_0, (void*)sampleBuffer, i2sBytesToRead, &bytesRead, 200);

    if ( bytesRead <= 0 ) {
      ei_printf("Bad I2S read: %d bytes\n", bytesRead);
    } else {
      if ( bytesRead < i2sBytesToRead ) {
        printf("Only got %d bytes from I2S\n", bytesRead);
      }

      //bytesRead/2 because sampleBuffer is 16 bit ints
      for ( int i = 0; i < bytesRead/2; i++ ) {
        sampleBuffer[i] = sampleBuffer[i] * 8;
      }

      if ( isRecording ) {
        SampleCallback(bytesRead/2);
      } else {
        break;
      }
    }
  }

  //Passing NULL to this frees the calling task
  vTaskDelete(NULL);
}

bool BeginSampling(uint32_t nSamples) {
  bool isSuccess = true;

  inference.buffer = (int16_t*)malloc(nSamples * sizeof(int16_t));

  //handle malloc error case
  if ( inference.buffer == NULL ) {
    isSuccess = false;
  } else {
    inference.buf_count = 0;
    inference.buf_ready = false;
    inference.n_samples = nSamples;

    ei_sleep(100);

    isRecording = true;

    //Create RTOS audio sampling task
    xTaskCreate(RecordSample, "RecordSample", 1024 * 32, (void*)SAMPLE_BUF_SIZE, 10, NULL);
  }

  return isSuccess;
}

int GetAudioData(size_t offset, size_t length, float* out_ptr) {
  numpy::int16_to_float(&inference.buffer[offset], out_ptr, length);

  return 0;
}