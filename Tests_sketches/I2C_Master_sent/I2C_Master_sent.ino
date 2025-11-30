#include <Wire.h> //master
#include <Arduino.h>

#define SLAVE_ADDR  0x11

// Команды
#define REG_LED     0x01 // 1 - on; 0 - off
#define REG_STRING  0x02
#define REG_COUNTER 0x03
#define REG_MILLIS  0x04

void writeLED(bool state) {
    Wire.beginTransmission(SLAVE_ADDR);
    Wire.write(REG_LED);
    Wire.write(state);
    Wire.endTransmission();
}

void sendStr(const char* str) {
    Wire.beginTransmission(SLAVE_ADDR);
    Wire.write(REG_STRING);
    Wire.print(str);
    Wire.endTransmission();
}

uint8_t getCounter() {
    Wire.beginTransmission(SLAVE_ADDR);
    Wire.write(REG_COUNTER);
    Wire.endTransmission(false);

    Wire.requestFrom(SLAVE_ADDR, 1);
    return Wire.read();
}

uint32_t getMillis() {
    Wire.beginTransmission(SLAVE_ADDR);
    Wire.write(REG_MILLIS);
    Wire.endTransmission(false);

    Wire.requestFrom(SLAVE_ADDR, 4);
    uint32_t ms;
    Wire.readBytes((uint8_t*)&ms, 4);
    return ms;
}


void setup() {
  Serial.begin(115200);
  Wire.begin();
}

void loop() {

    writeLED(1);
    delay(1000);

    writeLED(0);
    delay(1000);

    sendStr("hello");
    delay(1000);

    sendStr("from master");
    delay(1000);

    Serial.println(getCounter());
    delay(1000);

    Serial.println(getMillis());
    delay(1000);

  
}