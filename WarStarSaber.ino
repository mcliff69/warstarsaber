/*
     СУПЕР-МЕГО-КРУТОЙ СВЕТОВОЙ МЕЧ НА АРДУИНЕ И АДРЕСНЫХ СВЕТОДИОДАХ!

   Версия 2.0
     
   ЖЕЛЕЗО:
     Используем адресные светодиоды получить любой  цвет, а также плавное включение
     Используем SD карту, чтобы воспроизводить звуки
     Используем IMU (аксель + гироскоп), чтобы генерировать гудящий звук в зависимости от взмахов
     ИЛИ опираясь на взмахи, воспроизводить готовый звук с карты памяти
   ВОЗМОЖНОСТИ:
     Плавное включение/выключение со звуками меча
     Во время работы меч "пульсирует" случайным образом
     Во время работы издаёт звуки:
       РЕЖИМ 1: тон "гудения" зависит от угловой скорости (гироскоп) поворота меча, т.е. взмаха
       РЕЖИМ 2: гудение и звуки взмахов воспроизводятся с карты памяти
         Медленный взмах - длинный звук взмаха (случайно один из 4)
         Быстрый взмах - короткий звук взмаха (случайно один из 5)
     При ударе меч вспыхивает ярко-белым
     При ударе воспроизводится один из 16 звуков удара (случайно)
       Слабый удар - короткие звуки
       Сильный удар - длинные звуки
     При включении показывает уровень заряда аккумулятора длиной светящейся части в процентах
     Следит за напряжением аккумулятора:
       Аккумулятор разрядился ДО ВКЛЮЧЕНИЯ: меч не включится, светодиод кнопки мигнёт несколько раз
       Аккумулятор разрядился ВО ВРЕМЯ РАБОТЫ: меч выключается
   УПРАВЛЕНИЕ:
     Включение/выключение по удерживанию кнопки
     Одно нажатие - смена цвета (красный - оранжевый - жёлтый - зелёный - голубой - синий - фиолетовый)
     Тройное нажатие - смена звукового режима (режим генерации и режим звуков с карты памяти)
     Выбранный цвет и режим хранится в памяти и не сбрасывается при перезагрузке
*/

// ---------------------------- НАСТРОЙКИ -------------------------------
#define NUM_LEDS 16         // число МИКРОСХЕМ на ленте
#define BTN_TIMEOUT 800     // задержка кнопки для удерживания (миллисекунды)
#define BRIGHTNESS 255      // максимальная яркость ленты (0 - 255)

#define SWING_THR 220       // порог угловой скорости для взмаха
#define SWING_S_THR 300     // порог угловой скорости для сильного взмаха
#define STRIKE_THR 150      // порог ускорения для распознавания удара
#define STRIKE_S_THR 320    // порог ускорения для распознавания сильного удара
#define FLASH_DELAY 80      // время вспышки при ударе (миллисекунды)

#define BLINK_ALLOW 1       // разрешить мерцание (1 - разрешить, 0 - запретить)
#define BLINK_AMPL 20       // амплитуда мерцания клинка
#define BLINK_DELAY 30      // задержка между мерцаниями

#define R1 100              // сопротивление резистора делителя (kOm)   
#define R2 51               // сопротивление резистора делителя (kOm)
#define BATTERY_THR 10      // порог заряда АКБ в %, если 0, то не выключаться до конца

//#define DEBUG               // вывод в порт отладочной информации 
// ---------------------------- НАСТРОЙКИ -------------------------------

#define LED_PIN  5          // пин, куда подключен DIN ленты
#define BTN      3          // пин кнопки
#define BTN2     4          // пин кнопки 2
#define VOLT_PIN A6         // пин вольтметра
#define BTN_LED  12         // светодиод кнопки
#define BUSY_PIN 6          // mp3 is busy
#define SERIAL_RX 7         // Software Serial RX
#define SERIAL_TX 8         // Software Serial TX

// -------------------------- БИБЛИОТЕКИ ---------------------------
#include "Wire.h"           // вспомогательная библиотека для работы с акселерометром
#include "I2Cdev.h"         // вспомогательная библиотека для работы с акселерометром
#include "MPU6050.h"        // библиотека для работы с акселерометром
#include <toneAC.h>         // библиотека расширенной генерации звука
#include "FastLED.h"        // библиотека для работы с адресной лентой
#include <EEPROM.h>         // библиотека для работы с энергонезависимой памятью
#include <SoftwareSerial.h>
#include <DFMiniMp3.h>

