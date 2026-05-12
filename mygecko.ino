// DOIT ESP 32 DEV KIT V1
// #include <Arduino_FreeRTOS.h> // untuk arduino avr uno
#include "DHT.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h> // untuk ESP32
#include <freertos/task.h>

// defining variabel variabel alias
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define SCREEN_ADDRESS 0x3C
#define SERIAL_BAUDRATE 115200
#define DHTTYPE DHT11

// sensor
const int touchPin = 5;
const int btnNavigatePin = 27;
const int btnConfirmPin = 26;
const int btnStatusPin = 25;
const int buzzerPin = 14;
const int ldrPin = 13;
const int dhtPin = 4;

// INITIATE OBJECT VARIABEL and ...
DHT dht(dhtPin, DHTTYPE);

// LCD I2C 0x3C
Adafruit_SSD1306 displayI2c =
    Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

// define fungsi
void TaskDisplay(void *pvParameters);
void TaskInput(void *pvParameters);
void TaskLogic(void *pvParameters);
void TaskSensor(void *pvParameters);
void TaskAudio(void *pvParameters);

// variabel global

// hunger 0 ... 100
volatile int hunger = 100;
volatile int health = 100;

// menu
// kasi makan
// kasi obat
// tidur (menambah kesehatan, menjadi lapar)

void setup() {
  Serial.begin(SERIAL_BAUDRATE);

  displayI2c.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);

  // INITIATE
  // task input
  pinMode(btnNavigatePin, INPUT_PULLUP);
  pinMode(btnConfirmPin, INPUT_PULLUP);
  pinMode(btnStatusPin, INPUT_PULLUP);

  pinMode(touchPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  dht.begin();

  // task display
  // task ringan 2048, task berat 4096, ukuran memory
  xTaskCreate(TaskDisplay, "Task Display", 4096, NULL, 1,
              NULL); // stack 4096 berat
  xTaskCreate(TaskInput, "Task Input", 2048, NULL, 1, NULL);
  xTaskCreate(TaskSensor, "Task Sensor", 2048, NULL, 1, NULL);

  // scheduler sudah berjalan di ESP32 jadi fungsi vTaskStartScheduler tidak
  // diperlukan Menjalankan scheduler FreeRTOS Setelah ini, semua task akan
  // dijalankan oleh RTOS vTaskStartScheduler();
}

// kosong
void loop() {}

void TaskDisplay(void *pvParameters) {
  for (;;) {
    displayI2c.clearDisplay();

    displayI2c.setTextSize(2);
    displayI2c.setTextColor(SSD1306_WHITE);

    displayI2c.setCursor(10, 20);
    displayI2c.println("Hello");

    displayI2c.setCursor(10, 45);
    displayI2c.println("World!");

    displayI2c.display();

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// TASK INPUT
void TaskInput(void *pvParameters) {

  bool lastLeftState = HIGH;
  bool lastMiddleState = HIGH;
  bool lastRightState = HIGH;
  bool lastTouchState = HIGH;

  for (;;) {

    // baca tombol
    bool leftState = digitalRead(btnNavigatePin);
    bool middleState = digitalRead(btnConfirmPin);
    bool rightState = digitalRead(btnStatusPin);
    bool touchState = digitalRead(touchPin);

    // =========================
    // BUTTON KIRI
    // =========================
    if (leftState == LOW && lastLeftState == HIGH) {
      Serial.println("menekan tombol kiri");
    }

    // =========================
    // BUTTON TENGAH
    // =========================
    if (middleState == LOW && lastMiddleState == HIGH) {
      Serial.println("menekan tombol tengah");
    }

    // =========================
    // BUTTON KANAN
    // =========================
    if (rightState == LOW && lastRightState == HIGH) {
      Serial.println("menekan tombol kanan");
    }

    // =========================
    // TOUCH SENSOR
    // =========================
    if (touchState == LOW && lastTouchState == HIGH) {
      Serial.println("touch sensor disentuh");
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

void TaskSensor(void *pvParameters) {
  for (;;) {
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    int ldrValue = analogRead(ldrPin);

    // Validasi data
    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Gagal membaca sensor!");
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }

    Serial.print("Kelembapan: ");
    Serial.print(humidity);
    Serial.print(" % | ");
    Serial.print("Suhu: ");
    Serial.print(temperature);
    Serial.print(" C | ");
    Serial.print("Cahaya (LDR): ");
    Serial.println(ldrValue);

    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}
