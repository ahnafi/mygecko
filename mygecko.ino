#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <MainDisplay.h>
#include <Eat.h>
#include <Dance.h>
#include <Nono.h>
#include <Patpat.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C
#define SERIAL_BAUDRATE 115200

const int touchPin = 15;
const int btnLeft = 5;
const int btnMid = 18;
const int btnRight = 19;
const int buzzerPin = 23;

Adafruit_SSD1306 displayI2c = 
    Adafruit_SSD1306(
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        &Wire
    );

// define fungsi
void TaskDisplay(void *pvParameters);
void TaskInput(void *pvParameters);
void TaskAudio(void *pvParameters);

// var global
enum EventType {
  EV_NONE,
  EV_PAT,
  EV_EAT,
  EV_STATS,
  EV_MUSIC
};

enum DisplayState {
  STATE_IDLE,
  STATE_PATTING,
  STATE_EATING,
  STATE_STATS,
  STATE_DANCING
};

QueueHandle_t eventQueue;
QueueHandle_t audioQueue;

volatile int animationFrame = 0;
unsigned long lastAnimationFrame = 0;

void setup() {
  Serial.begin(SERIAL_BAUDRATE);

  displayI2c.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS); // INITIATE

  // task input
  pinMode(btnLeft, INPUT_PULLUP);
  pinMode(btnMid, INPUT_PULLUP);
  pinMode(btnRight, INPUT_PULLUP);
  pinMode(touchPin, INPUT_PULLUP);
  ledcAttach(buzzerPin, 5000, 8);

  displayI2c.clearDisplay();

  eventQueue = xQueueCreate(10, sizeof(EventType));
  audioQueue = xQueueCreate(10, sizeof(EventType));

  xTaskCreate(TaskDisplay, "Task Display", 6144, NULL, 1, NULL);
  xTaskCreate(TaskInput, "Task Input", 2048, NULL, 1, NULL);
  xTaskCreate(TaskAudio, "Task Audio", 2048, NULL, 1, NULL);
}

void loop() {}

