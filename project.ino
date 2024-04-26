#include <M5Core2.h>
#include <M5UnitENV.h>
#include "SSVQueueStackArray.h"
#include <arduino-timer.h>

struct TemperatureRecord {
  int temperature;
  RTC_TimeTypeDef timestamp;
};
#define MAX_DATA_POINTS 10 
int dataCount = 0; 
int dataCountCo = 0; 
SSVQueueStackArray <int> storage (STACK_Storage,
                                         PTFSA_Resize, 
                                         MAX_DATA_POINTS);   
SSVQueueStackArray <TemperatureRecord> Critical_storage;

SSVQueueStackArray <int> storageCo (STACK_Storage, 
                                         PTFSA_Resize,  
                                         MAX_DATA_POINTS);   
SSVQueueStackArray <TemperatureRecord> Critical_storageCo;


float Temp,templast,Co,Colast = -10;

SHT3X sht3x;
QMP6988 qmp;

ButtonColors off_clrs_PM = {BLACK, WHITE};
ButtonColors on_clrs_PM  = {BLACK, DARKGREY};
ButtonColors off_clrs = {BLACK, WHITE, WHITE};
ButtonColors on_clrs  = {DARKGREY, WHITE, WHITE};
int lastdata;
int lastdataCo;
float min_T = 0.0;
float max_T= 40.0;
int min_Co = 200;
int max_Co= 0;
int Screen_num,Screen_num_curr = 0;
bool isgrafTemp;
bool isgrafCo;
bool isAlarm;
bool isC02Alarm;
bool isTempAlarm;
bool timeout;

Button add_MIN(0, 0, 0, 0, false, ">", off_clrs_PM, on_clrs_PM, MC_DATUM);
Button sub_MIN(0, 0, 0, 0, false, "<", off_clrs_PM, on_clrs_PM, MC_DATUM);
Button add_MAX(0, 0, 0, 0, false, ">", off_clrs_PM, on_clrs_PM, MC_DATUM);
Button sub_MAX(0, 0, 0, 0, false, "<", off_clrs_PM, on_clrs_PM, MC_DATUM);

Button STime(0, 0, 0, 0, false, "Set Time", off_clrs, on_clrs, CC_DATUM);
Button SDate(0, 0, 0, 0, false, "Set Date", off_clrs, on_clrs, CC_DATUM);
Button SCo(0, 0, 0, 0, false,   "Set Co2", off_clrs, on_clrs, CC_DATUM);
Button STemp(0, 0, 0, 0, false, "Set Temp", off_clrs, on_clrs, CC_DATUM);

Button UP_L(0, 0, 0, 0, false, "+", off_clrs_PM, on_clrs_PM, BC_DATUM);
Button UP_M(0, 0, 0, 0, false, "+", off_clrs_PM, on_clrs_PM, BC_DATUM);
Button UP_R(0, 0, 0, 0, false, "+", off_clrs_PM, on_clrs_PM, BC_DATUM);
Button DOWN_L(0, 0, 0, 0, false, "-", off_clrs_PM, on_clrs_PM, BC_DATUM);
Button DOWN_M(0, 0, 0, 0, false, "-", off_clrs_PM, on_clrs_PM, BC_DATUM);
Button DOWN_R(0, 0, 0, 0, false, "-", off_clrs_PM, on_clrs_PM, BC_DATUM);

extern const unsigned char previewR[120264];

int last_value1 = 0, last_value2 = 0;
int cur_value1 = 0, cur_value2 = 0;

RTC_TimeTypeDef RTCtime;
char timeStrbuff[64];

RTC_DateTypeDef RTCDate;
char dateStrbuff[64];

int16_t hw = M5.Lcd.width() / 2;
int16_t hh = M5.Lcd.height() / 2;
auto time_out = timer_create_default();
auto Timer_autoshut = timer_create_default();

