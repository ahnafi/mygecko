#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <MainDisplay.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C
#define SERIAL_BAUDRATE 115200
#define DHTTYPE DHT11

const int touchPin = 15;
const int btnLeft = 5;
const int btnMid = 18;
const int btnRight = 19;
const int buzzerPin = 23;

Adafruit_SSD1306 displayI2c = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

// define fungsi
void TaskDisplay(void *pvParameters);
void TaskInput(void *pvParameters);
void TaskAudio(void *pvParameters);

// var global
// pat pat event
volatile bool patEvent = false;
TickType_t patEventStart = 0;
// eating
volatile bool eatEvent = false;
TickType_t eatEventStart = 0;
// stats
volatile bool statsEvent = false;
TickType_t statsEventStart = 0;
// soundboard
volatile bool soundBoardEvent = false;

// Variabel Wajib dari Spesifikasi Tugas
volatile bool playAnimation = false;
volatile int animationFrame = 0;
unsigned long lastAnimationFrame = 0;

void setup(){
    Serial.begin(SERIAL_BAUDRATE);

    displayI2c.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);  // INITIATE

    // task input
    pinMode(btnLeft, INPUT_PULLUP);
    pinMode(btnMid, INPUT_PULLUP);
    pinMode(btnRight, INPUT_PULLUP);
    pinMode(touchPin, INPUT_PULLUP);
    // pinMode(buzzerPin, OUTPUT);
    ledcAttach(buzzerPin, 5000, 8);

    displayI2c.clearDisplay();

    xTaskCreate(TaskDisplay, "Task Display", 6144, NULL, 1,NULL);
    xTaskCreate(TaskInput, "Task Input", 2048, NULL, 1, NULL);
    xTaskCreate(TaskAudio, "Task Audio", 2048, NULL, 1, NULL);
}

void loop(){}

void TaskDisplay(void *pvParameters){

    displayI2c.setTextSize(1);
    displayI2c.setTextColor(SSD1306_WHITE);
    lastAnimationFrame = millis();

    for (;;) {
        unsigned long currentMillis = millis();
        displayI2c.clearDisplay();

        if(patEvent){

            displayI2c.setCursor(0, 30);
            displayI2c.println("PAT PAT");

            displayI2c.display();

            vTaskDelay(3000 / portTICK_PERIOD_MS);

            if(xTaskGetTickCount() - patEventStart >= 2000 / portTICK_PERIOD_MS){
                patEvent = false;
            }

        }  else if(eatEvent){
            displayI2c.setCursor(0, 30);
            displayI2c.println("Eats nyam nyam");

            displayI2c.display();

            vTaskDelay(2000 / portTICK_PERIOD_MS);

            if(xTaskGetTickCount() - eatEventStart >= 2000 / portTICK_PERIOD_MS){
                eatEvent = false;
            }
        }else if (statsEvent) {
            displayI2c.setCursor(0, 30);
                       displayI2c.println("status kesehatan");

                       displayI2c.display();

                       vTaskDelay(2000 / portTICK_PERIOD_MS);

                       if(xTaskGetTickCount() - statsEventStart >= 2000 / portTICK_PERIOD_MS){
                           statsEvent = false;
                       }
        }else {
            unsigned long currentFrameDelay = MAIN_FRAME_DELAY;
                 if (currentMillis - lastAnimationFrame >= currentFrameDelay) {
                   lastAnimationFrame = currentMillis;

                   // if (isPatpatAnimation) {
                   //   displayI2c.drawBitmap(0, 0, patpat[animationFrame], SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
                   //   displayI2c.display();

                   //   animationFrame++;
                   //   if (animationFrame >= PATPAT_TOTAL_FRAMES) {
                   //     animationFrame = 0;
                   //     isPatpatAnimation = false;
                   //     playAnimation = false;
                   //   }
                   // } else {
                     displayI2c.drawBitmap(0, 0, video_frames[animationFrame], SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
                     displayI2c.display();

                     animationFrame++;
                     if (animationFrame >= MAIN_TOTAL_FRAMES) {
                       animationFrame = 0;
                     }
                   }
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void TaskInput(void *pvParameters){
    for (;;) {
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
              eatEvent = true;
              eatEventStart = xTaskGetTickCount();
            }

            // BUTTON TENGAH
            if (middleState == LOW && lastMiddleState == HIGH) {
              Serial.println("menekan tombol tengah");
              statsEvent = true;
              statsEventStart = xTaskGetTickCount();
            }

            // BUTTON KANAN
            if (rightState == LOW && lastRightState == HIGH) {
                soundBoardEvent = true;
              Serial.println("menekan tombol kanan");
            }

            // TOUCH SENSOR
            if (touchState == LOW && lastTouchState == HIGH) {
              TickType_t currentTime = xTaskGetTickCount();
              // Jika ini ketukan pertama atau jarak dari ketukan sebelumnya terlalu lama
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
                  patEvent = true;
                  patEventStart= xTaskGetTickCount();
                }
              }
            }

            // Reset counter ke 0 jika batas waktu menunggu ketukan kedua sudah habis
            if (touchCount > 0 && (xTaskGetTickCount() - lastTouchTime > doubleTapTimeout)) {
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
}

void TaskAudio(void *pvParameters){
    #define NOTE_C4  262
    #define NOTE_D4  294
    #define NOTE_E4  330
    #define NOTE_F4  349
    #define NOTE_G4  392
    #define NOTE_A4  440
    #define NOTE_AS4 466
    #define NOTE_B4  494
    #define NOTE_C5  523
    int melody[] = {
      NOTE_C4, NOTE_C4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_A4, NOTE_G4,
      NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4, NOTE_D4, NOTE_C4,
      NOTE_G4, NOTE_G4, NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4,
      NOTE_G4, NOTE_G4, NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4,
      NOTE_C4, NOTE_C4, NOTE_G4, NOTE_G4, NOTE_A4, NOTE_A4, NOTE_G4,
      NOTE_F4, NOTE_F4, NOTE_E4, NOTE_E4, NOTE_D4, NOTE_D4, NOTE_C4
    };
    // Durasi Nada (4 = Seperempat ketuk, 8 = Seperdelapan ketuk)
    int noteDurations[] = {
      4, 4, 4, 4, 4, 4, 2,
      4, 4, 4, 4, 4, 4, 2,
      4, 4, 4, 4, 4, 4, 2,
      4, 4, 4, 4, 4, 4, 2,
      4, 4, 4, 4, 4, 4, 2,
      4, 4, 4, 4, 4, 4, 2
    };
    for (;;) {
       if (soundBoardEvent){
           for (int thisNote = 0; thisNote < 42; thisNote++) {

               // Menghitung durasi nada (misal: 1000ms / 4 = 250ms)
               int noteDuration = 1000 / noteDurations[thisNote];

               // Mainkan nada
               ledcWriteTone(buzzerPin, melody[thisNote]);

               // Beri jeda sesuai durasi nada
               vTaskDelay(noteDuration);

               // Hentikan nada sejenak agar antar nada terdengar terpisah
               ledcWriteTone(buzzerPin, 0);
               vTaskDelay(noteDuration * 0.30);
             }
           soundBoardEvent = false;
       }

      vTaskDelay(200 / portTICK_PERIOD_MS);
     }
}
