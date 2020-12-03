#define RTC_CE 11
#define RTC_CLK 9
#define RTC_DATA 10

#define RTC_BURST_READ 0xBF
#define RTC_SEC_READ 0x81
#define RTC_MIN_READ 0x83
#define RTC_ORA_READ 0x85
#define RTC_DATA_READ 0x87
#define RTC_LUNA_READ 0x89
#define RTC_ZSPT_READ 0x8B
#define RTC_AN_READ 0x8D

volatile byte zi, luna, an, ziuaSapt, ora, minute, secunde, am, am_pmmode;

String zileSapt[] = { "DUM", "LUN", "MAR", "MIE", "JOI", "VIN", "SAM"};

#define LEAP_YEAR(Y)     ( (Y>0) && !(Y%4) && ( (Y%100) || !(Y%400) ))
int ziDinData(){
  uint16_t months[] =
  {
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 // days until 1st of month
  };   

  uint32_t days = an * 365;   // days until year
  for (uint16_t i = 4; i < an; i+=4)
  {
    if (LEAP_YEAR(i)) days++;  // adjust leap years
  }

  days += months[luna-1] + zi;
  if ((luna > 2) && LEAP_YEAR(an)) days++;
  return days % 7;
}

byte clockReadRegister(byte reg){
  digitalWrite(RTC_CE, 0);
  byte clockData = 0;

  pinMode(RTC_DATA, OUTPUT);
  // init clock 0 pentru a trasmite pe primul front crescator
  digitalWrite(RTC_CLK, 0);
  // CE high ca sa inceapa transmisia
  digitalWrite(RTC_CE, 1);
  for(int j = 0;j<8;j++){
      digitalWrite(RTC_CLK, 0);
      digitalWrite(RTC_DATA, reg & (1 << j));
      digitalWrite(RTC_CLK, 1);
      
   }
  // Citim pe rand toti cei 8 biti ai registrului
  pinMode(RTC_DATA, INPUT);
    for(int j = 0;j<8;j++){
      digitalWrite(RTC_CLK, 0);
      clockData = clockData + (digitalRead(RTC_DATA) << j);
      digitalWrite(RTC_CLK, 1);
    }
  // Finalizam tranzactia cu ceasul
  digitalWrite(RTC_CE, 0);
  return clockData;
}

void clockWriteRegister(byte reg, byte val){
  digitalWrite(RTC_CE, 0);
  pinMode(RTC_DATA, OUTPUT);
  // init clock 0 pentru a trasmite pe primul front crescator
  digitalWrite(RTC_CLK, 0);
  // CE high ca sa inceapa transmisia
  digitalWrite(RTC_CE, 1);
  for(int j = 0;j<8;j++){
      digitalWrite(RTC_CLK, 0);
      delayMicroseconds(1);
      digitalWrite(RTC_DATA, (reg & (1 << j)) >> j);
      digitalWrite(RTC_CLK, 1);
      delayMicroseconds(1);
      
   }
  for(int j = 0;j<8;j++){
      digitalWrite(RTC_CLK, 0);
      delayMicroseconds(1);
      digitalWrite(RTC_DATA, (val & (1 << j)) >> j);
      digitalWrite(RTC_CLK, 1);
      delayMicroseconds(1);
      
   }
  // Finalizam tranzactia cu ceasul
  digitalWrite(RTC_CE, 0);
}

void startClock(){
  clockWriteRegister(RTC_SEC_READ - 1, secunde & 0b0111111);
}

void setSecunde(){
  clockWriteRegister(RTC_SEC_READ - 1, secunde % 10 + ((secunde / 10) << 4));
}
void setMinute(){
  clockWriteRegister(RTC_MIN_READ - 1, minute % 10 + ((minute / 10) << 4));
}

void setOra(){
  byte data = 0;
  if(am_pmmode == 1)// 12 ore
  {
    data = ora + (am << 5) + (am_pmmode) << 8;
  }
  else
  {
    data = ora%10 + ((ora / 10) << 4);
  }
  clockWriteRegister(RTC_ORA_READ - 1, data);
}

void setZi(){
  clockWriteRegister(RTC_DATA_READ - 1, zi % 10 + ((zi / 10) << 4));
}

void setLuna(){
  clockWriteRegister(RTC_LUNA_READ - 1, luna % 10 + ((luna / 10) << 4));
}

void setAn(){
  clockWriteRegister(RTC_AN_READ - 1, an % 10 + ((an / 10) << 4));
}

void setZiSapt(){
  clockWriteRegister(RTC_ZSPT_READ - 1, ziuaSapt);
}

// Citirea datelor din registrii RTC in mod burst

void RTC_burst_read(){
  byte clockData[8] = {0};
  digitalWrite(RTC_CE, 0);

  pinMode(RTC_DATA, OUTPUT);
  // init clock 0 pentru a trasmite pe primul front crescator
  digitalWrite(RTC_CLK, 0);
  // CE high ca sa inceapa transmisia
  digitalWrite(RTC_CE, 1);
  for(int j = 0;j<8;j++){
      digitalWrite(RTC_CLK, 0);
      delayMicroseconds(1);
      digitalWrite(RTC_DATA, (RTC_BURST_READ & (1 << j)) >> j);
      digitalWrite(RTC_CLK, 1);
      delayMicroseconds(1);
      
   }
  // Citim pe rand toti cei 8 registri ai ceasului
  pinMode(RTC_DATA, INPUT);
  for(int i=0;i<8;i++){
    for(int j = 0;j<8;j++){
      digitalWrite(RTC_CLK, 0);
      delayMicroseconds(1);
      clockData[i] = clockData[i] + (digitalRead(RTC_DATA) << j);
      digitalWrite(RTC_CLK, 1);
      delayMicroseconds(1);
      
    }
    Serial.println(clockData[i], BIN);
  }
  // Finalizam tranzactia cu ceasul
  digitalWrite(RTC_CE, 0);
  secunde = (clockData[0] & 0b1111) + ((clockData[0] & (0b111 << 4)) >> 4) * 10;
  minute = (clockData[1] & 0b1111) + ((clockData[1] & (0b111 << 4)) >> 4) * 10;
  am_pmmode = (clockData[2] & (1<< 7)) >> 7;
  if(am_pmmode == 1) //12 ore
  {
    ora = (clockData[2] & 0b11111) + ((clockData[2] & (0b1 << 4)) >> 4) * 10;
    am = !clockData[2] & (1<< 5);
  }
  else // 24 ore
  {
    ora = (clockData[2] & 0b1111) + ((clockData[2] & (0b11 << 4)) >> 4) * 10;
  }
  zi = (clockData[3] & 0b1111) + ((clockData[3] & (0b11 << 4)) >> 4) * 10;
  luna = (clockData[4] & 0b1111) + ((clockData[4] & (0b1 << 4)) >> 4) * 10;
  ziuaSapt = clockData[5];
  an = (clockData[6] & 0b1111) + ((clockData[6] & (0b1111 << 4)) >> 4) * 10;
  ziuaSapt = ziDinData();
  
}

void setInitialState(){
  zi = 5;
  luna = 12;
  an = 20;
  ziuaSapt = 1;
  ora = 15;
  minute = 22;
  secunde = 0;
  am = 0;
  am_pmmode = 0;
  setSecunde();
  setMinute();
  setOra();
  setZi();
  setLuna();
  setZiSapt();
  setAn();
}