void storeTemperatureData(int temperature) {
  M5.Rtc.GetTime(&RTCtime);  
  M5.Rtc.GetDate(&RTCDate);
  File myFile = SD.open("/DataTemp.txt",
                          FILE_WRITE);
  if (dataCount < MAX_DATA_POINTS) {
    dataCount++;
  } else {
    storage.popOldest() ; 
    dataCount--;
  }
  int temps = temperature;
  storage.push(temperature);
  temperature = (temperature >= 0) ? (temperature/2) + 50 : -temperature/2;
  TemperatureRecord TR ={temperature,RTCtime};
  bool isCritical=  false;
  if (temperature >= max_T) {
    Critical_storage.push(TR);
    isCritical = true;
  }
  if (myFile) {  
        myFile.print(temps);
        sprintf(timeStrbuff, "%02d:%02d:%02d",RTCtime.Hours, RTCtime.Minutes,
            RTCtime.Seconds);
            myFile.print(" - ");
        myFile.print(timeStrbuff);
        sprintf(dateStrbuff, " %d/%02d/%02d", RTCDate.Year,
            RTCDate.Month, RTCDate.Date);
        myFile.print(dateStrbuff);
        myFile.print(" - is over threshold: ");
        myFile.println(isCritical);
        myFile.close(); 
    } 
     
    

}
void storeCo2Data(float co) {
  M5.Rtc.GetTime(&RTCtime);  
  M5.Rtc.GetDate(&RTCDate);
  File myFile = SD.open("/DataCo.txt",
                          FILE_WRITE);
  if (dataCountCo < MAX_DATA_POINTS) {
    dataCountCo++;
  } else {
    storageCo.popOldest() ; 
    dataCountCo--;
  }
  int temps = (int)co;
  storageCo.push(temps);
  co = (co >= 0) ? (co/2) + 50 : -co/2;
  TemperatureRecord TR ={co,RTCtime};
  bool isCritical=  false;
  if (temps >= max_Co) {
    Critical_storageCo.push(TR);
    isCritical = true;
  }
  if (myFile) {  
        myFile.print(temps);
        sprintf(timeStrbuff, "%02d:%02d:%02d",RTCtime.Hours, RTCtime.Minutes,
            RTCtime.Seconds);
            myFile.print(" - ");
        myFile.print(timeStrbuff);
        sprintf(dateStrbuff, " %d/%02d/%02d", RTCDate.Year,
            RTCDate.Month, RTCDate.Date);
        myFile.print(dateStrbuff);
        myFile.print(" - is over threshold: ");
        myFile.println(isCritical);
        myFile.close();  
    } 
     
    

}
void drawTemperatureGraph() {
  if(storage.peekNewest() != lastdata)
  {
    lastdata = storage.peekNewest();
    M5.Lcd.fillScreen(BLACK); 
  }
  M5.Lcd.drawLine(0, 120, 319, 120, WHITE); 
  M5.Lcd.drawLine(10, 0, 10, 239, WHITE); 

 
  for (int i = 1; i < dataCount; i++) {
    int x1 = map(i - 1, 0, dataCount - 1, 10, 319);
    long int temp1= storage.getDataByIndex(i-1);
    
    int y1 = map(temp1, -50, 50, 239, 0);
    int x2 = map(i, 0, dataCount - 1, 10, 319);
    long int temp2= storage.getDataByIndex(i);
   
    int y2 = map(temp2, -50, 50, 239, 0); 

    M5.Lcd.drawLine(x1, y1, x2, y2, WHITE); 
   
  }
   M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(1);

  for (int i = -50; i <= 50; i += 10) {
    int y = map(i, -50, 50, 239, 0);
    M5.Lcd.setCursor(0, y);
    M5.Lcd.print(i);
  }
  int thresholdY = map((int)max_T, -50, 50, 239, 0);
  M5.Lcd.drawLine(10, thresholdY, 319, thresholdY, RED);

  for (int i = 0; i < dataCount; i++) {
    long int temp3= storage.getDataByIndex(i);

    if (temp3 >= max_T) {
      int x = map(i, 0, dataCount - 1, 10, 319);
      int y = map(temp3, -50, 50, 239, 0);

      M5.Lcd.fillTriangle(x - 3, y - 3, x + 3, y - 3, x, y + 3, RED);
    }
  }
}
void drawCoGraph() {
  if(storageCo.peekNewest() != lastdataCo)
  {
    lastdataCo = storageCo.peekNewest();
    M5.Lcd.fillScreen(BLACK); 
  }
  M5.Lcd.drawLine(0, 230, 319, 230, WHITE);
  M5.Lcd.drawLine(10, 0, 10, 239, WHITE);

 
  for (int i = 1; i < dataCountCo; i++) {
    int x1 = map(i - 1, 0, dataCountCo - 1, 10, 319); 
    long int temp1= storageCo.getDataByIndex(i-1);
   
    int y1 = map(temp1, -100, 2100, 239, 0); 
    int x2 = map(i, 0, dataCountCo - 1, 10, 319); 
    long int temp2= storageCo.getDataByIndex(i);
  
    int y2 = map(temp2, -100, 2100, 239, 0); 

    M5.Lcd.drawLine(x1, y1, x2, y2, WHITE); 
   
  }
   M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(1);

  for (int i = 0; i <= 2000; i += 200) {
    int y = map(i, -100, 2100, 239, 0);
    M5.Lcd.setCursor(0, y-2);
    M5.Lcd.print(i/50);
  }
  int thresholdY = map((int)max_Co, -100, 2100, 239, 0);
  M5.Lcd.drawLine(10, thresholdY, 319, thresholdY, RED);

  for (int i = 0; i < dataCountCo; i++) {
    long int temp3= storageCo.getDataByIndex(i);
  
    if (temp3 >= max_Co) {
      int x = map(i, 0, dataCountCo - 1, 10, 319);
      int y = map(temp3, -100, 2100, 239, 0);
     
      M5.Lcd.fillTriangle(x - 3, y - 3, x + 3, y - 3, x, y + 3, RED);
    }
  }
}
void task1(void* pvParameters) {                                
  while (1) {  
      Temp =random(-10, 40);
      storeTemperatureData(Temp);
      delay(
          5000);  
  }
}
void task2(void* pvParameters) {

    cur_value1 = digitalRead(36); 
    cur_value2 = digitalRead(26);
    while (1) {
      
        if (cur_value1 != last_value1) {
        Co +=100;
        last_value1 = cur_value1;
    }
    if (cur_value2 != last_value2) {
        Co -=100;
        last_value2 = cur_value2;
    }
    int num =random(0, 2);
    if(num == 0){
      Co +=100;
    }else{
      Co -=100;
    }
    storeCo2Data(Co);
        delay(5000);
    }
    
       
}
bool timerCallback(void *argument) {
    M5.Axp.SetVibration(false); 
    Screen_num= 0;
          isAlarm = false;
    M5.Lcd.clear();
    return false;
          
}
bool timerout(void *argument) {
    timeout = false;
    return false;
}
void task3(void* pvParameters) {
  while (1) {
    float sum = Temp; 
    int count = 5; 
    for (int i = storage.getCount(); i >= storage.getCount() - 5 && i >= 0; i--) {
        sum += storage.getDataByIndex(i); 
    }
    float avg = sum / count; 
    isC02Alarm =(Co > (float)max_Co)? true : false;
    isTempAlarm = (Temp >= max_T) ? true : false;
    if(avg > max_T && Co > (float)max_Co && !isAlarm && !timeout){
      M5.Axp.SetVibration(true); 
      Screen_num= 404;
      isAlarm = true;
      time_out.in(90000,timerout);
      Timer_autoshut.in(30000,timerCallback);
      timeout = true;  
    }
    delay(500);
  }
}
void SCR_Alarm() {
    Screen_num_curr = 404;
    M5.Lcd.fillRect( 0, 0 ,  M5.Lcd.width(), M5.Lcd.height(), RED);
    M5.Lcd.setTextSize(3);
    M5.Lcd.setCursor(hw-100 , hh);
    M5.Lcd.setTextColor(BLACK); 
    M5.Lcd.print("ALARM");
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(hw+90 , hh+100);
    M5.Lcd.setTextColor(BLACK); 
    M5.Lcd.print("Cancel");
    M5.Lcd.fillRect( 0, 0 ,  M5.Lcd.width(), M5.Lcd.height(), WHITE);
    M5.Lcd.setTextSize(3);
    M5.Lcd.setCursor(hw-100 , hh);
    M5.Lcd.setTextColor(BLACK); 
    M5.Lcd.print("ALARM");
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(hw+90 , hh+100);
    M5.Lcd.setTextColor(BLACK); 
    M5.Lcd.print("Cancel");
}
void SCR_home() {
  
    Screen_num_curr = 0;
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(hw-5 , 230);
    M5.Lcd.print("...");
    M5.Lcd.setTextColor(WHITE,BLACK); 
    if(isgrafTemp){
      M5.Lcd.setTextSize(2);
       drawTemperatureGraph();
      
    }else if(isgrafCo){
      M5.Lcd.setTextSize(2);
       drawCoGraph();
    }else
    {
      if(isTempAlarm)
      {
        M5.Lcd.setCursor(20 , 80);
        M5.Lcd.setTextColor(RED,BLACK);
        M5.Lcd.setTextSize(3);
        M5.Lcd.print("!"); 
      }
      if (Temp <= 0.0) {
        if(Temp < -9){
          M5.Lcd.setCursor(40 , 80);
        }else{
          M5.Lcd.setCursor(60 , 80);
        }
        M5.Lcd.setTextColor(CYAN,BLACK);
      }
      else if (Temp > 0.0 && Temp <= 20.0) {
        if(Temp < 10){
          M5.Lcd.setCursor(80 , 80);
        }else{
          M5.Lcd.setCursor(60 , 80);
        }
        M5.Lcd.setTextColor(GREEN,BLACK);
      }
      else if (Temp > 20.0 && Temp <= 30.0) {
        M5.Lcd.setCursor(60 , 80);
        M5.Lcd.setTextColor(YELLOW,BLACK);
      }
      else {
        M5.Lcd.setCursor(60 , 80);
        M5.Lcd.setTextColor(RED,BLACK);
      }
      if(templast != Temp ){
          M5.Lcd.fillRect(0, 30, M5.Lcd.width(), 60, BLACK);
          templast = Temp;
      }
      if(Colast != Co ){
          M5.Lcd.fillRect(0, 120, M5.Lcd.width(), 60, BLACK);
          Colast = Co;
      }
     
      M5.Lcd.setTextSize(4);
      M5.Lcd.printf("%.1f",Temp);
      M5.Lcd.setTextSize(4);
      M5.Lcd.print("*C");
      M5.Lcd.setTextColor(WHITE,BLACK);
      
      M5.Lcd.setTextSize(3);
      if(isC02Alarm)
      {
        M5.Lcd.setCursor(20 , 160);
        M5.Lcd.setTextColor(RED,BLACK);
        M5.Lcd.print("!"); 
        M5.Lcd.setTextColor(WHITE,BLACK);
      }
      M5.Lcd.setCursor(70 , 160);
      M5.Lcd.printf(" %.1f",(Co/50));
      M5.Lcd.print("%"); 
    }
  
   
}
void SCR_SCO2() {
    Screen_num_curr = 4;
    M5.Lcd.setCursor(125 , 40);
    M5.Lcd.setTextSize(2);
    M5.Lcd.print("Co2");
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(150 , 70);
    M5.Lcd.print("max");
    M5.Lcd.setCursor(150 , 140);
    M5.Lcd.print("min");
    M5.Lcd.setCursor(155 , 230);
    M5.Lcd.print("...");
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE,BLACK);
    
    if(min_Co > 990){
      M5.Lcd.setCursor(hw -40 , 170);
    }else{
      M5.Lcd.setCursor(hw - 30 , 170);
    }
    M5.Lcd.printf("%d", min_Co);
    M5.Lcd.setCursor(hw - 40 , 100);
    
    M5.Lcd.printf("%d", max_Co);
   
  
}
void SCR_STemp() {
    Screen_num_curr = 5;
    M5.Lcd.setCursor(115 , 40);
    M5.Lcd.setTextSize(2);
    M5.Lcd.print("Temp");
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(150 , 70);
    M5.Lcd.print("max");
    M5.Lcd.setCursor(150 , 140);
    M5.Lcd.print("min");
    M5.Lcd.setCursor(155 , 230);
    M5.Lcd.print("...");
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE,BLACK);
    M5.Lcd.setCursor(hw - 30 , 100);
    M5.Lcd.printf("%.1f", max_T);
    if(min_T >=10.0){
      M5.Lcd.setCursor(hw - 30 , 170);
    }else{
      M5.Lcd.setCursor(hw - 20 , 170);
    }
    M5.Lcd.printf("%.1f", min_T);
   
   
  
}
void SCR_Settings() {
  Screen_num_curr = 1;
  M5.Lcd.setCursor(95 , 60);
  M5.Lcd.setTextSize(2);
  M5.Lcd.print("Settings");
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(hw-10 , 230);
  M5.Lcd.print("[O]");
  M5.Lcd.setTextColor(WHITE,BLACK);  
}
void SCR_STime() {
    Screen_num_curr = 2;
    M5.Lcd.setCursor(115 , 40);
    M5.Lcd.setTextSize(2);
    M5.Lcd.print("Time");
    sprintf(timeStrbuff, "%02d : %02d : %02d",RTCtime.Hours, RTCtime.Minutes,
            RTCtime.Seconds);
    M5.Lcd.setCursor(hw-90 , 130);
    M5.Lcd.setTextSize(2);
    M5.Lcd.print(timeStrbuff);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(hw-5 , 230);
    M5.Lcd.print("...");
    M5.Lcd.setTextColor(WHITE,BLACK);
}
void SCR_SDate() {
    Screen_num_curr = 3;
    M5.Lcd.setCursor(115 , 40);
    M5.Lcd.setTextSize(2);
    M5.Lcd.print("Date");
    sprintf(dateStrbuff, "%02d / %02d / %d", RTCDate.Date, RTCDate.Month, RTCDate.Year);
    M5.Lcd.setCursor(hw-90 , 130);
    M5.Lcd.setTextSize(2);
    M5.Lcd.print(dateStrbuff);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(hw-5 , 230);
    M5.Lcd.print("...");
    M5.Lcd.setTextColor(WHITE,BLACK);
}
void doButtonsSettings() {
    STime.set(30, 90, hw - 40, hh - 70);
    SDate.set(M5.Lcd.width()-150, 90, hw - 40, hh - 70);
    SCo.set(30, 150, hw - 40, hh - 70);
    STemp.set(M5.Lcd.width()-150, 150, hw - 40, hh - 70);
    Sett_Btn_enable();
}
void doButtonsTC() {
    add_MIN.setTextSize(3);
    sub_MIN.setTextSize(3);
    add_MIN.set(M5.Lcd.width()-100, 130, 50,50);
    sub_MIN.set(50, 130, 50,50);
    add_MAX.setTextSize(3);
    sub_MAX.setTextSize(3);
    add_MAX.set(M5.Lcd.width()-100, 60, 50,50);
    sub_MAX.set(50, 60, 50,50);
    Temp_CO_Btn_enable();
}
void doButtonsDT() {
    UP_L.setTextSize(3);
    UP_M.setTextSize(3);
    UP_R.setTextSize(3);
    UP_L.set(hw-100, 50, 50,50);
    UP_M.set(hw-30, 50, 50,50);
    UP_R.set(hw+40, 50, 50,50);
    DOWN_L.setTextSize(3);
    DOWN_M.setTextSize(3);
    DOWN_R.setTextSize(3);
    DOWN_L.set(hw-98, 150, 50,50);
    DOWN_M.set(hw-28, 150, 50,50);
    DOWN_R.set(hw+42, 150, 50,50);
    Date_Time_Btn_enable();

}
void Temp_CO_Btn_enable(){
  add_MIN.draw();
  sub_MIN.draw();
  add_MAX.draw();
  sub_MAX.draw();
}
void Temp_CO_Btn_disenable(){
  add_MIN.hide();
  sub_MIN.hide();
  add_MAX.hide();
  sub_MAX.hide();
}
void Date_Time_Btn_enable(){
    UP_L.draw();
    UP_M.draw();
    UP_R.draw();
  DOWN_L.draw();
  DOWN_M.draw();
  DOWN_R.draw();
}
void Date_Time_Btn_disenable(){
    UP_L.hide();
    UP_M.hide();
    UP_R.hide();
  DOWN_L.hide();
  DOWN_M.hide();
  DOWN_R.hide();
}
void Sett_Btn_enable(){
    STime.draw();
    SDate.draw();
    SCo.draw();
    STemp.draw();
}
void Sett_Btn_disenable(){
    STime.hide();
    SDate.hide();
    SCo.hide();
    STemp.hide();
}
void eventDisplay(Event& e) {
  String Str = (String)e.objName();

  if(e.typeName()=="E_TAP"||e.typeName()=="E_PRESSING"){
 
    if(Str=="Set Time"){

      M5.Lcd.clear();
      Screen_num = 2; 
      Date_Time_Btn_enable();
      M5.Rtc.GetTime(&RTCtime);
    
    }
    else if(Str=="Set Date"){
     
      M5.Lcd.clear();
      Screen_num = 3; 
      Date_Time_Btn_enable();
      M5.Rtc.GetDate(&RTCDate);
   
    }
    else if(Str=="Set Co2"){
     
      M5.Lcd.clear();
      Temp_CO_Btn_enable();
      Screen_num = 4; 
     
    }
    else if(Str=="Set Temp"){
     
      M5.Lcd.clear();
      Temp_CO_Btn_enable();
      Screen_num = 5; 
   
    }
    else if(Str==">"){
      if(Screen_num_curr == 5){ 
        if(e.to.y < 110 && max_T < 60){
          max_T+=0.5;
          M5.Lcd.fillRect(hw-65, 75, 130, 30, BLACK);
        }
        if(e.to.y > 110 && min_T < 20){
          min_T+=0.5;
          M5.Lcd.fillRect(hw-65, 145, 130, 30, BLACK);
        }
      }
      if(Screen_num_curr == 4){ 
        if(e.to.y < 110 && max_Co < 2000){
          max_Co+=10;
          M5.Lcd.fillRect(hw-65, 75, 130, 30, BLACK);
        }
        if(e.to.y > 110 && min_Co < 1000){
          min_Co+=10;
          M5.Lcd.fillRect(hw-65, 145, 130, 30, BLACK);
        }
      }
    }
    else if(Str=="<"){
      if(Screen_num_curr == 5){
        if(e.to.y < 110 && max_T <= 60 && max_T > 20){
          max_T-=0.5;
          M5.Lcd.fillRect(hw-65, 75, 130, 30, BLACK);
        }
        if(e.to.y > 110 && min_T <= 20 && min_T > -10){
          min_T-=0.5;
          M5.Lcd.fillRect(hw-65, 145, 130, 30, BLACK);
        }
      }
      if(Screen_num_curr == 4){ 
        if(e.to.y < 110 && max_Co <= 2000 && max_Co > 1000 ){
          max_Co-=10;
          M5.Lcd.fillRect(hw-65, 75, 130, 30, BLACK);
        }
        if(e.to.y > 110 && min_Co <= 1000 && min_Co >200){
          min_Co-=10;
          M5.Lcd.fillRect(hw-65, 145, 130, 30, BLACK);
        }
      }
    }
    else if(Str=="+"){
      if(Screen_num_curr == 2){ 
        if(e.to.x < 115){
          if(RTCtime.Hours<23){
            RTCtime.Hours += 1; 
          }else{
            RTCtime.Hours = 0;
          }
        }
        if(e.to.x > 125 && e.to.x < 185){
          if(RTCtime.Minutes<59){
            RTCtime.Minutes += 1; 
          }else{
            RTCtime.Minutes = 0;
          }
          
        }
        if(e.to.x > 190){
          if(RTCtime.Seconds<59){
            RTCtime.Seconds += 1; 
          }else{
            RTCtime.Seconds = 0;
          }
        }
        M5.Lcd.fillRect(65, 105, 190, 30, BLACK);
        M5.Rtc.SetTime(&RTCtime);
      }
      if(Screen_num_curr == 3){ 
        if(e.to.x < 115){
          if(RTCDate.Date<31){
            RTCDate.Date += 1; 
          }else{
            RTCDate.Date = 1;
          }
        }
        if(e.to.x > 125 && e.to.x < 185){
          if(RTCDate.Month<12){
            RTCDate.Month += 1; 
          }else{
            RTCDate.Month = 1;
          }
          
        }
        if(e.to.x > 190){
          RTCDate.Year+= 1; 
        }
        
        M5.Lcd.fillRect(65, 105, 240, 30, BLACK);
        M5.Rtc.SetDate(&RTCDate);
      }    
    }
    else if(Str=="-"){
      if(Screen_num_curr == 2){
        if(e.to.x < 115){
          if(RTCtime.Hours>0){
            RTCtime.Hours -= 1; 
          }else{
            RTCtime.Hours = 23;
          }
        }
        if(e.to.x > 125 && e.to.x < 185){
          if(RTCtime.Minutes>0){
            RTCtime.Minutes -= 1; 
          }else{
            RTCtime.Minutes = 59;
          }
          
        }
        if(e.to.x > 190){
          if(RTCtime.Seconds>0){
            RTCtime.Seconds -= 1; 
          }else{
            RTCtime.Seconds = 59;
          }
        }
        M5.Lcd.fillRect(65, 105, 190, 30, BLACK);
        M5.Rtc.SetTime(&RTCtime);
      }
      if(Screen_num_curr == 3){ 
        if(e.to.x < 115){
          if(RTCDate.Date>1){
            RTCDate.Date -= 1; 
          }else{
            RTCDate.Date = 31;
          }
          
        }
        if(e.to.x > 125 && e.to.x < 185){
          if(RTCDate.Month>1){
            RTCDate.Month -= 1; 
          }else{
            RTCDate.Month = 12;
          }
          
        }
        if(e.to.x > 190){
          RTCDate.Year -= 1; 
        }
        
        M5.Lcd.fillRect(65, 105, 240, 30, BLACK);
        M5.Rtc.SetDate(&RTCDate);
      }
    }
    else{}
  }
}
void setup() {
    M5.begin(true, true, true, true, mbus_mode_t::kMBusModeOutput,
             true);  
    Serial.begin(115200);
    
    pinMode(36, INPUT);  
    pinMode(26, INPUT);
     if (!SD.begin()) {  
        M5.Lcd.println(
            "Card failed, or not present");  
        while (1);
    }
    
    //if (!sht3x.begin(&Wire, SHT3X_I2C_ADDR, 21, 22, 400000U)) {
    //    Serial.println("Couldn't find SHT3X");
    //    while (1) delay(1);
    //}
    lastdata= 0;
    isgrafTemp = false;
    isgrafCo = false;
    timeout = false;
    isAlarm = false;
    isC02Alarm = false;
    isTempAlarm = false;
    hw = M5.Lcd.width() / 2;
    hh = M5.Lcd.height() / 2;
    RTCDate.Year  = 2022; 
    RTCDate.Month = 1;
    RTCDate.Date  = 9;
    templast = 0;
    min_T = 0.0;
    max_T= 40.0;
    min_Co = 200;
    max_Co= 1200;
    Co= 500;
    M5.Buttons.addHandler(eventDisplay, E_ALL - E_MOVE);
    doButtonsSettings();
    doButtonsTC();
    doButtonsDT();
    Temp_CO_Btn_disenable();
    Date_Time_Btn_disenable();
    Sett_Btn_disenable();
    M5.Lcd.clear();
    xTaskCreatePinnedToCore(
        task1,    
                  
        "task1",  
        4096,     
                  
        NULL,    
                  
        1,        
        NULL,     
        0);
    xTaskCreatePinnedToCore(task2, "task2", 4096, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(task3, "task3", 4096, NULL, 3, NULL, 0);
    SCR_home();
  
}
void loop() {
  M5.update(); 

  time_out.tick();
  Timer_autoshut.tick();
  switch (Screen_num) {
  case 0:
    SCR_home();
    break;
  case 1:
    
    SCR_Settings();
    break;
  case 2:
    Sett_Btn_disenable();
    SCR_STime();
    break;
  case 3:
    Sett_Btn_disenable();
    SCR_SDate();
    break;
  case 4:
    Sett_Btn_disenable();
    
    SCR_SCO2();
    break;
  case 5:
    Sett_Btn_disenable();
    SCR_STemp();
    break;
  case 404:
    SCR_Alarm();
    break;
  default:
    break;
  }
  
  if(Screen_num_curr == 1 && !isAlarm){
      if (M5.BtnB.wasReleased() || M5.BtnB.pressedFor(1000, 200)) {
        M5.Lcd.clear();
        Screen_num = 0; 
      }
  }else{
    if ((M5.BtnB.wasReleased() || M5.BtnB.pressedFor(1000, 200))&&!isAlarm) {
        M5.Lcd.clear();
        Temp_CO_Btn_disenable();
        Date_Time_Btn_disenable();
        Screen_num= 1; 
        Sett_Btn_enable();
      }
  }
  if(Screen_num_curr == 0 && !isgrafTemp && !isgrafCo && !isAlarm){
    if (M5.BtnA.wasReleased() || M5.BtnA.pressedFor(1000, 200)) {
      M5.Lcd.clear();
      isgrafTemp = true; 
    }
  }else if(Screen_num_curr == 0 && isgrafTemp && !isgrafCo && !isAlarm){
    if (M5.BtnA.wasReleased() || M5.BtnA.pressedFor(1000, 200)) {
      M5.Lcd.clear();
      isgrafTemp = false;
      isgrafCo = true; 
    }
  }
  else{
    if ((M5.BtnA.wasReleased() || M5.BtnA.pressedFor(1000, 200))&& Screen_num_curr == 0 && !isAlarm ) {
      M5.Lcd.clear();
      isgrafCo = false; 
      
  
    }
  }
  if( isAlarm){
      if (M5.BtnC.wasReleased() || M5.BtnC.pressedFor(1000, 200)) {
        M5.Lcd.clear();
        M5.Axp.SetVibration(false); 
        Timer_autoshut.cancel();
        Screen_num = 0; 
        isAlarm = false; 
      }
  }
  
  
}