// implement a notification class,
// its member methods will get called 
//
class Mp3Notify
{
public:
  static void PrintlnSourceAction(DfMp3_PlaySources source, const char* action)
  {
#ifdef DEBUG
    if (source & DfMp3_PlaySources_Sd) 
    {
        Serial.print("SD Card, ");
    }
    if (source & DfMp3_PlaySources_Usb) 
    {
        Serial.print("USB Disk, ");
    }
    if (source & DfMp3_PlaySources_Flash) 
    {
        Serial.print("Flash, ");
    }
    Serial.println(action);
#endif
  }
  static void OnError(uint16_t errorCode)
  {
#ifdef DEBUG
    // see DfMp3_Error for code meaning
    Serial.println();
    Serial.print("Com Error ");
    Serial.println(errorCode);
#endif
  }
  static void OnPlayFinished(DfMp3_PlaySources source, uint16_t track)
  {
#ifdef DEBUG
    Serial.print("Play finished for #");
    Serial.println(track);  
#endif
  }
  static void OnPlaySourceOnline(DfMp3_PlaySources source)
  {
#ifdef DEBUG
    PrintlnSourceAction(source, "online");
#endif
  }
  static void OnPlaySourceInserted(DfMp3_PlaySources source)
  {
#ifdef DEBUG
    PrintlnSourceAction(source, "inserted");
#endif
  }
  static void OnPlaySourceRemoved(DfMp3_PlaySources source)
  {
#ifdef DEBUG
    PrintlnSourceAction(source, "removed");
#endif
  }
};


// создание объектов
CRGB leds[NUM_LEDS];
MPU6050 accelgyro;
SoftwareSerial secondarySerial(SERIAL_RX, SERIAL_TX);
DFMiniMp3<SoftwareSerial, Mp3Notify> mp3(secondarySerial);

// -------------------------- БИБЛИОТЕКИ ---------------------------


// ------------------------------ ПЕРЕМЕННЫЕ ---------------------------------
int16_t ax, ay, az;
int16_t gx, gy, gz;
unsigned long ACC, GYR, COMPL;
int gyroX, gyroY, gyroZ, accelX, accelY, accelZ, freq, freq_f = 20;
float k = 0.2;
boolean on_off_flag = 0, is_on = 0;
unsigned long now, mpuTimer, blink_timer, battery_timer, bzzTimer, fire_timer = 0, remind_off, led_timer = 0;
byte nowColor, nowBGSound, nowONSound, nowOFFSound, nowLightMode, nowFire, nowBtn2Mode, nowVolume;
byte soundMode;
byte redOffset, greenOffset, blueOffset;
int blinkOffset;
byte led_level = 1;
CRGB nowRGB;

// количество mp3 файлов в папках
int on_sound_N, off_sound_N, bg_sound_N, strike_sound_N, short_strike_sound_N, swing_sound_N, short_swing_sound_N, rand_sound_N;

#define ON_FOLDER            1  // sd:/01/
#define OFF_FOLDER           2  // sd:/02/
#define BG_FOLDER            3  // sd:/03/
#define STRIKE_FOLDER        4  // sd:/04/
#define SHORT_STRIKE_FOLDER  5  // sd:/05/
#define SWING_FOLDER         6  // sd:/06/
#define SHORT_SWING_FOLDER   7  // sd:/07/
#define RAND_FOLDER          8  // sd:/08/
#define MISL_FOLDER          9  // sd:/09/


#define MODE_SABER 0
#define MODE_LIGHT 1
#define MODE_FIRE  2

#define S_MODE_OFF 0
#define S_MODE_BG 1
#define S_MODE_STRIKE  2
#define S_MODE_SWING 3
#define S_MODE_SINGLE 4


CRGBPalette16 gPal;

  // красный, оранжевый, жёлтый, зелёный, голубой, синий, фиолетовый, белый
CRGB COLORS[8] = {0xFF0000, 0xB03000, 0x807000, 0x00FF00, 0x007070, 0x0000FF, 0x4000A0, 0xFFFFFF};

boolean setup_on = 0, entering_setup_flag = 0, btn_on = 0, btn_on2 = 0;
byte setup_mode = 0, blinks = 0;
unsigned long btn_time = 0, btn_time2 = 0;
unsigned long sound_start_time = 0, last_busy = 0;
 
// ------------------------------ ПЕРЕМЕННЫЕ ---------------------------------

