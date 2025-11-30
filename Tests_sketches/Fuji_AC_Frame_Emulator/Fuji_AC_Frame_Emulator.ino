#include <avr/io.h>

uint32_t lastChange = 0;
uint8_t temp = 16; // начальная температура

// --------- отправка одного байта 500 бод 8E1 ---------
void uart0SendByte(uint8_t b) {
  while (!(UCSR0A & (1 << UDRE0)));
  UDR0 = b;
}

// --------- настройка UART0 (D1=TX, D0=RX) ---------
void uart0Init() {
  UBRR0H = 0x07;
  UBRR0L = 0xCF;
  UCSR0A = 0x00;
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00) | (1 << UPM01); // 8E1
  UCSR0B = (1 << TXEN0);   // только передатчик
}

byte frame[8] = {0x01, 0x20, 0xA0, 0x15, 0x00, 0x00, 0x00, 0x00};

void setup() {

  uart0Init();             // UART0 = 500 бод, **вывод на D1**
  
}

void loop() {

  if (millis() - lastChange > 3000) {
    lastChange = millis();
    temp = (temp >= 30) ? 16 : temp + 1; // 16→30 и обратно
    frame[4] = (temp << 0); // температура в битах. пишем просто в байт
  }

  // инвертируем для LIN
  for (byte &b : frame) b ^= 0xFF;

  // шлём кадр **только в UART0** (D1)
  for (byte b : frame) uart0SendByte(b);

  // вернем как было
  for (byte &b : frame) b ^= 0xFF;

  delay(1000);
}