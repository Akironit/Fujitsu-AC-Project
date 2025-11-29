#include <SoftwareSerial.h>

SoftwareSerial lin(2,3);   // то же самое

byte buf[8];
byte idx = 0;

void setup() {
  Serial.begin(9600);
  lin.begin(500);
  Serial.println("Slave: wait for frame...");
}

void loop() {
  while(lin.available()) {
    buf[idx++] = lin.read() ^ 0xFF;   // сразу раз-инвертируем
    if(idx == 8) {
      Serial.print("Receive: ");
      for(byte i=0;i<8;i++){
        if(buf[i]<0x10) Serial.print('0');
        Serial.print(buf[i],HEX); Serial.print(' ');
      }
      Serial.println();
      idx=0;
    }
  }
}