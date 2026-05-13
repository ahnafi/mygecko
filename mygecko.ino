#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C
#define SERIAL_BAUDRATE 115200
#define DHTTYPE DHT11

const int touchPin = 15;
const int btnLeft = 4;
const int btnMid = 5;
const int btnRight = 18;
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

void setup(){
    Serial.begin(SERIAL_BAUDRATE);

    displayI2c.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);  // INITIATE

    // task input
    pinMode(btnLeft, INPUT_PULLUP);
    pinMode(btnMid, INPUT_PULLUP);
    pinMode(btnRight, INPUT_PULLUP);
    pinMode(touchPin, INPUT_PULLUP);
    pinMode(buzzerPin, OUTPUT);

    displayI2c.clearDisplay();

    xTaskCreate(TaskDisplay, "Task Display", 6144, NULL, 1,NULL);
    xTaskCreate(TaskInput, "Task Input", 2048, NULL, 1, NULL);
    xTaskCreate(TaskAudio, "Task Audio", 2048, NULL, 1, NULL);
}

void loop(){}

void TaskDisplay(void *pvParameters){

    displayI2c.setTextSize(1);
    displayI2c.setTextColor(SSD1306_WHITE);

    for (;;) {

        displayI2c.clearDisplay();

        if(patEvent){

            displayI2c.setCursor(0, 30);
            displayI2c.println("PAT PAT");

            displayI2c.display(); 
            
            vTaskDelay(3000 / portTICK_PERIOD_MS);

            if(xTaskGetTickCount() - patEventStart >= 2000 / portTICK_PERIOD_MS){
                patEvent = false;
            }

        } else {

            displayI2c.setCursor(40, 30);
            displayI2c.println("MY GECKO");
            Serial.println("[info] test display");

            displayI2c.display();
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
              Serial.println("menekan tombol kiri (Navigasi Menu)");
            }

            // BUTTON TENGAH
            if (middleState == LOW && lastMiddleState == HIGH) {
              Serial.println("menekan tombol tengah (Konfirmasi Aksi)");
            }

            // BUTTON KANAN
            if (rightState == LOW && lastRightState == HIGH) {
              Serial.println("menekan tombol kanan (Batal)");
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
    for (;;) {
       digitalWrite(buzzerPin, HIGH);
       vTaskDelay(1000 / portTICK_PERIOD_MS);
       digitalWrite(buzzerPin, LOW);
       vTaskDelay(1000 / portTICK_PERIOD_MS);
       vTaskDelay(200 / portTICK_PERIOD_MS);
     }
}
