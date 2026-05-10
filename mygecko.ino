// DOIT ESP 32 DEV KIT V1
#include <Arduino_FreeRTOS.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SDD1306.h>
#include "DHT.h"

// defining variabel variabel alias
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define SCREEN_ADDRESS 0x3C
#define SERIAL_BAUDRATE 11
#define DHTTYPE DHT11

// sensor
const int touchPin = 2;
const int btnNavigatePin = 27;
const int btnConfirmPin = 26;
const int btnStatusPin = 25;
const int buzzerPin = 14;
const int ldrPin = 13;
const int dhtPin = 12;

// INITIATE OBJECT VARIABEL and ...
DHT dht(dhtPin,DHTTYPE);

// LCD I2C 0x3C
Adafruit_SDD1306 displayI2c = Adafruit_SDD1306(SCREEN_HEIGHT, SCREEN_WIDTH, &Wire);

// define fungsi
# funsi menampilkan lcd i2c
void Display(*pvParameters);
void Menu(*pvParameters);
void Feeding(*pvParameters);

// variabel global

// hunger 0 ... 100
volatile int hunger = 100;
volatile int health = 100;

// menu
// kasi makan
// kasi obat
// tidur (menambah kesehatan, menjadi lapar)

void setup(){
    Serial.begin(SERIAL_BAUDRATE);

    displayI2c.begin(SDD1306_SWITCHCAPVCC, SCREEN_ADDRESS);

    // task display
    xTaskCreate(Display,"Display", 128, NULL, 1, NULL);

    // Menjalankan scheduler FreeRTOS
    // Setelah ini, semua task akan dijalankan oleh RTOS
    vTaskStartScheduler();
}

// kosong
void loop(){}

// Fungsi fungsi task
void Display(*pvParameters){
    for(;;){
       display.clearDisplay();         //this line to clear previous logo
       display.setTextColor(WHITE);    //without this no display
       display.print("Hello World!");  //your TEXT here
       display.display();              //to shows or update your TEXT
   }
}

void Menu (){

}

void Feeding(*pvParameters){
    for(;;){
        if (
        //condition
        ){
            Serial.println("memberi makan")
        }
    }
}


void Sleep(*pvParameters){
    for(;;){
        if (
        //condition
        ){
            Serial.println("tidur")
        }
    }
}

void Medicine(*pvParameters){
    for(;;){
            if (
            //condition
            ){
                Serial.println("memberi obat")
            }
        }
}


void PatPat(*pvParameters){
    for(;;){
            if (
            //condition
            ){
                Serial.println("memberi pat pat")
            }
        }
}
