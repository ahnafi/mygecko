// DOIT ESP 32 DEV KIT V1
// #include <Arduino_FreeRTOS.h> // untuk arduino avr uno
#include "DHT.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h> // untuk ESP32
#include <freertos/task.h>
#include <freertos/queue.h>

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

// Struktur Pesan Antrean Sensor
struct SensorData {
  float temperature;
  float humidity;
  int ldrValue;
};

// Handle Antrean Global
QueueHandle_t sensorQueue;

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

// Status Game Global
volatile bool isSleepy = false;
volatile int currentMenu = 0; // 0: Makan, 1: Tidur, 2: Nyanyi
volatile bool actionTriggered = false; // Flag trigger aksi dari TaskInput

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

  // Membuat Antrean Sensor
  sensorQueue = xQueueCreate(5, sizeof(SensorData));

  // task display
  // task ringan 2048, task berat 4096, ukuran memory
  xTaskCreate(TaskDisplay, "Task Display", 4096, NULL, 1,
              NULL); // stack 4096 berat
  xTaskCreate(TaskInput, "Task Input", 2048, NULL, 1, NULL);
  xTaskCreate(TaskSensor, "Task Sensor", 2048, NULL, 1, NULL);
  xTaskCreate(TaskAudio, "Task Audio", 2048, NULL, 1, NULL);
  xTaskCreate(TaskLogic, "Task Logic", 2048, NULL, 1, NULL);

  // scheduler sudah berjalan di ESP32 jadi fungsi vTaskStartScheduler tidak
  // diperlukan Menjalankan scheduler FreeRTOS Setelah ini, semua task akan
  // dijalankan oleh RTOS vTaskStartScheduler();
}

// kosong
void loop() {}

