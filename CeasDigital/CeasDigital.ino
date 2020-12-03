#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <util/delay.h>
#include "ceas.h"

#define DHTPIN 8
#define DHTTYPE DHT11

LiquidCrystal_I2C lcd(0x27,16,2);
DHT dht(DHTPIN, DHTTYPE);
volatile boolean backlight_on = 1;

uint8_t clocksym[8] = {
  B00100,
  B01110,
  B01110,
  B01110,
  B11111,
  B00000,
  B00100,
  B00000};
const String clockModes[] = {"SET O", "SET M", "SET S", "SET L", "SET Z", "SET A", "SET ", "SET O", "SET M"}; // ultimele 3 sunt pentru alarma
volatile int zile_luna [] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const int btns[] = {2, 3, 5};
const int buzz = 6;

volatile int clockModeIndex = 0;

volatile int oraAlarma = 15, minutAlarma = 21, almAct = 1, almArm = 1, buzzAct = 0;

void setup()
{
  Serial.begin(9600);
  pinMode(RTC_CE, OUTPUT);
  pinMode(RTC_CLK, OUTPUT);
  pinMode(RTC_DATA, OUTPUT);

  pinMode(btns[0], INPUT);
  pinMode(btns[1], INPUT);
  pinMode(btns[2], INPUT);
  pinMode(buzz, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(2),ISR_MODE,FALLING);
  attachInterrupt(digitalPinToInterrupt(3),ISR_UP,FALLING);
  PCICR  |= (1 << PCIE2);
  PCMSK2 |= (1<<PCINT21);
  
  lcd.init();
  lcd.backlight();
  lcd.createChar(1, clocksym);
  dht.begin();
  //setInitialState();
  
}

void renderLCD(){
  if(backlight_on)
    lcd.backlight();
  else
    lcd.noBacklight();
  char lcd_buffer[32];
  if(clockModeIndex >= 7){
    sprintf(lcd_buffer, "%02d:%02d   ", oraAlarma, minutAlarma);
  }
  else{
    if(am_pmmode == 0) //24 ore
    {
      sprintf(lcd_buffer, "%02d:%02d:%02d  ", ora, minute, secunde);
    }
    else
    {
      if(am){
        sprintf(lcd_buffer, "%02d:%02d:%02dAM", ora, minute, secunde);
      }
      else{
        sprintf(lcd_buffer, "%02d:%02d:%02dPM", ora, minute, secunde);
      }
      
    }
  }
  lcd.setCursor(0, 0);
  lcd.print(lcd_buffer);
  lcd.setCursor(12, 0);
  float temp = dht.readTemperature();
  lcd.print((int)temp);
  lcd.setCursor(14, 0);
  lcd.print((char)223);
  lcd.setCursor(15, 0);
  lcd.print("C");
  sprintf(lcd_buffer, "%02d/%02d/%02d", zi, luna, an);
  
  lcd.setCursor(11, 0);
  if(almAct){
    lcd.write(1);
  }
  else{
    lcd.write(' ');
  }
  lcd.setCursor(0, 1);
  lcd.print(lcd_buffer);
  if(clockModeIndex == 0){
    lcd.setCursor(11, 1);
    lcd.print("  ");
    lcd.setCursor(13, 1);
    lcd.print(zileSapt[ziuaSapt]);
  }
  else{
    lcd.setCursor(11, 1);
    lcd.print(clockModes[clockModeIndex - 1]);
    if(clockModeIndex == 7)
      lcd.write(1);
  }
}

void loop()
{
  delay(500);
  if(ora == oraAlarma && minute == minutAlarma){
    if(almArm == 1 && almAct == 1){
      buzzAct = !buzzAct;
      if(buzzAct){
        tone(buzz, 500);
     }
     else{
       noTone(buzz);
     }
    }
    else{
      noTone(buzz);
    }
  }
  else{
    almArm = 1;
    noTone(buzz);
  }
 
  if(clockModeIndex == 0){
    RTC_burst_read();
  }
    
  renderLCD();

}

void ISR_MODE(){
  _delay_ms(150);
  if(!(PIND & 0x4))
    clockModeIndex = (clockModeIndex + 1) % 10;
}

void ISR_UP(){
   _delay_ms(150);
  if((PIND & 0x8))
    return;
  
  if (clockModeIndex!=0)
    switch (clockModeIndex) {
      case 5:
        zi++;
        if (zi==zile_luna[luna-1]) zi=1;
        setZi();
        break;
      case 4:
        luna++;
        if (luna==13) luna=1;
        if (zi>zile_luna[luna-1]) zi=zile_luna[luna-1];
        setZi();
        setLuna();
        break;
      case 6:
        an++;
        if (an==2020) an=2013;
        if (an%4 == 0) zile_luna[1] = 29;
        else zile_luna[1] = 28;
        if (zi>zile_luna[luna-1]) zi=zile_luna[luna-1];
        setZi();
        setAn();
        break;
      case 1:
        ora++;
        if(am_pmmode == 0){
          if (ora>=24) ora=0;
        }
        else{
          if(ora == 13){
            ora = 0;
            am = (am + 1) % 2;
          }
        }
        setOra();
        break;
      case 2:
        minute++;
        if (minute==60) minute=0;
        setMinute();
        break;
      case 3:
        secunde++;
        if (secunde==60) secunde=0;
        setSecunde();
        break;
      case 7:
        almAct = !almAct;
        break;
      case 8:
        oraAlarma = (oraAlarma + 1) % 24;
        break;
      case 9:
        minutAlarma = (minutAlarma + 1) % 60;
        break;
    }
}

ISR(PCINT2_vect) {
  if(ora == oraAlarma && minute == minutAlarma){
    almArm = 0;
    return;
  }
  _delay_ms(150);
 if (!(PIND & 0x20)) 
 {
  if(clockModeIndex == 0){
    if (backlight_on) {
     backlight_on = false; 
   }
   else {
     backlight_on = true;
   }
  }
  else{ // Comuta intre AM si PM
    if(am_pmmode == 1){
      am_pmmode = 0;
      ora = (ora + 12 * (!am))%24;
    }
    else{
      am_pmmode = 1;
      am = (ora > 12);
      if(ora > 12){
        ora -= 12;
      }
    }
    setOra();
  }
   
 }
}