void setup() {

  FastLED.addLeds<WS2811, LED_PIN, BRG>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(100);  // яроксть ленты 40%
  setAll(CRGB::Black);             // ставим чёрный цвет ленты

  Wire.begin();
  
#ifdef DEBUG
  Serial.begin(9600);
#endif
  
  // ---- НАСТРОЙКА ПИНОВ ----
  pinMode(BTN, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  pinMode(BUSY_PIN, INPUT_PULLUP);
  pinMode(BTN_LED, OUTPUT);
  digitalWrite(BTN_LED, 1);
  // ---- НАСТРОЙКА ПИНОВ ----

  randomSeed(analogRead(2));    // берём точку отсчёта для генератора случайных чисел

  // инициализация и настройка IMU
  accelgyro.initialize();
  accelgyro.setFullScaleAccelRange(MPU6050_ACCEL_FS_16);
  accelgyro.setFullScaleGyroRange(MPU6050_GYRO_FS_250);
  
#ifdef DEBUG
    if (accelgyro.testConnection()) Serial.println(F("MPU6050 OK"));
    else Serial.println(F("MPU6050 fail"));
#endif

  mp3.begin();
  mp3.reset();
  waitMs(5);
  
  on_sound_N            = mp3.getFolderTrackCount(ON_FOLDER);
  waitMs(1);
  off_sound_N           = mp3.getFolderTrackCount(OFF_FOLDER);
  waitMs(1);
  bg_sound_N            = mp3.getFolderTrackCount(BG_FOLDER) + 1; // 0 - режим трещания
  waitMs(1);
  strike_sound_N        = mp3.getFolderTrackCount(STRIKE_FOLDER);
  waitMs(1);
  short_strike_sound_N  = mp3.getFolderTrackCount(SHORT_STRIKE_FOLDER);
  waitMs(1);
  swing_sound_N         = mp3.getFolderTrackCount(SWING_FOLDER);
  waitMs(1);
  short_swing_sound_N   = mp3.getFolderTrackCount(SHORT_SWING_FOLDER);
  waitMs(1);
  rand_sound_N          = mp3.getFolderTrackCount(RAND_FOLDER);
  waitMs(1);

#ifdef DEBUG
    Serial.print("on_sound_N = ");
    Serial.println(on_sound_N);
    Serial.print("off_sound_N = ");
    Serial.println(off_sound_N);
    Serial.print("bg_sound_N = ");
    Serial.println(bg_sound_N);
    Serial.print("strike_sound_N = ");
    Serial.println(strike_sound_N);
    Serial.print("short_strike_sound_N = ");
    Serial.println(short_strike_sound_N);
    Serial.print("swing_sound_N = ");
    Serial.println(swing_sound_N);
    Serial.print("short_swing_sound_N = ");
    Serial.println(short_swing_sound_N);
    Serial.print("rand_sound_N = ");
    Serial.println(rand_sound_N);
#endif

  nowColor     = EEPROM.read(0) % 8;             // вспоминаем цвет меча
  nowONSound   = EEPROM.read(1) % on_sound_N;    // вспоминаем звук включения
  nowOFFSound  = EEPROM.read(2) % off_sound_N;   // вспоминаем звук выключения
  nowBGSound   = EEPROM.read(3) % bg_sound_N;    // вспоминаем звуковой фон
  nowLightMode = EEPROM.read(4) % 3;             // вспоминаем режим меча
  nowFire      = EEPROM.read(5) % 4;             // вспоминаем цвет огня
  nowBtn2Mode  = EEPROM.read(6) % 2;             // вспоминаем режим кнопки 2
  nowVolume    = EEPROM.read(7) % 11;            // вспоминаем громкость

  soundMode = S_MODE_OFF;
  
  mp3.setVolume(nowVolume * 3);
  
#ifdef DEBUG
    Serial.print("mp3.setVolume: ");
    Serial.println(nowVolume);
#endif

  // напомнить выключить меч через 10 мин
  remind_off = millis() + 600000;

  // когда была последняя проверка аккума
  battery_timer = millis();

  // устанавливаем цвет клинка
  setColor(nowColor);
  
  // устанавливаем палитру для огня
  setPal();
  
  // получить процент заряда аккумулятора
  byte capacity = voltage_measure();
  
  // перевести в длину клинка
  capacity = map(capacity, 0, 100, 0, NUM_LEDS + 1);

  // плавно отобразить заряд аккумулятора как длину клинка
  for (byte i = 0; i < capacity; i++) {
    setPixel(i, nowRGB);
    FastLED.show();
    waitMs(35);
  }
  // две секунды для отображения заряда аккума
  waitMs(2000);
  // гасим ленту                         
  setAll(CRGB::Black);
  // возвращаем яркость                    
  FastLED.setBrightness(BRIGHTNESS);      
}

void waitMs(uint16_t ms) {
  uint32_t start = millis();
  
  while ((millis() - start) < ms) {
    // calling mp3.loop() periodically allows for notifications 
    // to be handled without interrupts
    mp3.loop(); 
    delay(1);
  }
}

// моргать светодиодом в режиме установок
void setupBlink() {
  if (led_level == 0) {
    if ((blinks == 0 && (now > led_timer + 800)) 
        || (blinks > 0 && (now > led_timer + 300))) {
      led_timer = now;
      led_level = 1;
      digitalWrite(BTN_LED, led_level);
    }
  } else {
    if (now > led_timer + 300) {
      blinks++;
      blinks %= setup_mode + 1;
      led_timer = now;
      led_level = 0;
      digitalWrite(BTN_LED, led_level);
    }
  }
}

void loop() {
  
  // calling mp3.loop() periodically allows for notifications 
  // to be handled without interrupts
  mp3.loop();

  // опрос и отработка кнопки
  btnTick();
            
  now = millis();

  // если меч включен
  if (is_on) {
    // если в режиме установок параметров
    if (setup_on) {
      // моргаем светодиодом
      setupBlink();
    }
    // если в режиме меча
    if (nowLightMode == MODE_SABER) {
      // мерцание
      randomBlink();
      // если не в режиме установок параметров      
      if (!setup_on) {
        // получить ускорение, посчитать частоту для трещалки
        getFreq();
        // отработка удара          
        strikeTick();
        // отработка взмаха       
        swingTick();        
      }
    // если в режиме огня
    } else if (nowLightMode == MODE_FIRE) {
      // обновляем огонь
      fire2012();
    }
    // фоновый звук
    bg_sound();
    // проверка аккума         
    batteryTick();        

  // если меч выключен, но в дежурном режиме
  } else {
    // напомнить, что надо выключить меч
    if (now > remind_off) {
      sound_start_time = now;
      soundMode = S_MODE_SINGLE;
      mp3.playFolderTrack(MISL_FOLDER, 3);   // sd:/09/003.mp3
      
      // напомнить через минуту опять
      remind_off = now + 60000;
    }
  }
}


#define BOUNCE 60  // дребезг контактов в мс

#define SETUP_BG_SOUND   0
#define SETUP_BTN2_MODE  1
#define SETUP_LIGHT_MODE 2
#define SETUP_SOUND_ON   3
#define SETUP_SOUND_OFF  4
#define SETUP_VOLUME     5
#define SETUP_COLOR      6

void btnTick() {
  boolean pushed = !digitalRead(BTN);    // кнопка нажата / не нажата
  boolean pushed2 = !digitalRead(BTN2);    // кнопка нажата / не нажата
  now = millis();

// пропустить дребезг контактов
  if (now < btn_time + BOUNCE) pushed = btn_on;
  if (now < btn_time2 + BOUNCE) pushed2 = btn_on2;

//#ifdef DEBUG
//    if (now < btn_time + BOUNCE) Serial.println(F("Дребезг контактов 1"));
//    if (now < btn_time2 + BOUNCE) Serial.println(F("Дребезг контактов 2"));
//#endif

  if (pushed ^ btn_on) {// переход не нажата -> нажата или нажата -> не нажата
    btn_on = pushed;
    btn_time = now;
    waitMs(10);
    
#ifdef DEBUG
    if (pushed) 
      Serial.println(F("Кнопка 1 нажата"));
    else 
      Serial.println(F("Кнопка 1 отжата"));
#endif
  }

  if (pushed2 ^ btn_on2) {// переход не нажата -> нажата или нажата -> не нажата
    btn_on2 = pushed2;
    btn_time2 = now;
    waitMs(10);
    
#ifdef DEBUG
    if (pushed2) 
      Serial.println(F("Кнопка 2 нажата"));
    else 
      Serial.println(F("Кнопка 2 отжата"));
#endif
  }
    

  // Если флаг запроса включения/выключения
  if (on_off_flag) {
    // Сбросить флаг, если отжата кнопка 1
    if (!pushed) {
      on_off_flag = 0;
      waitMs(1);
    }
    // не реагировать на кнопки, пока не будет отжата кнопка 1
    return;
  }
  

  // Если мы входим в режим настроек
  if (entering_setup_flag) {
    // Сбросить флаг, если отжата кнопка 2
    if (!pushed2) {
      entering_setup_flag = 0;
      waitMs(1);
    }
    // не реагировать на кнопки, пока не будет отжата кнопка 2
    return;
  }
  
  // если кнопка 1 нажата 800 мс
  if (btn_on && (now > btn_time + BTN_TIMEOUT) && !setup_on) {

#ifdef DEBUG
  Serial.println(F("Включение / выключение меча"));
#endif

    // флаг запроса включения/выключения
    on_off_flag = 1;
    // вкл/выкл меча
    on_off();           
    return;
  }

  // Не реагировать на кнопки, пока меч не будет включен
  if (!is_on) {
    return;
  }  

  // переход в режим настроек меча и выход из него
  if (btn_on2 && (now > btn_time2 + BTN_TIMEOUT)) {
    
    // переключить режим
    setup_on ^= 1;

    // флаг, что мы входим в режим настроек
    entering_setup_flag = 1;
    

    // вход в настройки 
    if (setup_on) {
      setup_mode = 0;
      
#ifdef DEBUG
    Serial.println(F("Вход в настройки"));
#endif

    // выход из настроек 
    } else {
      
#ifdef DEBUG
    Serial.println(F("Выход из настроек"));
#endif
      // сохраняем настройки в энергонезависимую память
      EEPROM.write(0, nowColor);     // записать цвет меча
      EEPROM.write(1, nowONSound);   // записать звук включения
      EEPROM.write(2, nowOFFSound);  // записать звук выключения
      EEPROM.write(3, nowBGSound);   // записать звуковой фон
      EEPROM.write(4, nowLightMode); // записать режим меча
      EEPROM.write(5, nowFire);      // записать цвет огня
      EEPROM.write(6, nowBtn2Mode);  // записать режим кнопки 2
      EEPROM.write(7, nowVolume);    // записать громкость

      // включаем светодиод
      digitalWrite(BTN_LED, 1);
    }

  // режим установок меча
  } else if (setup_on) {

    // отпустили кнопку 2, какой параметр будем менять
    if (!btn_on2 && btn_time2 == now) {
      setup_mode++;
      setup_mode %= 6;
      
#ifdef DEBUG
    Serial.print(F("Режим установки параметра "));
    Serial.println(setup_mode);
#endif

    // отпустили кнопку 1, меняем параметр
    } else if (!btn_on && btn_time == now) {
      
//      // цвет
//      if (setup_mode == SETUP_COLOR) {
//        inc_color();
//
//      } else 
      
      // фоновый звук
      if (setup_mode == SETUP_BG_SOUND) {
        inc_bg_sound();

      // режим кнопки 2 
      // nowBtn2Mode = 0 - смена фонового звука
      // nowBtn2Mode = 1 - произвольная фраза
      } else if (setup_mode == SETUP_BTN2_MODE) {
        nowBtn2Mode++;
        nowBtn2Mode %= 2;

#ifdef DEBUG
    Serial.print(F("Устанавливаем nowBtn2Mode в "));
    Serial.println(nowBtn2Mode);
#endif

      // режим меча (меч, фонарь, огонь)
      } else if (setup_mode == SETUP_LIGHT_MODE) {
        nowLightMode++;                           
        nowLightMode %= 3;
        mp3.stop();
        noToneAC();

#ifdef DEBUG
    Serial.print(F("Устанавливаем nowLightMode в "));
    Serial.println(nowLightMode);
#endif
        
      // звук включения
      } else if (setup_mode == SETUP_SOUND_ON) {
        nowONSound++;                           
        nowONSound %= on_sound_N;
        sound_start_time = now;
        soundMode = S_MODE_SINGLE;
        mp3.playFolderTrack(ON_FOLDER, nowONSound + 1);   // sd:/01/00X.mp3

#ifdef DEBUG
    Serial.print(F("Устанавливаем nowONSound в "));
    Serial.println(nowONSound);
#endif

      // звук выключения
      } else if (setup_mode == SETUP_SOUND_OFF) {
        nowOFFSound++;                           
        nowOFFSound %= off_sound_N;
        sound_start_time = now;
        soundMode = S_MODE_SINGLE;
        mp3.playFolderTrack(OFF_FOLDER, nowOFFSound + 1);   // sd:/02/00X.mp3

#ifdef DEBUG
    Serial.print(F("Устанавливаем nowOFFSound в "));
    Serial.println(nowOFFSound);
#endif

      // громкость
      } else if (setup_mode == SETUP_VOLUME) {
        nowVolume++;                           
        nowVolume %= 11;
        mp3.setVolume(nowVolume * 3);
        
#ifdef DEBUG
    Serial.print(F("Устанавливаем nowVolume в "));
    Serial.println(nowVolume);
#endif
      } 
    }
  
  // рабочий режим
  } else {
    
    // отпустили кнопку 1, первая кнопка меняет цвет
    if (!btn_on && btn_time == now) {
      inc_color();

    // отпустили кнопку 2, и режим меча
    } else if (!btn_on2 && btn_time2 == now && nowLightMode == MODE_SABER) {
      
      // меняет фоновый звук
      if (nowBtn2Mode == 0) {
        inc_bg_sound();

      // или воспроизводит произвольный трек из папки 08
      } else {
        sound_start_time = now;
        soundMode = S_MODE_SINGLE;
        mp3.playFolderTrack(RAND_FOLDER, random(rand_sound_N) + 1);   // sd:/08/00X.mp3

#ifdef DEBUG
    Serial.println(F("произвольный трек из папки 08"));
#endif
      }
    }
  }
}

// следующий цвет
void inc_color() {
  if (nowLightMode == MODE_SABER || nowLightMode == MODE_LIGHT) {
    // сменить цвет
    nowColor++;
    nowColor %= 8;
    // установить цвет
    setColor(nowColor);
    // плавно меняем цвет               
    light_up();                         
    
#ifdef DEBUG
    Serial.print(F("Устанавливаем nowColor в "));
    Serial.println(nowColor);
#endif

  // nowLightMode == MODE_FIRE
  } else {                              
    nowFire++;
    nowFire %= 4;
    setPal();

#ifdef DEBUG
    Serial.print(F("Устанавливаем nowFire в "));
    Serial.println(nowFire);
#endif
  }
}

void setPal() {
    if (nowFire == 0)
      gPal = CRGBPalette16 (CRGB::Black, CRGB::DarkRed, CRGB::OrangeRed, CRGB::Yellow);
    else if (nowFire == 1)
      gPal = CRGBPalette16 (CRGB::Black, CRGB::DarkBlue, CRGB::Blue, CRGB::Purple);
    else if (nowFire == 2)
      gPal = CRGBPalette16 (CRGB::Black, CRGB::DarkGreen, CRGB::Green, CRGB::Cyan);
    else
      gPal = CRGBPalette16 (CRGB::Black, CRGB::DarkRed, CRGB::DarkMagenta, CRGB::Blue);
}

// следующий фоновый звук
void inc_bg_sound() {
  nowBGSound++;
  nowBGSound %= bg_sound_N;

  mp3.stop();
  noToneAC();

#ifdef DEBUG
    Serial.print(F("Устанавливаем nowBGSound в "));
    Serial.println(nowBGSound);
#endif
}

// вкл/выкл меча со звуками
void on_off() {
    now = millis();
    
    // включаем, если меч был выключен                      
    if (!is_on) {                 
      if (voltage_measure() >= BATTERY_THR) {

#ifdef DEBUG
        Serial.println(F("SABER ON"));
#endif
        // вырубить трещалку
        noToneAC();                    

        if (nowLightMode == MODE_SABER) {
          sound_start_time = now;
          soundMode = S_MODE_SINGLE;
          // воспроизвести звук включения
          mp3.playFolderTrack(ON_FOLDER, nowONSound + 1);  // sd:/01/00X.mp3 
          // ждём воспроизведение
          waitMs(200);
        }
        // лента включается                    
        light_up();
        bzzTimer = now;
        // запомнить, что меч включен
        is_on = true;                
          
      // аккум разряжен
      } else {

#ifdef DEBUG
        Serial.println(F("LOW VOLTAGE!"));
#endif
        for (int i = 0; i < 10; i++) {
          digitalWrite(BTN_LED, i%2);
          waitMs(400);
        }
      }

    // выключаем, если меч включен
    } else { 
      // вырубить трещалку                 
      noToneAC();
                          
      sound_start_time = now;
      soundMode = S_MODE_SINGLE;
      if (voltage_measure() < BATTERY_THR) {
        // звук умирающего телефона
        mp3.playFolderTrack(MISL_FOLDER, 1);   // sd:/09/001.mp3
      } else {
        // звук выключения меча
        mp3.playFolderTrack(OFF_FOLDER, nowOFFSound + 1);   // sd:/02/00X.mp3
      }
      // ждём воспроизведение
      waitMs(300);
      // лента выключается                    
      light_down();                   

#ifdef DEBUG
      Serial.println(F("SABER OFF"));
#endif
      is_on = false;                 // запомнить, что меч выключен

      // напомнить через 10 мин, что надо выключить меч
      remind_off = now + 600000;
    }
}

// фоновый звук
void bg_sound() {
  // проверить, идет ли воспроизведение mp3 
  boolean busy = !digitalRead(BUSY_PIN);    
  now = millis();
  if (busy) {
    last_busy = now;
  }
  
  if (nowLightMode == MODE_SABER) {

    // режим трещания
    if (nowBGSound == 0) {
      if (now > bzzTimer + 3) {          // обновлять частоту трещания каждые 3 мс
        toneAC(freq_f, nowVolume);       // трещать
        bzzTimer = now;                  // сбросить таймер
      }
      
    // фоновый звук с карты
    } else {
      
      if (
            // если идет воспроизведение
            busy ||
            // пропускаем 200 мс с момента запуска трека, чтобы сигнал BUSY успел установиться
            (now < sound_start_time + 200) ||
            // пропускаем 200 мс с момента, когда трек еще проигрывался, 
            // чтобы сигнал BUSY успел установиться при циклическом воспроизведении
            (now < last_busy + 200 && soundMode == S_MODE_BG)) {
              return;
      }
      
      sound_start_time = now;
      soundMode = S_MODE_BG;
      mp3.playFolderTrack(BG_FOLDER, nowBGSound);   // sd:/03/00X.mp3
      mp3.setRepeatPlayCurrentTrack(true);

#ifdef DEBUG
      Serial.print("Фоновый звук ");
      Serial.println(nowBGSound);
#endif
    }

  // режим огня
  } else if (nowLightMode == MODE_FIRE) {
    if (
          // если идет воспроизведение
          busy ||
          // пропускаем 200 мс с момента запуска трека, чтобы сигнал BUSY успел установиться
          (now < sound_start_time + 200) ||
          // пропускаем 200 мс с момента, когда трек еще проигрывался, 
          // чтобы сигнал BUSY успел установиться при циклическом воспроизведении
          (now < last_busy + 200 && soundMode == S_MODE_BG)) {
            return;
    }
    
    sound_start_time = now;
    soundMode = S_MODE_BG;
    mp3.playFolderTrack(MISL_FOLDER, 2);   // sd:/09/002.mp3
    mp3.setRepeatPlayCurrentTrack(true);

#ifdef DEBUG
      Serial.println("Фоновый звук огня");
#endif

  }
}

void randomBlink() {
  if (BLINK_ALLOW && (millis() - blink_timer > BLINK_DELAY)) {
    blink_timer = millis();
    blinkOffset = blinkOffset * k + random(-BLINK_AMPL, BLINK_AMPL) * (1 - k);
    if (nowColor == 0) blinkOffset = constrain(blinkOffset, -15, 5);
    redOffset = constrain(nowRGB.red + blinkOffset, 0, 255);
    greenOffset = constrain(nowRGB.green + blinkOffset, 0, 255);
    blueOffset = constrain(nowRGB.blue + blinkOffset, 0, 255);
    setAll(CRGB(redOffset, greenOffset, blueOffset));
  }
}

void strikeTick() {
  if (ACC > STRIKE_THR) {      // если ускорение превысило порог
    now = millis();
    if (now < sound_start_time + 400 && soundMode == S_MODE_STRIKE) {
      return;
    }
    soundMode = S_MODE_STRIKE;
    sound_start_time = now;
    if (ACC > STRIKE_S_THR) {
      mp3.playFolderTrack(STRIKE_FOLDER, random(strike_sound_N) + 1);   // sd:/04/00X.mp3
    } else {
      mp3.playFolderTrack(SHORT_STRIKE_FOLDER, random(short_strike_sound_N) + 1);   // sd:/05/00X.mp3
    }
    strike_flash();

#ifdef DEBUG
      Serial.print(F("STRIKE "));
      Serial.println(ACC);
#endif
  }
}

void swingTick() {
  now = millis();
  if (now < sound_start_time + 800 && soundMode == S_MODE_SWING) {
    return;
  }
  if (now < sound_start_time + 400 && soundMode == S_MODE_STRIKE) {
    return;
  }
  if ((GYR > SWING_THR) && nowBGSound > 0 ) {
    sound_start_time = now;
    soundMode = S_MODE_SWING;
    if (GYR > SWING_S_THR) {
      mp3.playFolderTrack(SHORT_SWING_FOLDER, random(short_swing_sound_N) + 1);   // sd:/07/00X.mp3
    } else {
      mp3.playFolderTrack(SWING_FOLDER, random(swing_sound_N) + 1);   // sd:/06/00X.mp3
    }

#ifdef DEBUG
      Serial.print(F("SWING "));
      Serial.println(GYR);
#endif
  }
}

void getFreq() {
  if (micros() > mpuTimer + 500) {                            // каждые полмиллисекунды
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);       // получить показания с IMU

    // найти абсолютное значение, разделить на 100
    gyroX = abs(gx / 100);
    gyroY = abs(gy / 100);
    gyroZ = abs(gz / 100);
    accelX = abs(ax / 100);
    accelY = abs(ay / 100);
    accelZ = abs(az / 100);

    // найти среднеквадратичное (сумма трёх векторов в общем)
    ACC = sq((long)accelX) + sq((long)accelY) + sq((long)accelZ);
    ACC = sqrt(ACC);
    GYR = sq((long)gyroX) + sq((long)gyroY) + sq((long)gyroZ);
    GYR = sqrt((long)GYR);
    COMPL = ACC + GYR;
    
//       // отладка работы IMU
//       Serial.print("$");
//       Serial.print(gyroX);
//       Serial.print(" ");
//       Serial.print(gyroY);
//       Serial.print(" ");
//       Serial.print(gyroZ);
//       Serial.println(";");
    
    freq = (long)COMPL * COMPL / 1500;                        // изменяем частоту по параболе
    freq = constrain(freq, 18, 300);                          // ставим пределы
    freq_f = freq * k + freq_f * (1 - k);                     // сглаживающий фильтр
    mpuTimer = micros();                                      // сбросить таймер
  }
}


