#define SWITCH_WET_PIN 2          // Кнопка переключения режимов омывателя
#define WET_PIN 5                 // Контакт включения омывателя лобового стекла
#define OUR_WET_PIN 6             // Контакт включения омывателя фар

#define SWITCH_LIGHT_PIN 3        // Кнопка переключения режимов люстры на крыше
#define LIGHT_PIN 7               // Контакт включения дальнего света
#define OUR_LIGHT_PIN 8           // Контакт включения люстры

#define SWITCH_HEAT_PIN 4         // Кнопка переключения режимов обогрева руля
#define HEAT_PIN A0               // Контакт датчика температуры руля
#define OUR_HEAT_PIN 9            // Контакт включения обогрева руля

#define SWITCH_CAMERA_PIN A2      // Кнопка включения передней камеры
#define REAR_PIN A1               // Контакт включения заднего хода
#define OUR_CAMERA_PIN A3         // Контакт включения передней камеры

#define DATA_PIN 10               // Пин подключен к DS входу 74HC595
#define LATCH_PIN 11              // Пин подключен к ST_CP входу 74HC595
#define CLOCK_PIN 12              // Пин подключен к SH_CP входу 74HC595

#define HERTZ 16                  // Тайминг основного тела программы
#define FALSE_PUSH_TIME 50        // Время защиты от дребезга контактов

#define LONG_WET_PUSH_TIME 500    // Минимальное время нажатия для срабатывания длинного нажатия
#define ENEMY_WET_TIME 500        // Минимальное время работы омывателя лобового стекла для засчитывания попытки
#define WET_COUNT 5               // Число срабатываний омывателя лобового стекла для срабатывания омывателя фар
#define OUR_WET_TIME 3000         // Время работы омывателя фар в автоматическом режиме

#define LONG_LIGHT_PUSH_TIME 500  // Минимальное время нажатия для срабатывания длинного нажатия
#define LONG_HEAT_PUSH_TIME 500   // Минимальное время нажатия для срабатывания длинного нажатия

#define HEAT_MIN 20               // Минимальная температура для срабатывания обогрева руля
#define HEAT_MAX 35               // Максимальная температура руля, которой нужно достичь 
#define HEAT_MAXSAVE 200          // Из скольки показаний температуры выбирать среднюю
#define HEAT_PERIOD_ON 10000      // Время работы подогрева
#define HEAT_PERIOD_OFF 15000     // Время паузы подогрева
#define HEAT_CORRECT 16           // Поправка показаний температуры руля при его включении
#define HEAT_SCATTER 5            // Не принимать данные температуры при резком скачке выше или ниже на это значение
#define HEAT_SCATTER_MIN -100     // Не принимать данные температуры ниже этого значения
#define HEAT_SCATTER_MAX 100      // Не принимать данные температуры выше этого значения

#include <EEPROM.h>

#define STATE_WET_MEM 0           // Ячейка сохранения режима омывателя
#define STATE_LIGHT_MEM 1         // Ячейка сохранения режима люстры
#define STATE_HEAT_MEM 2          // Ячейка сохранения режима обогрева руля

// Главный тайминг
unsigned long currentTime;
unsigned long loopTime;

// Обработка нажатий клавиш переключения режимов омывателя
boolean isWetButtonPress = false;
boolean isWetButtonLongPress = false;
boolean processWetPress = false;
unsigned long buttonWetTime;

// Обработка процесса поливки из омывателя стеклоочистителя
boolean isWetAuto = false;
boolean isWetComplete = false;
boolean processWet = false;
unsigned long wetTime;
byte wetCount = 0;

// Обработка процесса поливки из омывателя фар
boolean isOurWet = false;
unsigned long ourWetTime;

// Обработка включения дальнего света
boolean isLightAuto = false;
boolean isLightOn = false;
boolean processLight = false;
unsigned long lightTime;

// Обработка нажатий клавиш переключения режимов люстры на крыше
boolean isLightButtonPress = false;
boolean isLightButtonLongPress = false;
boolean processLightPress = false;
unsigned long buttonLightTime;

// Обработка нажатий клавиш переключения режимов обогрева руля
boolean isHeatAuto = 0;
boolean isHeatButtonPress = false;
boolean isHeatButtonLongPress = false;
boolean processHeatPress = false;
unsigned long buttonHeatTime;

// Обработка нажатий клавиш включения передней камеры
boolean isCameraButtonPress = false;
boolean processCameraPress = false;
unsigned long buttonCameraTime;
boolean isCameraOn = false;