void TaskDisplay(void *pvParameters) {
  for (;;) {
    displayI2c.clearDisplay();
    displayI2c.setTextSize(1);
    displayI2c.setTextColor(SSD1306_WHITE);

    // Baris Status Atribut
    displayI2c.setCursor(0, 0);
    displayI2c.print("HP:"); displayI2c.print(health);
    displayI2c.print(" LAPAR:"); displayI2c.print(hunger);
    
    displayI2c.setCursor(0, 10);
    displayI2c.print("Mata: "); displayI2c.println(isSleepy ? "Ngantuk" : "Segar");

    // Render Menu Navigasi
    displayI2c.setCursor(0, 25);
    displayI2c.println("Menu Pilihan:");
    displayI2c.setCursor(10, 35);
    
    if (currentMenu == 0) displayI2c.println("> Ngasih Makan");
    else if (currentMenu == 1) displayI2c.println("> Tidur");
    else if (currentMenu == 2) displayI2c.println("> Bernyanyi");

    displayI2c.display();
    vTaskDelay(500 / portTICK_PERIOD_MS); // Refresh layar tiap 500ms
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
    bool leftState = digitalRead(btnNavigatePin); // misal tombol untuk navigasi
    bool middleState = digitalRead(btnConfirmPin); // misal tombol untuk confirm
    bool rightState = digitalRead(btnStatusPin);
    bool touchState = digitalRead(touchPin);

    // =========================
    // BUTTON KIRI (Navigate Menu)
    // =========================
    if (leftState == LOW && lastLeftState == HIGH) {
      Serial.println("menekan tombol kiri (Navigasi Menu)");
      currentMenu = (currentMenu + 1) % 3;
    }

    // =========================
    // BUTTON TENGAH (Confirm Action)
    // =========================
    if (middleState == LOW && lastMiddleState == HIGH) {
      Serial.println("menekan tombol tengah (Konfirmasi Aksi)");
      actionTriggered = true;
    }

    // =========================
    // BUTTON KANAN (Cancel / Batal)
    // =========================
    if (rightState == LOW && lastRightState == HIGH) {
      Serial.println("menekan tombol kanan (Batal)");
      actionTriggered = false; // Batalkan aksi jika belum terproses
      currentMenu = 0;         // Reset pilihan menu ke awal (opsional)
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
  // Pelacakan nilai historis untuk mendeteksi perubahan
  float lastSentTemp = -999.0;
  float lastSentHumid = -999.0;
  int lastSentLdr = -999;

  for (;;) {
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    int ldrValue = analogRead(ldrPin);

    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Gagal membaca sensor DHT!");
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }

    // Evaluasi Perubahan Data (Delta Thresholding)
    if (abs(temperature - lastSentTemp) > 0.2 || 
        abs(humidity - lastSentHumid) > 1.0 || 
        abs(ldrValue - lastSentLdr) > 50) {
      
      SensorData newData = {temperature, humidity, ldrValue};
      
      // Kirim ke antrean (non-blocking, tunggu maksimal 0 ticks)
      if (sensorQueue != NULL && xQueueSend(sensorQueue, &newData, 0) == pdPASS) {
        lastSentTemp = temperature;
        lastSentHumid = humidity;
        lastSentLdr = ldrValue;
        Serial.println("[QUEUE] Perubahan terdeteksi. Data dikirim ke antrean.");
      }
    }

    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

// ==========================================
// TUGAS LOGIKA UTAMA (GAME ENGINE)
// ==========================================
void TaskLogic(void *pvParameters) {
  SensorData localSensor = {25.0, 50.0, 0}; // Nilai bawaan awal
  TickType_t lastDecayTime = xTaskGetTickCount();
  const TickType_t decayInterval = 5000 / portTICK_PERIOD_MS; // Evaluasi tiap 5 detik

  for (;;) {
    // ----------------------------------------------------
    // A. PEMBACAAN ANTREAN SENSOR (QUEUE RECEIVE)
    // ----------------------------------------------------
    // Ambil data terbaru jika tersedia tanpa memblokir eksekusi (0 ticks delay)
    if (sensorQueue != NULL && xQueueReceive(sensorQueue, &localSensor, 0) == pdPASS) {
      // Data telah diupdate secara lokal
    }

    // ----------------------------------------------------
    // B. GAME RULES: Pengaruh Lingkungan (Sensor LDR)
    // ----------------------------------------------------
    // Nilai LDR 0-4095. Semakin gelap, nilai LDR umumnya semakin besar.
    if (localSensor.ldrValue > 3000) { 
      isSleepy = true;
    } else {
      isSleepy = false;
    }

    // ----------------------------------------------------
    // C. DECAY SYSTEM (Hunger & Health)
    // ----------------------------------------------------
    if (xTaskGetTickCount() - lastDecayTime >= decayInterval) {
      // 1. Hunger Decay
      if (hunger > 1) hunger--;

      // 2. Health Decay (Dipengaruhi Suhu & Kelembaban Ekstrem)
      if (localSensor.temperature > 35.0 || localSensor.temperature < 18.0) {
        if (health > 1) health -= 2; // Lingkungan buruk, HP cepat turun
      } else {
        if (hunger < 20 && health > 1) health--; // Lapar memicu turunnya health
      }
      
      Serial.print("[LOGIC] Hunger: "); Serial.print(hunger);
      Serial.print(" | Health: "); Serial.print(health);
      Serial.print(" | Mengantuk: "); Serial.println(isSleepy ? "Ya" : "Tidak");
      
      lastDecayTime = xTaskGetTickCount();
    }

    // ----------------------------------------------------
    // D. PROSES ACTION (Menu Pemicu)
    // ----------------------------------------------------
    if (actionTriggered) {
      if (currentMenu == 0) {
        hunger = (hunger + 20 > 100) ? 100 : hunger + 20;
        Serial.println("[AKSI] Memberi Makan! Hunger meningkat.");
      } else if (currentMenu == 1) {
        health = (health + 20 > 100) ? 100 : health + 20;
        Serial.println("[AKSI] Asisten Tidur. Health membaik.");
      } else if (currentMenu == 2) {
        Serial.println("[AKSI] Bernyanyi! Mengaktifkan Buzzer.");
      }
      actionTriggered = false; // Reset flag aksi
    }

    vTaskDelay(200 / portTICK_PERIOD_MS); // Siklus pemrosesan RTOS
  }
}

// TUGAS KELUARAN AUDIO / BUZZER
void TaskAudio(void *pvParameters) {
  for (;;) {
    digitalWrite(buzzerPin, HIGH);
    vTaskDelay(100 / portTICK_PERIOD_MS); // Bunyi 100ms

    digitalWrite(buzzerPin, LOW);
    vTaskDelay(100 / portTICK_PERIOD_MS); // Jeda 100ms

    digitalWrite(buzzerPin, HIGH);
    vTaskDelay(100 / portTICK_PERIOD_MS); // Bunyi 100ms

    digitalWrite(buzzerPin, LOW);
    
    // Jeda panjang sebelum mengulang melodi/pola suara (misal setiap 5 detik)
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}