// функция выбора цвета одного светодиода
void setPixel(int pixel, CRGB rgb) {
  leds[pixel] = rgb;
}

// функция заливки всех светодиодов выбранным цветом
void setAll(CRGB rgb) {
  for (int i = 0; i < NUM_LEDS; i++ ) {
    leds[i] = rgb;
  }
  FastLED.show();
}

// плавное включение меча
void light_up() {
  for (char i = 0; i < NUM_LEDS; i++) {          // включить все диоды выбранным цветом
    setPixel(i, nowRGB);
    FastLED.show();
    waitMs(25);
  }
}

// плавное выключение меча
void light_down() {
  for (char i = NUM_LEDS - 1; i >= 0; i--) {      // выключить все диоды
    setPixel(i, CRGB::Black);
    FastLED.show();
    waitMs(25);
  }
}

// моргание при ударе
void strike_flash() {
  setAll(CRGB::White);             // цвет клинка белым
  waitMs(FLASH_DELAY);                // ждать
  setAll(nowRGB);          // цвет клинка старым цветом
}

// выбор цвета
// 0-красный, 1-оранжевый, 2-жёлтый, 3-зелёный, 4-голубой, 5-синий, 6-фиолетовый, 7-белый
void setColor(byte color) {
  nowRGB = COLORS[color];
}

