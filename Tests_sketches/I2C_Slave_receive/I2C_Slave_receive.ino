#include <Arduino.h>
#include <Wire.h>  //slave

#define SLAVE_ADDR 0x11

// команды
#define REG_LED 0x01
#define REG_STRING 0x02
#define REG_COUNTER 0x03
#define REG_MILLIS 0x04
#define LED_PIN 3

// последняя выбранная команда
// в обработчике приёма
uint8_t cmd = 0;

// счётчик сообщений
uint8_t counter = 0;

// обработчик приёма
void receiveCb(int amount) {
    cmd = Wire.read();
    ++counter;
    switch (cmd) {
        case REG_LED:
            digitalWrite(LED_PIN, Wire.read());
            break;
        case REG_STRING:
            while (Wire.available()) {
              Serial.print((char)Wire.read());
            }
            Serial.println();
            break;
        case REG_COUNTER: break;
        case REG_MILLIS: break;
    }
}

// обработчик запроса
void requestCb() {
    switch (cmd) {
        case REG_LED: break;
        case REG_STRING: break;
        case REG_COUNTER:
            Wire.write(counter);
            break;
        case REG_MILLIS:
            {
              uint32_t ms = millis();
              Wire.write((uint8_t*)&ms, 4);
            }
            break;
    }
}

void setup() {
    Serial.begin(115200);
    Wire.begin(SLAVE_ADDR);
    Wire.onReceive(receiveCb);
    Wire.onRequest(requestCb);
    pinMode(LED_PIN, OUTPUT);
}

void loop() {
}