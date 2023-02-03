#include <MsTimer2.h>
//Для считывания
#define ENGINE_DATA_PIN 2 // pin 2
#define ENGINE_DATA_INT 0  // для прерываний
#define OBD_DATA true
#define  MY_HIGH  HIGH //LOW //если пользуетесь только одним проводом, 
#define  MY_LOW   LOW //HIGH //поменять хай и лоу местами, если витой парой, то оставить как есть
#define  TOYOTA_MAX_BYTES  24

volatile uint8_t ToyotaNumBytes, ToyotaID;
volatile uint8_t ToyotaData[TOYOTA_MAX_BYTES];
volatile uint16_t ToyotaFailBit = 0;





// облегчаем себе жизнь
#define OBD_INJ 1 //Injector pulse width (INJ)
#define OBD_IGN 2 //Ignition timing angle (IGN)
#define OBD_IAC 3 //Idle Air Control (IAC)
#define OBD_RPM 4 //Engine speed (RPM)
#define OBD_MAP 5 //Manifold Absolute Pressure (MAP)
#define OBD_ECT 6 //Engine Coolant Temperature (ECT)
#define OBD_TPS 7 // Throttle Position Sensor (TPS)
#define OBD_SPD 8 //Speed (SPD)

float g_power = 0.055; // производительнсть форсунки на 3S FE за 15 сек //ДЛЯ ДРУГОГО АВТО ИЗМЕНИТЕ ЭТОТ ПАРАМЕТР!!!!!!!!!!!!!!
float g_fors_cnt = 4;  // количество форсунок                           //ДЛЯ ДРУГОГО АВТО ИЗМЕНИТЕ ЭТОТ ПАРАМЕТР!!!!!!!!!!!!!!
float g_fors_col = 2;  // кол-во открытий за 2 оборота                  //ДЛЯ ДРУГОГО АВТО ИЗМЕНИТЕ ЭТОТ ПАРАМЕТР!!!!!!!!!!!!!!
float per_sec; // число оборотов в секунду
float oborot_cnt; // число оборотов
float fors_time; // время открытия форсунки в сек
float rashod; //расход авто
float per_hous; // расход за час
float time_100km; // время которое нужно, чтобы проехать 100 км
int   per_100km; // расход на сотку
int   timers = 1;
int   full_time = 0;
float tmLast = 0;
float tmNow = 0;
float dPeriod;

boolean       OBDConnected;
unsigned long OBDLastSuccessPacket;

//Для передачи в торк

String  inputString     = "";
boolean stringComplete = false;

void    timer(){full_time ++;}

void setup() {
  MsTimer2::set(1000, timer);
  
  Serial.begin(9600);
  inputString.reserve(200);
  Serial.println(">");
  // настройка входа
  pinMode(ENGINE_DATA_PIN, INPUT); // подтяжка
  //настройка прерываний
  attachInterrupt(ENGINE_DATA_INT, ChangeState, CHANGE);
//начальные установки таймера c момента включения
}