void TaskDisplay(void *pvParameters) {

  displayI2c.setTextSize(1);
  displayI2c.setTextColor(SSD1306_WHITE);
  lastAnimationFrame = millis();

  DisplayState currentState = STATE_IDLE;
  TickType_t stateStartTime = 0;

  for (;;) {
    unsigned long currentMillis = millis();
    displayI2c.clearDisplay();

    EventType newEvent;
    if (xQueueReceive(eventQueue, &newEvent, 0) == pdTRUE) {
      if (newEvent == EV_PAT) {
        currentState = STATE_PATTING;
        stateStartTime = xTaskGetTickCount();
        animationFrame = 0;
      } else if (newEvent == EV_EAT) {
        currentState = STATE_EATING;
        stateStartTime = xTaskGetTickCount();
        animationFrame = 0;
      } else if (newEvent == EV_STATS) {
        currentState = STATE_STATS;
        stateStartTime = xTaskGetTickCount();
      } else if (newEvent == EV_MUSIC) {
        currentState = STATE_DANCING;
        stateStartTime = xTaskGetTickCount();
        animationFrame = 0;
      }
    }

    if (currentState != STATE_IDLE) {
      if (xTaskGetTickCount() - stateStartTime >= 4000 / portTICK_PERIOD_MS) {
        currentState = STATE_IDLE;
        animationFrame = 0;
      }
    }

    if (currentState == STATE_PATTING) {
      unsigned long currentFrameDelay = PATPAT_FRAME_DELAY;
      if (currentMillis - lastAnimationFrame >= currentFrameDelay) {
        lastAnimationFrame = currentMillis;
        displayI2c.drawBitmap(0, 0, patpat_video_frames[animationFrame], SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
        displayI2c.display();
        animationFrame++;
        if (animationFrame >= PATPAT_TOTAL_FRAMES) animationFrame = 0;
      }
    } else if (currentState == STATE_EATING) {
      unsigned long currentFrameDelay = EAT_FRAME_DELAY;
      if (currentMillis - lastAnimationFrame >= currentFrameDelay) {
        lastAnimationFrame = currentMillis;
        displayI2c.drawBitmap(0, 0, eat_video_frames[animationFrame], SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
        displayI2c.display();
        animationFrame++;
        if (animationFrame >= EAT_TOTAL_FRAMES) animationFrame = 0;
      }
    } else if (currentState == STATE_STATS) {
      displayI2c.setCursor(0, 30);
      displayI2c.println("status kesehatan");
      displayI2c.display();
    } else if (currentState == STATE_DANCING) {
      unsigned long currentFrameDelay = DANCE_FRAME_DELAY;
      if (currentMillis - lastAnimationFrame >= currentFrameDelay) {
        lastAnimationFrame = currentMillis;
        displayI2c.drawBitmap(0, 0, dance_video_frames[animationFrame], SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
        displayI2c.display();
        animationFrame++;
        if (animationFrame >= DANCE_TOTAL_FRAMES) animationFrame = 0;
      }
    } else {
      unsigned long currentFrameDelay = MAIN_FRAME_DELAY;
      if (currentMillis - lastAnimationFrame >= currentFrameDelay) {
        lastAnimationFrame = currentMillis;
        
        displayI2c.drawBitmap(
            0,
            0,
            video_frames[animationFrame],
            SCREEN_WIDTH,
            SCREEN_HEIGHT, 
            SSD1306_WHITE
            );
            
        displayI2c.display();

        animationFrame++;
        if (animationFrame >= MAIN_TOTAL_FRAMES) {
          animationFrame = 0;
        }
      }
    }

    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

void TaskInput(void *pvParameters) {
  bool lastLeftState = HIGH;
  bool lastMiddleState = HIGH;
  bool lastRightState = HIGH;
  bool lastTouchState = HIGH;

  // Variabel Pelacak Double Tap Touch Sensor
  int touchCount = 0;
  TickType_t lastTouchTime = 0;
  const TickType_t doubleTapTimeout = 600 / portTICK_PERIOD_MS;

  for (;;) {
    // baca tombol
    bool leftState = digitalRead(btnLeft);
    bool middleState = digitalRead(btnMid);
    bool rightState = digitalRead(btnRight);
    bool touchState = digitalRead(touchPin);

    // BUTTON KIRI
    if (leftState == LOW && lastLeftState == HIGH) {
      Serial.println("menekan tombol kiri ");
      EventType ev = EV_EAT;
      xQueueSend(eventQueue, &ev, 0);
    }

    // BUTTON TENGAH
    if (middleState == LOW && lastMiddleState == HIGH) {
      Serial.println("menekan tombol tengah");
      EventType ev = EV_STATS;
      xQueueSend(eventQueue, &ev, 0);
    }

    // BUTTON KANAN
    if (rightState == LOW && lastRightState == HIGH) {
      Serial.println("menekan tombol kanan");
      EventType ev = EV_MUSIC;
      xQueueSend(eventQueue, &ev, 0);
      xQueueSend(audioQueue, &ev, 0);
    }

    // TOUCH SENSOR
    if (touchState == LOW && lastTouchState == HIGH) {
      TickType_t currentTime = xTaskGetTickCount();
      // Jika ini ketukan pertama atau jarak dari ketukan sebelumnya terlalu
      // lama
      if (touchCount == 0 || (currentTime - lastTouchTime > doubleTapTimeout)) {
        touchCount = 1;
        lastTouchTime = currentTime;
        Serial.println("[TOUCH] Tap 1... (Menunggu tap 2)");
      } else {
        // Ketukan kedua terdeteksi dalam batas waktu doubleTapTimeout
        touchCount++;
        if (touchCount == 2) {
          Serial.println("[TOUCH] Pat-pat terdeteksi! Memicu animasi.");
          touchCount = 0; // Reset counter
          EventType ev = EV_PAT;
          xQueueSend(eventQueue, &ev, 0);
        }
      }
    }

    // Reset counter ke 0 jika batas waktu menunggu ketukan kedua sudah habis
    if (touchCount > 0 &&
        (xTaskGetTickCount() - lastTouchTime > doubleTapTimeout)) {
      touchCount = 0;
    }

    // update state sebelumnya
    lastLeftState = leftState;
    lastMiddleState = middleState;
    lastRightState = rightState;
    lastTouchState = touchState;

    // debounce
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

void TaskAudio(void *pvParameters) {
#define NOTE_C4 262
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_AS4 466
#define NOTE_B4 494
#define NOTE_C5 523
  int melody[] = {NOTE_C4, NOTE_C4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_A4,
                  NOTE_G4, NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4,
                  NOTE_D4, NOTE_C4, NOTE_G4, NOTE_G4, NOTE_F4, NOTE_F4,
                  NOTE_E4, NOTE_E4, NOTE_D4, NOTE_G4, NOTE_G4, NOTE_F4,
                  NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4, NOTE_C4, NOTE_C4,
                  NOTE_G4, NOTE_G4, NOTE_A4, NOTE_A4, NOTE_G4, NOTE_F4,
                  NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4, NOTE_D4, NOTE_C4};
  // Durasi Nada (4 = Seperempat ketuk, 8 = Seperdelapan ketuk)
  int noteDurations[] = {4, 4, 4, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 2,
                         4, 4, 4, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 2,
                         4, 4, 4, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 2};
  for (;;) {
    EventType ev;
    if (xQueueReceive(audioQueue, &ev, 0) == pdTRUE) {
      if (ev == EV_MUSIC) {
        for (int thisNote = 0; thisNote < 42; thisNote++) {

          // Menghitung durasi nada (misal: 1000ms / 4 = 250ms)
          int noteDuration = 1000 / noteDurations[thisNote];

          // Mainkan nada menggunakan pin langsung
          ledcWriteTone(buzzerPin, melody[thisNote]);

          // Beri jeda sesuai durasi nada
          vTaskDelay(pdMS_TO_TICKS(noteDuration));

          // Hentikan nada dengan mengatur siklus aktif ke nol
          ledcWrite(buzzerPin, 0);
          vTaskDelay(pdMS_TO_TICKS(noteDuration * 0.3));
        }
      }
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}