// проверка аккума
void batteryTick() {
  now = millis();
  // проверять каждые 30 сек
  if (now > battery_timer + 30000) {
    // заряд аккума ниже порога
    if (voltage_measure() < BATTERY_THR) {
      on_off();                              // выкл меча
    }
    battery_timer = now;
  }
}

// алгоритм, получающий из напряжения остаток заряда в процентах
// аппроксимировано вручную по графику разряда литий-иона
byte voltage_measure() {
  long voltage = 0;
  for (int i = 0; i < 10; i++) {    // среднее арифметическое из 10 измерений
    voltage += analogRead(VOLT_PIN);
  }
  // делитель на R1-R2, 10 измерений, 3 банки аккума, 100 для повышения точности, если на входе 5 вольт, analogRead() выдает 1023
  // если в среднем на каждой банке 4.2 Вольт, то voltage = 420, voltage_measure() = 100 (%)
  voltage = voltage * (R1 + R2) / R2 / 10 / 3 * 100 * 5 / 1023;
  if (voltage > 387)
    voltage =  map(voltage, 420, 387, 100, 77);
  else if (voltage > 375)
    voltage =  map(voltage, 387, 375, 77, 54);
  else if (voltage > 368)
    voltage =  map(voltage, 375, 368, 54, 31);
  else if (voltage > 340)
    voltage =  map(voltage, 368, 340, 31, 8);
  else
    voltage =  map(voltage, 340, 260, 8, 0);
  return constrain(voltage, 0, 100);
}

// режим огня
#define COOLING  55
#define SPARKING 120

void fire2012() {
  
  if (millis() - fire_timer > 16) {
    fire_timer = millis();

    // Array of temperature readings at each simulation cell
    static byte heat[NUM_LEDS];

    // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS; j++) {
      byte colorindex = scale8( heat[j], 240);
      leds[j] = ColorFromPalette( gPal, colorindex);
    }
    FastLED.show();
  }
}