void loop() {
  
if ((ToyotaData[OBD_RPM] > 0) && timers == 1) {
  MsTimer2::start();
  timers = 0;
}
else if ((ToyotaData[OBD_RPM] < 1) && timers == 0) {
  MsTimer2::stop();
 timers = 1 ; 
 full_time = 0;
 }
  //считываем протокол
  if (ToyotaNumBytes > 0)  {

    // set last success
    OBDLastSuccessPacket = millis();
    // set connected to true
    OBDConnected = true;
    // reset the counter.
    ToyotaNumBytes = 0;
  } // end if (ToyotaNumBytes > 0)
   // Ведем косвенные подсчеты
   //реальная температура
  int OBD_ECTi;
  if (ToyotaData[OBD_ECT] >= 244)
    OBD_ECTi = ((ToyotaData[OBD_ECT] - 244) * 10.0) + 174.0;
  else if (ToyotaData[OBD_ECT] >= 238)
    OBD_ECTi = ((ToyotaData[OBD_ECT] - 238) * 4.0) + 145.0;
  else if (ToyotaData[OBD_ECT] >= 228)
    OBD_ECTi = ((ToyotaData[OBD_ECT] - 228) * 2.1) + 122.0;
  else if (ToyotaData[OBD_ECT] >= 210)
    OBD_ECTi = ((ToyotaData[OBD_ECT] - 210) * 1.11) + 102.0;
  else if (ToyotaData[OBD_ECT] >= 180)
    OBD_ECTi = ((ToyotaData[OBD_ECT] - 180) * 0.666) + 82.0;
  else if (ToyotaData[OBD_ECT] >= 135)
    OBD_ECTi = ((ToyotaData[OBD_ECT] - 135) * 0.444) + 62.0;
  else if (ToyotaData[OBD_ECT] >= 82)
    OBD_ECTi = ((ToyotaData[OBD_ECT] - 82) * 0.377) + 42.0;
  else if (ToyotaData[OBD_ECT] >= 39)
    OBD_ECTi = ((ToyotaData[OBD_ECT] - 39) * 0.465) + 22.0;
  else if (ToyotaData[OBD_ECT] >= 15)
    OBD_ECTi = ((ToyotaData[OBD_ECT] - 15) * 0.833) + 2.0;
  else
    OBD_ECTi = (ToyotaData[OBD_ECT] * 2.0) + (-28.0);


 /* -----------------------------  НЕ ЗНАЮ КАК ПОСЧИТАТЬ ПРАВИЛЬНО РАСХОД, ЕСЛИ КТО ПОДСКАЖЕТ, ВОТ МОЙ email Bondy)_13@list.ru --------------------------------         */
          
          tmLast = 1;
          tmNow = 2;
          dPeriod = (tmNow - tmLast) / ToyotaData[OBD_SPD];
          
          // число оборотов в сек
          per_sec = (ToyotaData[OBD_RPM]*25) / 60.0;
          // сколько было произведено оборотов
          oborot_cnt = per_sec * dPeriod;
          // время открытия форсунки в сек
          fors_time = (oborot_cnt / 2) * g_fors_col * g_fors_cnt * (ToyotaData[OBD_INJ]/10) / 1000.0;
          // сколько было израсходовано за последний интервал времени
          rashod = fors_time * (g_power / 15.0);
          // сколько будет израсходовано за час
          per_hous = rashod * 3600 / dPeriod;
          // сколько требуется времени чтобы проехать 100 км (в секундах)
          time_100km = (100.0 / ToyotaData[OBD_SPD]) * 3600.0;
          // расход на 100км
          per_100km = 0.0;
          if (ToyotaData[OBD_SPD] > 0) {
            per_100km = rashod * time_100km / dPeriod;
          }
          
 /*---------------------------------------------------------------------------------------------------------------------------------------------      */
 
 
  //check for lost connection
  if (OBDLastSuccessPacket + 3500 < millis() && OBDConnected) {
    OBDConnected = false;
  } // end if loas conntcion

  //передаем в торк
  if (stringComplete) {
    int    len = inputString.length();
    String ans = "4" + inputString.substring(1, 2) + " ";

    if (len > 4)      ans = ans + inputString.substring(3, 5) + " ";
    else if (len > 3) ans = ans + inputString.substring(2, 4) + " ";
    else if (len > 2) ans = ans + inputString.substring(3, 4) + " ";

    if           (inputString.substring(0, 3) == "ATZ")   Serial.println("ELM327 v1.4");
    else if      (inputString.substring(0, 3) == "ATI")   Serial.println("ELM327 v1.4");
    else if      (inputString.substring(0, 2) == "DP")    Serial.println("SAE J1850 PWM");
    else if      (inputString.substring(0, 3) == "DP1")   Serial.println("SAE J1850 PWM");
    else if      (inputString.substring(0, 4) == "AT@1")  Serial.println("made Alexey Bondarenko");
    else if      (inputString.substring(0, 4) == "ATTP1") Serial.println("OK");
    else if (inputString.substring(0, 2) == "AT")    Serial.println("OK");
    else if (inputString.substring(0, 4) == "0100")  Serial.println("41 00 08 38 80 02"); //поддерживаемые пиды 1 группы
    else if (inputString.substring(0, 4) == "0101")  Serial.println("41 01 00 00 00 00");
    else if (inputString.substring(0, 4) == "0120")  Serial.println("41 20 00 00 00 00"); // поддерживаемые пиды 2й группы
    else if (inputString.substring(0, 4) == "0140")  Serial.println("41 40 00 00 80 00"); // поддерживаемые пиды 3й группы
    else if (inputString.substring(0, 4) == "0151")  Serial.println("41 51 01 00 00 00"); // тип топлива, тут стоит бензин на данный момент, дизель будет "41 51 04 00 00 00"
    else if (inputString.substring(0, 4) == "0160")  Serial.println("41 60 00 00 00 00"); // поддерживаемые пиды 4й группы
    else if (inputString.substring(0, 4) == "0180")  Serial.println("41 80 00 00 00 00"); // поддерживаемые пиды 5й группы
    else if (inputString.substring(0, 4) == "01A0")  Serial.println("41 A0 80 00 00 00"); //поддерживаемые пиды 6й группы
    else if (inputString.substring(0, 4) == "01C0")  Serial.println("41 C0 00 00 00 00"); // поддерживаемые пиды 7й группы
    else if (inputString.substring(0, 2) == "03")    Serial.println("43 03 00 00 00 00\r\n43 13 00 00"); //типа ошибки в машине....
    else if (inputString.substring(0, 4) == "0902") Serial.println("49 02 1Z3768470804"); // серийный номер авто... ВИН
  //вывод температуры ОЖ
       if ((OBD_ECTi > (-16)) && (OBD_ECTi <16)) {if (inputString.substring(0, 4) == "0105")  Serial.print("41 05 "), Serial.print("0"), Serial.println(OBD_ECTi, HEX);} 
            else {if (inputString.substring(0, 4) == "0105")  Serial.print("41 05 "), Serial.println(OBD_ECTi, HEX); }
  //Правильные обороты двигателя
       if ((ToyotaData[OBD_RPM] * 100 / 4) < 1001) { if (inputString.substring(0, 4) == "010C") Serial.print("41 0C "), Serial.print("0"), Serial.println(ToyotaData[OBD_RPM] * 100, HEX);}
            else {if (inputString.substring(0, 4) == "010C") Serial.print("41 0C "), Serial.println(ToyotaData[OBD_RPM] * 100, HEX);} //обороты двигателя
  
  // угол положения заслонки
       if (ToyotaData[OBD_TPS] <10) { if (inputString.substring(0, 4) == "0111") Serial.print("41 11 "), Serial.print("0"), Serial.println(ToyotaData[OBD_TPS]*255/100, HEX);} // угол положения заслонки
            else { if (inputString.substring(0, 4) == "0111") Serial.print("41 11 "), Serial.println(ToyotaData[OBD_TPS]*255/100, HEX);}
  
  //Скорость автомобиля
       if (ToyotaData[OBD_SPD] < 16) { if (inputString.substring(0, 4) == "010D") Serial.print("41 0D "), Serial.print("0"), Serial.println(ToyotaData[OBD_SPD], HEX);} //////Скорость автомобиля
            else { if (inputString.substring(0, 4) == "010D") Serial.print("41 0D "), Serial.println(ToyotaData[OBD_SPD], HEX);} //////Скорость автомобиля
 
 //Время с момента запуска двигателя
    if (full_time < 16) {if (inputString.substring(0, 4) == "011F") Serial.print("41 1F 00 "), Serial.print("0"), Serial.println(full_time, HEX);}
    else if (( full_time > 15) && (256 >full_time)) {if (inputString.substring(0, 4) == "011F") Serial.print("41 1F 00 "), Serial.println(full_time, HEX);}
    else if ((full_time > 255) && (4096 > full_time)) { if (inputString.substring(0, 4) == "011F") Serial.print("41 1F 0"), Serial.println(full_time, HEX);}
    else if (full_time > 4096) { if (inputString.substring(0, 4) == "011F") Serial.print("41 1F "), Serial.println(full_time, HEX);}          

   //угол опережения зажигания
    if ((ToyotaData[OBD_IGN] + 50) < 10) {if (inputString.substring(0, 4) == "010E") Serial.print("41 0E "), Serial.print("0"), Serial.println((ToyotaData[OBD_IGN] + 50), HEX);}
      else {if (inputString.substring(0, 4) == "010E") Serial.print("41 0E "), Serial.println((ToyotaData[OBD_IGN] + 50), HEX);}
  // время форсунок
       if (ToyotaData[OBD_INJ] < 16) {if (inputString.substring(0, 4) == "01E4")  Serial.print("41 E4 "), Serial.print("0"),Serial.println(ToyotaData[OBD_INJ], HEX);}
            else {if (inputString.substring(0, 4) == "01E4")  Serial.print("41 E4 "), Serial.println(ToyotaData[OBD_INJ], HEX);}
  //Давление во впускном коллекторе
       if (ToyotaData[OBD_MAP] < 16) {if (inputString.substring(0, 4) == "010B")  Serial.print("41 0B "), Serial.print("0"),Serial.println(ToyotaData[OBD_MAP], HEX);}
            else {if (inputString.substring(0, 4) == "010B")  Serial.print("41 0B "), Serial.println(ToyotaData[OBD_MAP], HEX);}
//Вращение вентилятора
       if (ToyotaData[OBD_IAC] < 16) {if (inputString.substring(0, 4) == "01A2")  Serial.print("41 A2 "), Serial.print("0"),Serial.println(ToyotaData[OBD_IAC], HEX);}
            else {if (inputString.substring(0, 4) == "01A2")  Serial.print("41 A2 "), Serial.println(ToyotaData[OBD_IAC], HEX);}
//Расход в час
       if (per_hous < 16) {if (inputString.substring(0, 4) == "01A3")  Serial.print("41 A3 "), Serial.print("0"),Serial.println(per_hous, HEX);}
            else {if (inputString.substring(0, 4) == "01A3")  Serial.print("41 A3 "), Serial.println(per_hous, HEX);}
 //расход на 100 км
        if (per_100km < 16) {if (inputString.substring(0, 4) == "01A4")  Serial.print("41 A4 "), Serial.print("0"),Serial.println(per_100km, HEX);}
            else {if (inputString.substring(0, 4) == "01A4")  Serial.print("41 A4 "), Serial.println(per_100km, HEX);}              
    
    Serial.println(">");
    inputString = "";
    stringComplete = false;
  }
}



