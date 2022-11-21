/*
 * WakeupFormPEKKey: Use AXP202 interrupt pin to wake up T-Watch
 * Copyright 2020 Lewis he
 */

#include "config.h"
#include <WiFi.h>
#include "Free_Fonts.h"
#include <WiFiMulti.h>
#include <EEPROM.h>


WiFiMulti wifiMulti;
TFT_eSPI *tft;
AXP20X_Class *power;
bool irq = false;
TTGOClass *ttgo;
char buf[128];
String buffers;
bool rtcIrq = false;
char y[20];
char m[20];
char d[20];
char h[20];//文字格納用
char mo[20];//文字格納用
char s[20];
int y_i,mo_i,d_i,h_i,m_i,s_i;
String years,months,dayes,houres,mints,secs,adapt;
struct tm timeInf;
int eep_y,eep_mo,eep_d;
int p;
char aaa[128];


void get_NTP(){
  getLocalTime(&timeInfo);
  sprintf(y, "%02d",timeInf.tm_year + 1900);
  sprintf(mo, "%02d",timeInf.tm_mon + 1);
  sprintf(d, "%02d",timeInf.tm_mday);
  sprintf(h, "%02d",timeInf.tm_hour);
  sprintf(m, "%02d",
          timeInf.tm_min);//人間が読める形式に変換 
  sprintf(s, "%02d",
          timeInf.tm_sec);//人間が読める形式に変換 
  c_to_String();
  //intへ変換
  to_Int();
  ttgo->rtc->setDateTime(y_i, mo_i, d_i, h_i, m_i, s_i);
  save_EEP();
}


void c_to_String(){
  years=String(y);
  months=String(mo);
  dayes=String(d);
  houres=String(h);
  mints=String(m);
  secs=String(s);
}


void to_Int(){
  y_i=years.toInt();
  mo_i=months.toInt();
  d_i=dayes.toInt();
  h_i=houres.toInt();
  m_i=mints.toInt();
  s_i=secs.toInt();
}


void to_String(int a,int b, int c){
  years=String(a);
  months=String(b);
  dayes=String(c);
  if (months.length()<2){
     months="0"+months;
  };
  if (dayes.length()<2){
     dayes="0"+dayes;
  };
}


void get_EEP(){
  /* EEPROM読み出し */
  EEPROM.get(0, y_i);
  EEPROM.get(4, mo_i);
  EEPROM.get(8, d_i);
  to_String(y_i,mo_i,d_i);
}


void save_EEP(){
  EEPROM.put(0, y_i);  //書き込み内容を設定
  EEPROM.put(4, mo_i);  //書き込み内容を設定
  EEPROM.put(8, d_i);  //書き込み内容を設定
  EEPROM.commit(); 
}


void set_text(String message,int textSize,int x,int y){ //, int  delay_times){
    // Set text datum to middle centre
    ttgo->tft->setTextDatum(MC_DATUM);

    // Set text colour to orange with black background
    ttgo->tft->setTextColor(TFT_WHITE, TFT_BLACK);
    
    ttgo->tft->setTextSize(textSize);
    ttgo->tft->drawString(message, x, y, GFXFF);
    //delay(delay_times);
    //ttgo->tft->fillRect(0, 0, 33, 25, TFT_BLACK);
    ttgo->tft->setTextPadding(3);
}


void vive(int H_delay,int L_delay){
  digitalWrite(4,HIGH);
  delay(H_delay);
  digitalWrite(4,LOW);
  delay(L_delay);
}



void do_Loop_RTC(){
    //日付
    get_EEP();
    
    //時刻切り出し
    snprintf(buf, sizeof(buf), "%s", ttgo->rtc->formatDateTime());
    buffers = String(buf);
    buffers = buffers.substring(0,5);

   
    //バッテリ
    if (p!=ttgo->power->getBattPercentage()){
      ttgo->tft->fillRect(0, 0, 53, 33, TFT_BLACK);
    }
    set_text(String(p)+"%",2,23,15);


    p = ttgo->power->getBattPercentage();

    
    if (p==50){
      vive(100,100);
    }
    else{
    
    }

    if ((String(mo_i)).length()<2){
      set_text(String(y_i)+"/0"+String(mo_i)+"/"+String(d_i),2,173,15);
    }

    else if((String(d_i)).length()<2){
      set_text(String(y_i)+"/"+String(mo_i)+"/0"+String(d_i),2,173,15);
    }

    else if(((String(mo_i)).length()<2)&&((String(d_i)).length()<2)){
      set_text(String(y_i)+"/"+String(mo_i)+"/0"+String(d_i),2,173,15);
    }

    else{
      set_text(String(y_i)+"/"+String(mo_i)+"/"+String(d_i),2,173,15);
    }
    
    //時計
    set_text(buffers,5,153,55); //int16_t TFT_eSPI::drawString(const char *string, int32_t poX, int32_t poY, uint8_t font)
    //set_text(houres+":"+mints,5,163,55);
    
    save_EEP();
    delay(1000);
}


