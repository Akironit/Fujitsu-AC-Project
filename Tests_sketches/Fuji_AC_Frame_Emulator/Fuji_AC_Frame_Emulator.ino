#include <SoftwareSerial.h>

SoftwareSerial lin(2,3);   // RX=2, TX=3

void setup() {
  Serial.begin(9600);      // USB-отладка
  lin.begin(500);          // 500 бод, 8N1 (SoftwareSerial чётность не умеет,
                           // но мы сами инвертируем байты → получим 8E1)
  Serial.println("Emulator: start");
}

byte frame[8] = {0x01, 0x20, 0xA0, 0x15, 0x00, 0x00, 0x00, 0x00};

void loop() {
  for(byte &b : frame) b ^= 0xFF;   // инвертируем
  lin.write(frame, 8);
  lin.flush();
  for(byte &b : frame) b ^= 0xFF;   // возвращаем обратно (для печати)
  
  Serial.print("Send: ");
  for(byte b : frame) {
    if(b<0x10) Serial.print('0');
    Serial.print(b,HEX); Serial.print(' ');
  }
  Serial.println();
  delay(1000);
}