// GET DATA FROM OBD
float getOBDdata(int OBDdataIDX) {
  // define return value
  float returnValue;
  switch (OBDdataIDX) {
    case 0:// UNKNOWN
      returnValue = ToyotaData[0];
      break;
    case OBD_INJ: //  Injector pulse width (INJ) - in milisec
      returnValue = ToyotaData[OBD_INJ] / 10;
      break;
    case OBD_IGN: // Ignition timing angle (IGN) - degree- BTDC
      returnValue = ToyotaData[OBD_IGN] - 90;
      break;
    case OBD_IAC: //Idle Air Control (IAC) - Step # X = 125 = open 100%
      returnValue = ToyotaData[OBD_IAC] / 125 * 100;
      break;
    case OBD_RPM: //Engine speed (RPM)
      returnValue = ToyotaData[OBD_RPM] * 25;
      break;
    case OBD_MAP: //Manifold Absolute Pressure (MAP) - kPa Abs
      returnValue = ToyotaData[OBD_MAP];
      break;
    case OBD_ECT: // Engine Coolant Temperature (ECT)
      if (ToyotaData[OBD_ECT] >= 244)
        returnValue = ((float)(ToyotaData[OBD_ECT] - 244) * 10.0) + 132.0;
      else if (ToyotaData[OBD_ECT] >= 238)
        returnValue = ((float)(ToyotaData[OBD_ECT] - 238) * 4.0) + 103.0;
      else if (ToyotaData[OBD_ECT] >= 228)
        returnValue = ((float)(ToyotaData[OBD_ECT] - 228) * 2.1) + 80.0;
      else if (ToyotaData[OBD_ECT] >= 210)
        returnValue = ((float)(ToyotaData[OBD_ECT] - 210) * 1.11) + 60.0;
      else if (ToyotaData[OBD_ECT] >= 180)
        returnValue = ((float)(ToyotaData[OBD_ECT] - 180) * 0.666) + 40.0;
      else if (ToyotaData[OBD_ECT] >= 135)
        returnValue = ((float)(ToyotaData[OBD_ECT] - 135) * 0.444) + 20.0;
      else if (ToyotaData[OBD_ECT] >= 82)
        returnValue = ((float)(ToyotaData[OBD_ECT] - 82) * 0.377) + 0.0;
      else if (ToyotaData[OBD_ECT] >= 39)
        returnValue = ((float)(ToyotaData[OBD_ECT] - 39) * 0.465) + (-20.0);
      else if (ToyotaData[OBD_ECT] >= 15)
        returnValue = ((float)(ToyotaData[OBD_ECT] - 15) * 0.833) + (-40.0);
      else
        returnValue = ((float)ToyotaData[OBD_ECT] * 2.0) + (-70.0);

      break;
    case OBD_TPS: // Throttle Position Sensor (TPS) - DEGREE
      returnValue = ToyotaData[OBD_TPS] / 2;
      break;
    case OBD_SPD: // Speed (SPD) - km/h
      returnValue = ToyotaData[OBD_SPD];
      break;
    case 9:// UNKNOWN
      returnValue = ToyotaData[9];
      break;
    case 10:// UNKNOWN
      returnValue = ToyotaData[10];
      break;
    case 11:// FLAG #1
      returnValue = ToyotaData[11];
      break;
    case 12:// FLAG # 2
      returnValue = ToyotaData[12];
      break;
    default: // DEFAULT CASE (in no match to number)
      // send "error" value
      returnValue =  9999.99;
  } // end switch
  // send value back
  return returnValue;
} // end void getOBDdata