void setup(){
    //EEPROM
    EEPROM.begin(8); //1kbサイズ
    // Get TTGOClass instance
    ttgo = TTGOClass::getWatch();

    // Initialize the hardware, the BMA423 sensor has been initialized internally
    ttgo->begin();

    // Turn on the backlight
    ttgo->openBL();

    //Receive objects for easy writing
    power = ttgo->power;

    pinMode(AXP202_INT, INPUT_PULLUP);
    attachInterrupt(AXP202_INT, [] {
        irq = true;
    }, FALLING);


    pinMode(RTC_INT_PIN, INPUT_PULLUP);
    attachInterrupt(RTC_INT_PIN, [] {
        rtcIrq = 1;
    }, FALLING);

    pinMode(4,OUTPUT);

    
    

    
    //tftとWi-Fi
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    //ttgo->tft->begin();
  
    
    configTime(9 * 3600L, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp");//NTPの設定

    // Must be enabled first, and then clear the interrupt status,
    // otherwise abnormal
    power->enableIRQ(AXP202_PEK_SHORTPRESS_IRQ,true);

    //  Clear interrupt status
    power->clearIRQ();

    // Wait for the power button to be pressed
    while (!irq) {
        wifiMulti.addAP(ssid1, pw1);
        wifiMulti.addAP(ssid2, pw2);

        
        if ((WiFi.begin(ssid1, pw1) != WL_DISCONNECTED)&&((WiFi.begin(ssid2, pw2) != WL_DISCONNECTED))) {
          //ESP.restart();
          ttgo->tft->fillRect(63, 0, 38, 33, TFT_BLACK);
          set_text("X",2,83,15);
      
        }
    
 
        while ((wifiMulti.run() != WL_CONNECTED )&& (!irq) ) {
          ttgo->tft->fillRect(63, 0, 38, 33, TFT_BLACK);
          set_text("...",2,83,15);
          do_Loop_RTC();


          //Wi-Fiつながったら切る
          /*if((wifiMulti.run() == WL_CONNECTED)&& (!irq)){
            
            
            break;
          }*/
          
        }


        
        while((wifiMulti.run() == WL_CONNECTED)&& (!irq) ){
          ttgo->tft->fillRect(63, 0, 38, 33, TFT_BLACK);
          set_text(".il",2,83,15);
          get_NTP();
          do_Loop_RTC();
        }
        
        
    }
    /*
    After the AXP202 interrupt is triggered, the interrupt status must be cleared,
    * otherwise the next interrupt will not be triggered
    */
    power->clearIRQ();


    // Set screen and touch to sleep mode
    ttgo->displaySleep();

    /*
    When using T - Watch2020V1, you can directly call power->powerOff(),
    if you use the 2019 version of TWatch, choose to turn off
    according to the power you need to turn off
    */
#ifdef LILYGO_WATCH_2020_V1
    ttgo->powerOff();
    // LDO2 is used to power the display, and LDO2 can be turned off if needed
    // power->setPowerOutPut(AXP202_LDO2, false);
#else
    power->setPowerOutPut(AXP202_LDO3, false);
    power->setPowerOutPut(AXP202_LDO4, false);
    power->setPowerOutPut(AXP202_LDO2, false);
    // The following power channels are not used
    power->setPowerOutPut(AXP202_EXTEN, false);
    power->setPowerOutPut(AXP202_DCDC2, false);
#endif

    // Use ext0 for external wakeup
    // esp_sleep_enable_ext0_wakeup((gpio_num_t)AXP202_INT, LOW);

    // Use ext1 for external wakeup
    esp_sleep_enable_ext1_wakeup(GPIO_SEL_35, ESP_EXT1_WAKEUP_ALL_LOW);

    esp_deep_sleep_start();
}

void loop()
{
  
}

#ifndef LOAD_GLCD
//ERROR_Please_enable_LOAD_GLCD_in_User_Setup
#endif

#ifndef LOAD_GFXFF
ERROR_Please_enable_LOAD_GFXFF_in_User_Setup!
#endif