// Обработка включения заднего хода
boolean isRearOn = false;
boolean processRear = false;
unsigned long rearTime;

// Обработка температуры руля
boolean isHeatOn = false;
boolean isHeatOnNow = false;
boolean processHeat = false;
unsigned long heatTime;
double trueTemp = 0;
double oldTemps[HEAT_MAXSAVE];
boolean trueTempDone = false;
unsigned long heatingTime;
unsigned long heatingLoopTime;
int tempIterator = 0;

// Число - 8 бит для управления 
byte indicator = 0;

void setup() {
  // Слушаем кнопки
  pinMode(SWITCH_WET_PIN, INPUT);
  pinMode(SWITCH_LIGHT_PIN, INPUT);
  pinMode(SWITCH_HEAT_PIN, INPUT);
  pinMode(SWITCH_CAMERA_PIN, INPUT);

  // Входы\выходы омывателя фар
  pinMode(WET_PIN, INPUT);
  pinMode(OUR_WET_PIN, OUTPUT);
  digitalWrite(OUR_WET_PIN, LOW);

  // Входы\выходы люстры
  pinMode(LIGHT_PIN, INPUT);
  pinMode(OUR_LIGHT_PIN, OUTPUT);
  digitalWrite(OUR_LIGHT_PIN, LOW);

  // Входы\выходы обогрева руля
  pinMode(HEAT_PIN, INPUT);
  pinMode(OUR_HEAT_PIN, OUTPUT);
  digitalWrite(OUR_HEAT_PIN, LOW);
  
  // Входы\выходы камеры
  pinMode(REAR_PIN, INPUT);
  pinMode(OUR_CAMERA_PIN, OUTPUT);
  digitalWrite(OUR_CAMERA_PIN, LOW);

  // Пины для управления сдвиговым регистром (на нём светодиоды)
  pinMode(LATCH_PIN, OUTPUT);
  digitalWrite(LATCH_PIN, LOW);
  pinMode(CLOCK_PIN, OUTPUT);
  digitalWrite(CLOCK_PIN, LOW);
  pinMode(DATA_PIN, OUTPUT);
  digitalWrite(DATA_PIN, LOW);
  
  // Считываем сохранённый режим и устанавливаем его
  isWetAuto = EEPROM.read(STATE_WET_MEM);
  isLightAuto = EEPROM.read(STATE_LIGHT_MEM);
  isHeatAuto = EEPROM.read(STATE_HEAT_MEM);

  // Инициализациях светодиодных индикаторов - зажигаем все, через 2 секунды гасим
  indicator = 255;
  setLeds();
  delay(2000);
  indicator = 0;
  setLeds();

  currentTime = millis();
  loopTime = currentTime;

  Serial.begin(9600);
}

// Главный цикл с таймингом 
void loop() {
  currentTime = millis();

  // Обрабатываем каждые 16 мс (60 герц)
  if(currentTime >= (loopTime + HERTZ)) {
    // Обрабатываем кнопку переключения режимов омывателя
    processWetButton();

    // Обрабатываем омыватель лобового стекла
    processEnemyWet();

    // Обрабатываем кнопку переключения режимов люстры
    processLightButton();
    
    // Обрабатываем кнопку включения передней камеры
    processCameraButton();

    // Обрабатываем дальний свет
    processEnemyLight();

    // Обрабатываем кнопку переключения режимов обогрева руля
    processHeatButton();
    
    // Обрабатываем температуру руля
    processEnemyHeat();
    
    // Обрабатываем сигнал заднего хода
    processEnemyCamera();
    
    // Включаем или выключаем обогрев, чтобы не спалить улитку
    processHeatSafe();
    
    loopTime = currentTime;
  }
}

