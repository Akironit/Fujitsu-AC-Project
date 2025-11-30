#include <Wire.h>
#include <avr/io.h>          // регистры UART0

constexpr uint8_t I2C_ADDR   = 0x42;   // наш адрес на шине
constexpr uint8_t FRAME_LEN  = 8;

volatile bool     frameFresh = false;  // флаг «новый кадр есть»
volatile uint8_t  frameBuf[FRAME_LEN]; // последний кадр (уже раз-инвертирован)
volatile bool i2cCalled = false;

/* ---------- аппаратный UART0 500 бод 8E1 ---------- */
void uart0Init() {
  UBRR0H = 0x07;                       // 500 бод при 16 МГц
  UBRR0L = 0xCF;
  UCSR0A = 0x00;                       // без удвоения скорости
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00) | (1 << UPM01); // 8E1
  UCSR0B = (1 << RXEN0);               // только приём
}

/* ---------- I²C: отдаём 9 байт (8 данных + флаг свежести) ---------- */
void i2cRequest() {
  Wire.write((uint8_t*)frameBuf, FRAME_LEN);
  Wire.write(frameFresh ? 1 : 0);    // 9-й байт
  frameFresh = false;                // сбросили флаг
}

/* ---------- setup ---------- */
void setup() {
  uart0Init();
  Wire.begin(I2C_ADDR);              // слейв на I²C
  Wire.onRequest(i2cRequest);        // вызовут, когда мастер спросит
  pinMode(LED_BUILTIN, OUTPUT);      // мигаем при кадре
}

/* ---------- главный цикл: собираем кадр ---------- */
uint8_t idx = 0;
uint8_t tmp[FRAME_LEN];

void loop() {
  if (UCSR0A & (1 << RXC0)) {
    uint8_t b = UDR0 ^ 0xFF;      // раз-инвертируем

    if (b == 0x01) idx = 0;       // нашли начало – сброс
    tmp[idx++] = b;

    if (idx == FRAME_LEN) {       // кадр готов
      memcpy((void*)frameBuf, tmp, FRAME_LEN);
      frameFresh = true;
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      idx = 0;                    // сразу ждём следующий 0x01
    }
  }

  static uint32_t t;
  if (millis() - t > 200) {   // 200 мс – «я жив»
    t = millis();
    digitalWrite(LED_BUILTIN, i2cCalled ? HIGH : LOW );
    i2cCalled = false;
  }

}