// VOID CHANGE
void ChangeState()
{
  //Serial.print(digitalRead(ENGINE_DATA_PIN));
  static uint8_t ID, EData[TOYOTA_MAX_BYTES];
  static boolean InPacket = false;
  static unsigned long StartMS;
  static uint16_t BitCount;

  int state = digitalRead(ENGINE_DATA_PIN);

  if (InPacket == false)  {
    if (state == MY_HIGH)   {
      StartMS = millis();
    }   else   { // else  if (state == MY_HIGH)
      if ((millis() - StartMS) > (15 * 8))   {
        StartMS = millis();
        InPacket = true;
        BitCount = 0;
      } // end if  ((millis() - StartMS) > (15 * 8))
    } // end if  (state == MY_HIGH)
  }  else   { // else  if (InPacket == false)
    uint16_t bits = ((millis() - StartMS) + 1 ) / 8; // The +1 is to cope with slight time errors
    StartMS = millis();
    // process bits
    while (bits > 0)  {
      if (BitCount < 4)  {
        if (BitCount == 0)
          ID = 0;
        ID >>= 1;
        if (state == MY_LOW)  // inverse state as we are detecting the change!
          ID |= 0x08;
      }   else    { // else    if (BitCount < 4)
        uint16_t bitpos = (BitCount - 4) % 11;
        uint16_t bytepos = (BitCount - 4) / 11;
        if (bitpos == 0)      {

          // Start bit, should be LOW
          if ((BitCount > 4) && (state != MY_HIGH))  { // inverse state as we are detecting the change!
            ToyotaFailBit = BitCount;
            InPacket = false;
            break;
          } // end if ((BitCount > 4) && (state != MY_HIGH))

        }  else if (bitpos < 9)  { //else TO  if (bitpos == 0)

          EData[bytepos] >>= 1;
          if (state == MY_LOW)  // inverse state as we are detecting the change!
            EData[bytepos] |= 0x80;

        } else { // else if (bitpos == 0)

          // Stop bits, should be HIGH
          if (state != MY_LOW)  { // inverse state as we are detecting the change!
            ToyotaFailBit = BitCount;
            InPacket = false;
            break;
          } // end if (state != MY_LOW)

          if ( (bitpos == 10) && ((bits > 1) || (bytepos == (TOYOTA_MAX_BYTES - 1))) ) {
            ToyotaNumBytes = 0;
            ToyotaID = ID;
            for (int i = 0; i <= bytepos; i++)
              ToyotaData[i] = EData[i];
            ToyotaNumBytes = bytepos + 1;
            if (bits >= 16)  // Stop bits of last byte were 1's so detect preamble for next packet
              BitCount = 0;
            else  {
              ToyotaFailBit = BitCount;
              InPacket = false;
            }
            break;
          }
        }
      }
      ++BitCount;
      --bits;
    } // end while
  } // end (InPacket == false)
} // end void change

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') continue;
    if (inChar == '\r') {
      stringComplete = true;
      if (inputString == "") inputString = "NULL";
      inputString.toUpperCase();
      continue;
    }
    inputString += inChar;
  }
}