// Обработка нажатий клавиши режимов омывателя
void processWetButton() {
  // Если кнопка нажата
  if(digitalRead(SWITCH_WET_PIN) == LOW) {
    // Если не была нажата до этого
    if(!processWetPress && !isWetButtonPress) {
      // Запускаем таймер защиты от дребезга
      buttonWetTime = currentTime;
      processWetPress = true;
    }

    // Если была нажата до этого в течении 50 мс
    if(processWetPress && !isWetButtonPress && currentTime >= (buttonWetTime + FALSE_PUSH_TIME)) {
      // Считаем, что кнопка действительно нажата
      isWetButtonPress = true;
    }

    // Если кнопка действительно нажата и уже удерживается в течении 1000 мс
    if(processWetPress && isWetButtonPress && !isWetButtonLongPress && currentTime >= (buttonWetTime + LONG_WET_PUSH_TIME)) {
      // Запускаем процесс ручного омывания фар
      isWetButtonLongPress = true;
      startWet();
    }
  } 
  // Если кнопка НЕ нажата
  else {
    // Если до этого было нажатие короче 50 мс
    if(processWetPress && !isWetButtonPress) {
      // Считаем, что это дребезг контактов и ничего не делаем
      isWetButtonPress = false;
      processWetPress = false;
    }

    // Если до этого было нажатие длинее 50 см
    if(isWetButtonPress && processWetPress) {
      // Если это было нажатие длинее 1000 мс
      if(isWetButtonLongPress) {
        // Останавливаем процесс ручного омывания фар
        stopWet();
      } 
      else {
        // Иначе переключаем режим
        toggleWetState();
      }

      // Останавливаем таймеры и обнуляем переменные
      isWetButtonPress = false;
      processWetPress = false;
      isWetButtonLongPress = false;
      buttonWetTime = 0;
    }
  }
}

// Обработка срабатываний омывателя лобового стекла
void processEnemyWet() {
  // Если режим авто и омыватель лобового стекла включён
  if(isWetAuto && digitalRead(WET_PIN) == HIGH) {
    // Если омыватель лобового стекла не был включен до этого
    if(!processWet && !isWetComplete) {
      // Запускаем таймер защиты от быстрых срабатываний
      wetTime = currentTime;
      processWet = true;
    }

    // Если омыватель лобового стекла включен дольше 500 мс
    if(processWet && !isWetComplete && currentTime >= (wetTime + ENEMY_WET_TIME)) {
      // Считаем, что омывание лобового стекла имело место быть
      isWetComplete = true;
    }
  } 
  // Если режим авто и омыватель лобового стекла выключен
  else if (isWetAuto) {
    // Если омыватель лобового стекла был включен меньше 500 мс
    if(processWet && !isWetComplete) {
      // Не засчитываем попытку
      processWet = false;
      isWetComplete = false;
    }

    // Если омыватель лобового стекла был включен больше 500 мс
    if(isWetComplete && processWet) {
      // Увеличиваем число срабатываний на 1
      wetCount++;

      // Останавливаем таймеры и обнуляем переменные
      isWetComplete = false;
      processWet = false;
      wetTime = 0;
    }
  }

  // Если режим авто, в данный момент не идёт омывание фар и число срабатываний омывателя лобового стекла больше или равно 5
  if(isWetAuto && wetCount >= WET_COUNT && !isOurWet) {
    // Включаем омыватель фар
    wetCount = 0;
    isOurWet = true;
    ourWetTime = currentTime;
    startWet();
  }

  // Если включен омыватель фар и он работает дольше 3000 мс
  if(isOurWet && currentTime >= (ourWetTime + OUR_WET_TIME)) {
    // Останавливаем его
    stopWet();
    isOurWet = false;
    ourWetTime = 0;
  }
}

// Переключает автоматические режимы омывателя фар
void toggleWetState() {
  isWetAuto = !isWetAuto;
  
  EEPROM.write(STATE_WET_MEM, isWetAuto);
  
  setWetStatusLed(isWetAuto);
}

// Запускает омыватель фар и индицирует это светодиодом
void startWet() {
  setWetActiveLed(true);
  digitalWrite(OUR_WET_PIN, HIGH);
}

// Останавливае омыватель фар и индицирует это светодиодом
void stopWet() {
  setWetActiveLed(false);
  digitalWrite(OUR_WET_PIN, LOW);
}

