#include <HardwareSerial.h>
#include <Arduino.h>


const byte kModeIndex = 3;
const byte kModeMask = 0b00001110;
const byte kModeOffset = 1;

const byte kFanIndex = 3;
const byte kFanMask = 0b01110000;
const byte kFanOffset = 4;

const byte kEnabledIndex = 3;
const byte kEnabledMask = 0b00000001;
const byte kEnabledOffset = 0;

const byte kErrorIndex = 3;
const byte kErrorMask = 0b10000000;
const byte kErrorOffset = 7;

const byte kEconomyIndex = 4;
const byte kEconomyMask = 0b10000000;
const byte kEconomyOffset = 7;

const byte kTemperatureIndex = 4;
const byte kTemperatureMask = 0b01111111;
const byte kTemperatureOffset = 0;

const byte kUpdateMagicIndex = 5;
const byte kUpdateMagicMask = 0b11110000;
const byte kUpdateMagicOffset = 4;

const byte kSwingIndex = 5;
const byte kSwingMask = 0b00000100;
const byte kSwingOffset = 2;

const byte kSwingStepIndex = 5;
const byte kSwingStepMask = 0b00000010;
const byte kSwingStepOffset = 1;

const byte kControllerPresentIndex = 6;
const byte kControllerPresentMask = 0b00000001;
const byte kControllerPresentOffset = 0;

const byte kControllerTempIndex = 6;
const byte kControllerTempMask = 0b01111110;
const byte kControllerTempOffset = 1;


typedef struct FujiFrames  {
    byte onOff = 0;             // 0 = выключен, 1 = включен
    byte temperature = 16;      // Температура (градусы Цельсия)
    byte acMode = 0;            // Режим работы (0 = FAN, 1 = DRY, 2 = COOL, 3 = HEAT, 4 = AUTO)
    byte fanMode = 0;           // Скорость вентилятора (0-4: 0 = AUTO, 1 = QUIET, 2 = LOW, 3 = MEDIUM, 4 = HIGH)
    byte acError = 0;           // 0 = нет ошибки, 1 = есть ошибка
    byte economyMode = 0;       // 0 = обычный режим, 1 = экономный
    byte swingMode = 0;         // 0 = без раскачки, 1 = есть раскачка жалюзи
    byte swingStep = 0;         // Шаг раскачки (0/1)
    byte controllerPresent = 0; // 0 = пульта нет, 1 = пульт есть
    byte updateMagic = 0; // unsure what this value indicates
    byte controllerTemp = 16;   // Температура, измеренная пультом

    bool writeBit = false;      // 1 = мы хотим что-то изменить
    bool loginBit = false;      // 1 = кадр-авторизация
    bool unknownBit = false; // unsure what this bit indicates

    byte messageType = 0;       // Тип сообщения (0 = STATUS (обычный ответ), 1 = ERROR (ошибка), 2 = LOGIN (авторизация), 3 = UNKNOWN)
    byte messageSource = 0;     // Кто отправил (PRIMARY=32, SECONDARY=33)
    byte messageDest = 0;       // Кому отправлено (UNIT=1, PRIMARY=32, SECONDARY=33)
} FujiFrame;  

class FujiHeatPump
{
  private:
    Stream *_serial;          // УКАЗАТЕЛЬ на аппаратный UART
    byte            readBuf[8];       // Массив 8 байт для входящего кадра
    byte            writeBuf[8];      // Массив 8 байт для исходящего кадра
    
    byte            controllerAddress;                  // Наш адрес: 32 (PRIMARY) или 33 (SECONDARY)
    bool            controllerIsPrimary = true;         // true = PRIMARY; false = SECONDARY
    bool            seenSecondaryController = false;    // true = оригинальный пульт отвечает, мы вторичны
    bool            controllerLoggedIn = false;         // true = мы успешно авторизовались
    unsigned long   lastFrameReceived;                  // millis() последнего кадра (для таймаута)
    
    byte            updateFields;     // Битовая маска: какие поля менять
    FujiFrame       updateState;      // Что мы ХОТИМ изменить
    FujiFrame       currentState;     // Копия последнего кадра (текущее состояние)

    // FujiFrame decodeFrame();                    // работает с сырыми 8 байтами
    // void encodeFrame(FujiFrame ff);             // Обратно: структура → сырые 8 байт writeBuf
    // void printFrame(byte buf[8], FujiFrame ff); // Красивый вывод в Serial (для отладки)
    
    bool pendingFrame = false;  // true = в writeBuf лежит готовый кадр, надо отправить
  
  public:

    FujiFrame decodeFrame();                    
    void encodeFrame(FujiFrame ff);             
    void printFrame(byte buf[8], FujiFrame ff); 

    void connect(Stream *serial, bool secondary);   // Подключаемся к любому Serial (Serial1, Serial2, …) и выбираем роль (true = вторичный)
    void connect(Stream *serial, bool secondary, int rxPin, int txPin); // переназначаем пины (только ESP32)
    void connect(bool secondary); // инициализация без Serial

    bool waitForFrame();          // Дождаться кадра, вернуть true если пришёл
    void sendPendingFrame();      // Отправить подготовленный кадр
    bool isBound();               // true = мы в сети (приходили кадры < 1 с)
    bool updatePending();         // true = есть изменения, которые ещё не ушли

    void setReadBuf(const byte* newReadBuf);

    void setOnOff(bool o);        // Записать новые значения, будут отправлены со следующим кадром
    void setTemp(byte t);         
    void setMode(byte m);         
    void setFanMode(byte fm);     
    void setEconomyMode(byte em); 
    void setSwingMode(byte sm);   
    void setSwingStep(byte ss);   
    
    bool getOnOff();              // Прочитать состояния и значения из пришедшего кадра
    byte getTemp();
    byte getMode();
    byte getFanMode();
    byte getEconomyMode();
    byte getSwingMode();
    byte getSwingStep();
    byte getControllerTemp();
    
    FujiFrame *getCurrentState();
    FujiFrame *getUpdateState();
    byte getUpdateFields();
    
    bool debugPrint = false;
    
};

enum class FujiMode : byte {
  UNKNOWN = 0,
  FAN     = 1,
  DRY     = 2,
  COOL    = 3,
  HEAT    = 4,
  AUTO    = 5
};

enum class FujiMessageType : byte {
  STATUS  = 0,
  ERROR   = 1,
  LOGIN   = 2,
  UNKNOWN = 3,
};

enum class FujiAddress : byte {
  START       = 0,
  UNIT        = 1,
  PRIMARY     = 32,
  SECONDARY   = 33,
};

enum class FujiFanMode : byte {
  FAN_AUTO      = 0,
  FAN_QUIET     = 1,
  FAN_LOW       = 2,
  FAN_MEDIUM    = 3,
  FAN_HIGH      = 4
};

const byte kOnOffUpdateMask       = 0b10000000;
const byte kTempUpdateMask        = 0b01000000;
const byte kModeUpdateMask        = 0b00100000;
const byte kFanModeUpdateMask     = 0b00010000;
const byte kEconomyModeUpdateMask = 0b00001000;
const byte kSwingModeUpdateMask   = 0b00000100;
const byte kSwingStepUpdateMask   = 0b00000010;
