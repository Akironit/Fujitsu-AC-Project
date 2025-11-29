#include <Wire.h>

constexpr uint8_t SLAVE_ADDR = 0x42;
constexpr uint8_t FRAME_LEN  = 8;

uint8_t buf[FRAME_LEN];
uint8_t fresh;

void setup() {
  Wire.begin();        // мастер
  Serial.begin(115200);
  Serial.println("Master: старт");
}

void loop() {
  // запрашиваем 9 байт
  Wire.requestFrom(SLAVE_ADDR, uint8_t(FRAME_LEN + 1));
  if (Wire.available() == 9) {
    //Serial.println("Есть ответ на 9 байт!");
    for (uint8_t i = 0; i < FRAME_LEN; ++i) buf[i] = Wire.read();
    fresh = Wire.read();

     if (fresh) {
      Serial.print("<-- ");
      for (uint8_t b : buf) {
        if (b < 0x10) Serial.print('0');
        Serial.print(b, HEX);
        Serial.print(' ');
      }
      Serial.println();
     }
  } else {
    Serial.println("I²C ошибка длины");
  }

  delay(300);   // опрос
}