// Обработка нажатий клавиши режимов омывателя
void processLightButton() {
  // Если кнопка нажата
  if(digitalRead(SWITCH_LIGHT_PIN) == HIGH) {
    // Если не была нажата до этого
    if(!processLightPress && !isLightButtonPress) {
      // Запускаем таймер защиты от дребезга
      buttonLightTime = currentTime;
      processLightPress = true;
    }

    // Если была нажата до этого в течении 50 мс
    if(processLightPress && !isLightButtonPress && currentTime >= (buttonLightTime + FALSE_PUSH_TIME)) {
      // Считаем, что кнопка действительно нажата
      isLightButtonPress = true;
    }

    // Если кнопка действительно нажата и уже удерживается в течении 1000 мс
    if(processLightPress && isLightButtonPress && !isLightButtonLongPress && currentTime >= (buttonLightTime + LONG_LIGHT_PUSH_TIME)) {
      isLightButtonLongPress = true;
      toggleLightState();
    }
  } 
  // Если кнопка НЕ нажата
  else {
    // Если до этого было нажатие короче 50 мс
    if(processLightPress && !isLightButtonPress) {
      // Считаем, что это дребезг контактов и ничего не делаем
      isLightButtonPress = false;
      processLightPress = false;
    }

    // Если до этого было нажатие длинее 50 см
    if(isLightButtonPress && processLightPress) {
      // Если это было нажатие длинее 1000 мс
      if(isLightButtonLongPress) {

      } 
      else {
        // Если включен авто режим, то ручное управление его выключает
        if(isLightAuto) {
          isLightAuto = false;
          setLightStatusLed(isLightAuto);
        }
        if(digitalRead(OUR_LIGHT_PIN) == HIGH) {
          stopLight();
        } 
        else {
          startLight();
        }
      }

      // Останавливаем таймеры и обнуляем переменные
      isLightButtonPress = false;
      processLightPress = false;
      isLightButtonLongPress = false;
      buttonLightTime = 0;
    }
  }
}

// Обработка включения дальнего света
void processEnemyLight() {
  // Если режим авто и дальний свет включен
  if(isLightAuto && digitalRead(LIGHT_PIN) == HIGH) {
    // Если люстра выключена, запускаем таймер защиты от дребезга
    if(!isLightOn && !processLight) {
      lightTime = currentTime;
      processLight = true;
    }
    
    // Если дальний всё ещё включен, то включаем люстру
    if(!isLightOn && processLight && currentTime >= (lightTime + FALSE_PUSH_TIME)) {
      startLight();
      processLight = false;
    }
  } 
  // Если режим авто и дальний свет выключен
  else if (isLightAuto) {
    // Если люстра включена, запускаем таймер защиты от дребезга
    if(isLightOn && !processLight) {
      lightTime = currentTime;
      processLight = true;
    }
    
    // Если дальний всё ещё выключен, то выключаем люстру
    if(isLightOn && processLight && currentTime >= (lightTime + FALSE_PUSH_TIME)) {
      stopLight();
      processLight = false;
    }
  }
}

// Переключает автоматические режимы люстры
void toggleLightState() {
  isLightAuto = !isLightAuto;
  
  EEPROM.write(STATE_LIGHT_MEM, isLightAuto);
  
  setLightStatusLed(isLightAuto);
}

// Включает люстру и индицирует это светодиодом
void startLight() {
  setLightActiveLed(true);
  digitalWrite(OUR_LIGHT_PIN, HIGH);
  isLightOn = true;
}

// Выключает люстру и индицирует это светодиодом
void stopLight() {
  setLightActiveLed(false);
  digitalWrite(OUR_LIGHT_PIN, LOW);
  isLightOn = false;
}

// Обработка нажатий клавиши режимов обогрева руля
void processHeatButton() {
  // Если кнопка нажата
  if(digitalRead(SWITCH_HEAT_PIN) == HIGH) {
    // Если не была нажата до этого
    if(!processHeatPress && !isHeatButtonPress) {
      // Запускаем таймер защиты от дребезга
      buttonHeatTime = currentTime;
      processHeatPress = true;
    }

    // Если была нажата до этого в течении 50 мс
    if(processHeatPress && !isHeatButtonPress && currentTime >= (buttonHeatTime + FALSE_PUSH_TIME)) {
      // Считаем, что кнопка действительно нажата
      isHeatButtonPress = true;
    }

    // Если кнопка действительно нажата и уже удерживается в течении 1000 мс
    if(processHeatPress && isHeatButtonPress && !isHeatButtonLongPress && currentTime >= (buttonHeatTime + LONG_HEAT_PUSH_TIME)) {
      isHeatButtonLongPress = true;
      toggleHeatState();
    }
  } 
  // Если кнопка НЕ нажата
  else {
    // Если до этого было нажатие короче 50 мс
    if(processHeatPress && !isHeatButtonPress) {
      // Считаем, что это дребезг контактов и ничего не делаем
      isHeatButtonPress = false;
      processHeatPress = false;
    }

    // Если до этого было нажатие длинее 50 см
    if(isHeatButtonPress && processHeatPress) {
      // Если это было нажатие длинее 1000 мс
      if(isHeatButtonLongPress) {

      } 
      else {
        // Если включен авто режим, то ручное управление его выключает
        if(isHeatAuto) {
          isHeatAuto = false;
          setHeatStatusLed(isHeatAuto);
        }
        if(isHeatOn) {
          stopHeat();
        }
        else {
          startHeat();
        }
      }

      // Останавливаем таймеры и обнуляем переменные
      isHeatButtonPress = false;
      processHeatPress = false;
      isHeatButtonLongPress = false;
      buttonHeatTime = 0;
    }
  }
}

