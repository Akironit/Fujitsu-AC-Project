#include <Wire.h>
#include <avr/io.h>

constexpr uint8_t I2C_ADDR = 0x42; // наш адрес на шине
constexpr uint8_t FRAME_LEN = 8;
constexpr uint32_t FRAME_TIMEOUT_MS = 50;  // если между байтами > 50 мс – кадр протух
constexpr uint32_t ACTIVITY_TIMEOUT_MS = 3500; // таймаут "шина пуста"

// Команды
#define REG_MODE 0x01
#define REG_FUJI_FRAME 0x02

// Битовые флаги статуса
#define STATUS_FRESH        (1 << 0)
#define STATUS_FRAME_ERR    (1 << 1)
#define STATUS_PARITY_ERR   (1 << 2)
#define STATUS_OVERRUN_ERR  (1 << 3)
#define STATUS_ACTIVITY     (1 << 4)
#define STATUS_NO_ACTIVITY  (1 << 5)

volatile uint8_t frameBuf[FRAME_LEN];  // последний кадр
volatile uint8_t frameStatus = 0;      // 9-й байт (битовая маска)
volatile uint8_t cmd = 0;

/* ---------- аппаратный UART0 500 бод 8E1 ---------- */
void uart0Init()
{
  UBRR0H = 0x07; // 500 бод при 16 МГц
  UBRR0L = 0xCF;
  UCSR0A = 0x00;                                         // без удвоения скорости
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00) | (1 << UPM01); // 8E1
  UCSR0B = (1 << RXEN0);                                 // только приём
}

/* ---------- проверка ошибок USART ---------- */
uint8_t readByteWithErrorCheck() {
  // ждём байт
  while (!(UCSR0A & (1 << RXC0)));
  
  // Проверяем ошибки (до чтения UDR!)
  if (UCSR0A & (1 << FE0))  frameStatus  |= STATUS_FRAME_ERR;   // Framing Error
  if (UCSR0A & (1 << DOR0)) frameStatus  |= STATUS_OVERRUN_ERR; // Data Overrun
  if (UCSR0A & (1 << UPE0)) frameStatus  |= STATUS_PARITY_ERR;  // Parity Error
  
  return UDR0;
}


/* ---------- I²C ---------- */
void i2cReceive(int amount)
{
  cmd = Wire.read();
}

void i2cRequest()
{
  switch (cmd)
  {
    case REG_MODE:
      break;

    case REG_FUJI_FRAME:
        Wire.write((uint8_t *)frameBuf, FRAME_LEN);
        Wire.write(frameStatus);      // отправляем битовую маску
        frameStatus &= ~STATUS_FRESH; // сбрасываем флаг свежести (остальные флаги сохраняем)
      break;

    default:
      break;
  }
  
}

/* ---------- таймеры ---------- */
uint32_t lastByteMillis = 0;      // время последнего байта
uint32_t lastActivityCheckMillis = 0; // время последней проверки активности

/* ---------- setup ---------- */
void setup()
{
  uart0Init();
  Wire.begin(I2C_ADDR); // слейв на I²C
  Wire.onReceive(i2cReceive);
  Wire.onRequest(i2cRequest);   // вызовут, когда мастер спросит
  pinMode(LED_BUILTIN, OUTPUT); // мигаем при кадре
}

/* ---------- главный цикл: собираем кадр ---------- */
uint8_t idx = 0;
uint8_t tmp[FRAME_LEN];

void loop()
{
  // === 1. Таймаут сборки кадра (если байт не пришёл 50 мс) ===
  if (idx > 0 && (millis() - lastByteMillis > FRAME_TIMEOUT_MS)) {
    frameStatus |= STATUS_FRAME_ERR;  // помечаем ошибку
    idx = 0;                          // сбрасываем сборку
  }
  
  // === 2. Таймаут активности (если 3 секунды ничего не было) ===
  if (millis() - lastActivityCheckMillis > ACTIVITY_TIMEOUT_MS) {
    lastActivityCheckMillis = millis();
    if (!(frameStatus & STATUS_ACTIVITY)) {
      // За последние 3 секунды не было ни одного байта
      frameStatus |= STATUS_NO_ACTIVITY;
    }
    // Сбрасываем флаг активности (в следующем цикле его поднимем, если будут данные)
    frameStatus &= ~STATUS_ACTIVITY;
  }
  
  // === 3. Приём байта ===
  if (UCSR0A & (1 << RXC0)) {
    lastByteMillis = millis();               // отметили время
    frameStatus |= STATUS_ACTIVITY;          // поднимаем флаг активности
    frameStatus &= ~STATUS_NO_ACTIVITY;      // сбрасываем "шина пуста"
    
    uint8_t b = readByteWithErrorCheck();    // читаем с проверкой ошибок
    
    // Синхронизация: ищем 0x01 как первый байт
    if (b == 0x01) idx = 0;
    
    // Сохраняем байт, если есть место
    if (idx < FRAME_LEN) {
      tmp[idx++] = b ^ 0xFF;                 // инвертируем LIN-уровни
    }
    
    // Кадр собран
    if (idx == FRAME_LEN) {
      memcpy((void*)frameBuf, tmp, FRAME_LEN);
      frameStatus = 0;
      frameStatus |= STATUS_ACTIVITY;
      frameStatus |= STATUS_FRESH;           // поднимаем флаг свежести
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      idx = 0;                               // готовимся к следующему кадру
    }
  }
}