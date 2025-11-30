#include <Wire.h>         // мастер
#include "B:\Microcontrollers\Arduino_Projects\Fujitsu-AC-Project\Tests_sketches\Fuji_src\FujiHeatPump.cpp"

class Timer {
  public:
    enum TimeUnit { TU_MILLIS,
                    TU_MICROS 
    };
    Timer(unsigned long intervalMs, TimeUnit timeUnit)  // Конструктор
    : last_(0), interval_(intervalMs), running_(false), timeUnit_(timeUnit) {}

    void start() {
      resetLast();
      running_ = true;
    }
    void stop() {
      running_ = false;
    }
    void reset() {
      resetLast();
    }

    bool tick() {
      unsigned long now = (timeUnit_ == TU_MILLIS) ? millis() : micros();
      return tick(now);
    }

    bool tick(unsigned long now) {
    if (!running_) return false;
    unsigned long diff = now - last_;
    if (diff < interval_) return false;
    unsigned long count = diff / interval_;
    last_ += count * interval_;
    return true;
    }

    bool tick(unsigned long now, unsigned long interval) {
    if (!running_) return false;
    unsigned long diff = now - last_;
    if (diff < interval) return false;
    unsigned long count = diff / interval;
    last_ += count * interval;
    return true;
    }


  private:
    unsigned long last_;
    unsigned long interval_;
    bool running_;
    TimeUnit timeUnit_;

    void resetLast() {
      last_ = (timeUnit_ == TU_MILLIS) ? millis() : micros();
    }
};

// -=-=-=- DEFINE -=-=-=-
#define SLAVE_CALL_DELAY  500   // I2C
#define SLAVE_ADDR        0x42  //
#define FRAME_LEN         8     //
#define REG_MODE          0x01  // 
#define REG_FUJI_FRAME    0x02  //

// Битовые флаги статуса
#define STATUS_FRESH        (1 << 0)
#define STATUS_FRAME_ERR    (1 << 1)
#define STATUS_PARITY_ERR   (1 << 2)
#define STATUS_OVERRUN_ERR  (1 << 3)
#define STATUS_ACTIVITY     (1 << 4)
#define STATUS_NO_ACTIVITY  (1 << 5)

FujiHeatPump hp;
Timer timerSlaveCall = Timer(SLAVE_CALL_DELAY, Timer::TU_MILLIS);


volatile uint8_t buf[FRAME_LEN];     // из getFrame()
volatile uint8_t status;              // из getFrame()

void setup() {
  Wire.begin();        
  Serial.begin(115200);
  Serial.println("-=-=-=-=- Master: Start -=-=-=-=- ");

  timerSlaveCall.start();
}


void getFrame(bool isPrintInSerial) {
  /* Запрашивает у Slave 8 байт данных и 1 байт их свежести.
  Если данные уже запрашивались, а новых еще нет, то при след. запросе 
  свежесть будет = 0 */
  Wire.beginTransmission(SLAVE_ADDR);
  Wire.write(REG_FUJI_FRAME);
  Wire.endTransmission(false);

  Wire.requestFrom((uint8_t)SLAVE_ADDR, (uint8_t)9);
  Wire.readBytes((uint8_t*)&buf, 8);
  status = Wire.read();

  uint8_t debug_status = status & ~(STATUS_ACTIVITY | STATUS_FRESH);
  if (debug_status != 0) {
    Serial.println("-=-=-=-=-=- Error in receive data -=-=-=-=-=-");
    Serial.print("Code: ");
    Serial.println(status);
    if (status & STATUS_FRAME_ERR)    Serial.println("Ошибка в структуре кадра и/или <8 байт пришло");
    if (status & STATUS_PARITY_ERR)   Serial.println("Ошибка четности.");
    if (status & STATUS_OVERRUN_ERR)  Serial.println("Переполнение буфера!");
    if (status & STATUS_ACTIVITY)     Serial.println("Шина активна.");
    if (status & STATUS_NO_ACTIVITY)  Serial.println("Шина HE активна!");
  }

  if ((status == 1) && isPrintInSerial) {
    Serial.print("<-- ");
    for (uint8_t b : buf) {
      if (b < 0x10) Serial.print('0');  // если старшая цифра «0», печатаем её явно
      Serial.print(b, HEX);
      Serial.print(' ');
    }
    Serial.print("Fresh = ");
    Serial.print(status);
    Serial.println();
  }
}



void loop() {
  
  if (timerSlaveCall.tick()) {
    getFrame(false); // 8 байт кадра УЖЕ ИНВЕРТИРОВАНЫ, и 1 свежесть
  }

  if (status & STATUS_FRESH) {
    hp.setReadBuf((byte*)buf);          
    FujiFrame ff = hp.decodeFrame();  
    
    // Используем printFrame для отладки
    hp.printFrame((byte*)buf, ff);    // cast к byte* если нужно
    
    status = 0;  // сбросили флаг
  }


}