// Обработка температуры руля
void processEnemyHeat() {
  double heatTemp = analogRead(HEAT_PIN);
  heatTemp = heatTemp/1024*5*100;

  if(heatTemp > HEAT_SCATTER_MIN && heatTemp < HEAT_SCATTER_MAX) {
    if(isHeatOnNow) {
      heatTemp = heatTemp + HEAT_CORRECT;
    }
    if(trueTempDone && (heatTemp+HEAT_SCATTER >= trueTemp || heatTemp-HEAT_SCATTER <= trueTemp)) {
      for(int i = 0; i < HEAT_MAXSAVE-1; i++) {
        oldTemps[i] = oldTemps[i+1];
      }
    }
    oldTemps[HEAT_MAXSAVE-1] = heatTemp;
    if(tempIterator < HEAT_MAXSAVE) {
      tempIterator++;
    }
  }
  if(tempIterator == HEAT_MAXSAVE) {
    trueTempDone = true;
  }
  trueTemp = 0;
  for(int i = 0; i < HEAT_MAXSAVE; i++) {
    trueTemp = trueTemp + oldTemps[i];
  }
  trueTemp = trueTemp / HEAT_MAXSAVE;
  //Serial.print(trueTemp);
  //Serial.print(" ~ ");
  //Serial.println(heatTemp);
  
  // Если набили среднюю температуру с датчика в массив
  if(trueTempDone) {
    // Если режим авто и температура меньше необходимой
    if(isHeatAuto && trueTemp < HEAT_MIN) {
      // Если обогрев руля выключен, запускаем таймер защиты от дребезга
      if(!isHeatOn && !processHeat) {
        heatTime = currentTime;
        processHeat = true;
      }
      
      // Если температура всё ещё меньше необходимой, включаем обогрев
      if(!isHeatOn && processHeat && currentTime >= (heatTime + FALSE_PUSH_TIME)) {
        startHeat();
        processHeat = false;
      }
    } 
    // Если режим авто и температура больше необходимой
    else if (isHeatAuto && trueTemp > HEAT_MAX) {
      // Если обогрев включен, запускаем таймер защиты от дребезга
      if(isHeatOn && !processHeat) {
        heatTime = currentTime;
        processHeat = true;
      }
      
      // Если температура всё ещё больше необходимой, то выключаем обогрев
      if(isHeatOn && processHeat && currentTime >= (heatTime + FALSE_PUSH_TIME)) {
        stopHeat();
        processHeat = false;
      }
    }
  }
}

// Переключает автоматические режимы обогрева руля
void toggleHeatState() {
  isHeatAuto = !isHeatAuto;
  
  EEPROM.write(STATE_HEAT_MEM, isHeatAuto);
  
  setHeatStatusLed(isHeatAuto);
}

// Включает обогрев руля и индицирует это светодиодом
void startHeat() {
  setHeatActiveLed(true);
  digitalWrite(OUR_HEAT_PIN, HIGH);
  heatingTime = millis();
  isHeatOnNow = true;
  isHeatOn = true;
}

// Выключает обогрев руля и индицирует это светодиодом
void stopHeat() {
  setHeatActiveLed(false);
  digitalWrite(OUR_HEAT_PIN, LOW);
  isHeatOnNow = false;
  isHeatOn = false;
}

// Включает или выключает обогрев для сохранения улитки
void processHeatSafe() {
  if(isHeatOn) {
    heatingLoopTime = millis();
    //Serial.println(heatingLoopTime - heatingTime);
    //Serial.println(isHeatOnNow);
    if(isHeatOnNow) {
      if(heatingLoopTime - heatingTime > HEAT_PERIOD_ON) {
        isHeatOnNow = false;
        digitalWrite(OUR_HEAT_PIN, LOW);
        heatingTime = millis();
      }
    } else {
      if(heatingLoopTime - heatingTime > HEAT_PERIOD_OFF) {
        isHeatOnNow = true;
        digitalWrite(OUR_HEAT_PIN, HIGH);
        heatingTime = millis();
      }
    }
  }
}

// Обработка нажатий клавиши включения камеры
void processCameraButton() {
  // Если кнопка нажата
  if(digitalRead(SWITCH_CAMERA_PIN) == LOW) {
    // Если не была нажата до этого
    if(!processCameraPress && !isCameraButtonPress) {
      // Запускаем таймер защиты от дребезга
      buttonCameraTime = currentTime;
      processCameraPress = true;
    }

    // Если была нажата до этого в течении 50 мс
    if(processCameraPress && !isCameraButtonPress && currentTime >= (buttonCameraTime + FALSE_PUSH_TIME)) {
      // Считаем, что кнопка действительно нажата
      isCameraButtonPress = true;
    }
  } 
  // Если кнопка НЕ нажата
  else {
    // Если до этого было нажатие короче 50 мс
    if(processCameraPress && !isCameraButtonPress) {
      // Считаем, что это дребезг контактов и ничего не делаем
      isCameraButtonPress = false;
      processCameraPress = false;
    }

    // Если до этого было нажатие длинее 50 см
    if(isCameraButtonPress && processCameraPress) {
      // Кнопка нажата и отжата, переключаем камеру!
      toggleCameraState();

      // Останавливаем таймеры и обнуляем переменные
      isCameraButtonPress = false;
      processCameraPress = false;
      buttonCameraTime = 0;
    }
  }
}

// Переключает переднюю камеру
void toggleCameraState() {
  isCameraOn = !isCameraOn;
  
  // Если нужно включить камеру
  if(isCameraOn) {
    // То если не включен задний ход, включаем её
    if(!isRearOn) {
      startCamera();
    }
  // Если нужно выключить камеру
  } else {
    stopCamera();
  }
}

// Включает переднюю камеру
void startCamera() {
  digitalWrite(OUR_CAMERA_PIN, HIGH);
}

// Выключает переднюю камеру
void stopCamera() {
  digitalWrite(OUR_CAMERA_PIN, LOW);
}

// Обработка включения заднего хода
void processEnemyCamera() {
  // Если задний ход включен
  if(digitalRead(REAR_PIN) == HIGH) {
    // Запускаем таймер защиты от дребезга
    if(!isRearOn && !processRear) {
      rearTime = currentTime;
      processRear = true;
    }
    
    // Если задний ход всё ещё включен, то утверждаем это
    if(!isRearOn && processRear && currentTime >= (rearTime + FALSE_PUSH_TIME)) {
      isRearOn = true;
      processRear = false;
      
      // Если была включена камера, выключаем её
      if(isCameraOn) {
        stopCamera();
      }
    }
  // Если задний ход выключен
  } else {
    // Запускаем таймер защиты от дребезга
    if(isRearOn && !processRear) {
      rearTime = currentTime;
      processRear = true;
    }
    
    // Если задний ход всё ещё выключен, то утверждаем это
    if(isRearOn && processRear && currentTime >= (rearTime + FALSE_PUSH_TIME)) {
      isRearOn = false;
      processRear = false;
      
      // Если до этого была включена камера, включаем её
      if(isCameraOn) {
        startCamera();
      }
    }
  }
}

void setLeds() {
  // устанавливаем синхронизацию "защелки" на LOW
  digitalWrite(LATCH_PIN, LOW);
  // передаем последовательно на dataPin
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, indicator);
  // "защелкиваем" регистр, тем самым устанавливая значения на выходах
  digitalWrite(LATCH_PIN, HIGH);
}

void setWetStatusLed(boolean status) {
  if(status) {
    indicator = indicator | 2;
  } 
  else {
    indicator = indicator ^ 2;
  }
  setLeds();
}

void setWetActiveLed(boolean status) {
  if(status) {
    indicator = indicator | 4;
  } 
  else {
    indicator = indicator ^ 4;
  }
  setLeds();
}

void setLightStatusLed(boolean status) {
  if(status) {
    indicator = indicator | 8;
  } 
  else {
    indicator = indicator ^ 8;
  }
  setLeds();
}

void setLightActiveLed(boolean status) {
  if(status) {
    indicator = indicator | 16;
  } 
  else {
    indicator = indicator ^ 16;
  }
  setLeds();
}

void setHeatStatusLed(boolean status) {
  if(status) {
    indicator = indicator | 32;
  } 
  else {
    indicator = indicator ^ 32;
  }
  setLeds();
}

void setHeatActiveLed(boolean status) {
  if(status) {
    indicator = indicator | 64;
  } 
  else {
    indicator = indicator ^ 64;
  }
  setLeds();
}


