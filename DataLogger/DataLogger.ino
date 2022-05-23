#include <SoftwareSerial.h>
#include <SD.h>
#include <virtuabotixRTC.h>
#include "DHT.h"

#define DHTPIN 4
#define DHTRESPONCE 6000
#define DHTTYPE DHT22

#define SDCSPIN 10

#define RTCDAT 7
#define RTCCLK 5
#define RTCRST 9

#define HUMIDITY 'h'
#define TEMPREATURE 't'

#define HM10TX 2
#define HM10RX 3

#define TIME_BETWEEN_SAVES 6000  // 1 MIN

#define ACK_TIMEOUT_TIME 5000

struct DHTData {
  float tempreature;
  float humidity;
  static unsigned long lastRead, lastSaved;
  DHTData() {
  }
  DHTData(float temp, float hum) {
    this->tempreature = temp;
    this->humidity = hum;
  }
  static bool isTimeToSave() {
    return millis() - lastSaved > TIME_BETWEEN_SAVES;
  }
  static bool isTimeToRead() {
    return millis() - lastRead > DHTRESPONCE;
  }
};
unsigned long DHTData::lastRead = millis();
unsigned long DHTData::lastSaved = millis();

struct Modules {
  DHT *dht;
  bool isSDinitialized;
  bool isBLEConnected;
  virtuabotixRTC myRTC = virtuabotixRTC(RTCCLK, RTCDAT, RTCRST);
  SoftwareSerial BTserial=SoftwareSerial(HM10TX, HM10RX);
  Modules() {
    dht = new DHT(DHTPIN, DHTTYPE);
    dht->begin();
    myRTC.updateTime();
    isSDinitialized = SD.begin(SDCSPIN);
    isBLEConnected=0;
    BTserial.begin(9600);
  }

  bool readDHT(DHTData &currentData) {
    if (DHTData::isTimeToRead()) {
      DHTData::lastRead = millis();
      float humd = dht->readHumidity();
      float temp = dht->readTemperature();
      if (isnan(temp) || isnan(humd))
        return false;
      currentData.humidity = humd;
      currentData.tempreature = temp;
      return true;
    }
    return false;
  }
  File prepareToWrite(char &type) {
    myRTC.updateTime();

    String path = String(type) + '/' + String(myRTC.year) + '/' + String(myRTC.month) + '/';
    if (!SD.exists(path)) {
      SD.mkdir(path);
    }
    File f = SD.open(path + String(myRTC.dayofmonth), FILE_WRITE);
    f.print(String(myRTC.hours) + ':' + String(myRTC.minutes) + ':' + String(myRTC.seconds) + ' ');
    return f;
  }
  bool writeToSD(char type, float data) {
    File file = prepareToWrite(type);
    if (!file)
      return false;
    file.print(data);
    file.println();
    file.close();
    return true;
  }

  uint8_t days_in_month(int &y, uint8_t &m) {
    if ((m == 4) || (m == 11) || (m == 9) || (m == 6)) return 30;
    if (m != 2) return 31;
    if ((y % 400) == 0) return 29;
    if ((y % 100) == 0) return 28;
    if ((y % 4) == 0) return 29;
    return 28;
  }
  float getDayAvg(File &file) {
    float sum = 0.0;
    int count = 0;
    while (file.available()) {
      String line = file.readStringUntil('\n');
      String data = line.substring(line.indexOf(' '));
      sum += data.toFloat();
      count++;
    }
    return (count == 0) ? 0 : sum / (count * 1.0);
  }


  void retrieveDataDays(char type, int year, uint8_t month, uint8_t from_day = 0, uint8_t to_day = 0) {
    File dir = SD.open(String(type) + '/' + String(year) + '/' + String(month) + '/');
    to_day = (to_day == 0) ? days_in_month(year, month) : to_day;
    while (true) {
      File file = dir.openNextFile();
      if(!file) {
        break;
      }
      String name = String(file.name());
      uint8_t file_day = name.toInt();
      if (file_day < from_day || file_day > to_day) {
        file.close();
        continue;
        }
      float avg = getDayAvg(file);
      file.close();
      sendToBLE('\0',avg,false);//continue in mobile to check this code
    }
    dir.close();
    sendToBLE('h',-1);
  }

  void retrieveDataMonths(char &type, int &year) {
    String path=String(type) + '/' + String(year) + '/';
    File year_dir = SD.open(path);
    bool save_avg=false;
    while (true) {
      File month_dir = year_dir.openNextFile();
      String avg_file_path=path+String(month_dir.name())+"/_avg";
      if (!month_dir) {
        break;
      }
      if(SD.exists(avg_file_path)){
        File f=SD.open(avg_file_path);
        Serial.println("avg file found!" + avg_file_path);
        String line = f.readStringUntil('\n');
        sendToBLE('\0',line.toFloat(),false);
        month_dir.close();
        f.close();
        continue;
      }
      save_avg=!String(month_dir.name()).equals(String(myRTC.month));
      float month_avg_sum=0.0;
      uint8_t num_of_days=0;
      while(true){
        File file = month_dir.openNextFile();
        if (!file) {
          break;
        }
        month_avg_sum += getDayAvg(file);
        num_of_days++;
        file.close();
      }
      float month_avg=(num_of_days<=0) ? 0 : month_avg_sum/ (num_of_days*1.0);
      if(save_avg){
        File f=SD.open(avg_file_path,FILE_WRITE);
        f.println(String(month_avg));
        f.close();
      }
      sendToBLE('\0',month_avg,false);//continue in mobile to check this code
      month_dir.close();
    }
    year_dir.close();
    sendToBLE('h',-1);
    delay(200);
  }
  void retrieveDataYears(char type){
    String base_path=String(type) + '/';
    File type_dir = SD.open(base_path);
    bool save_year_avg=false;
    while (true) {
    File year_dir = type_dir.openNextFile();
    if (!year_dir) {
        break;
    }
    String year_avg_path=base_path+String(year_dir.name())+"/_avg";
    String path=base_path+String(year_dir.name())+"/";
    if(SD.exists(year_avg_path)){
      File f=SD.open(year_avg_path);
        Serial.println("avg year file found!" + year_avg_path);
        String line = f.readStringUntil('\n');
        sendToBLE('\0',line.toFloat(),false);
        year_dir.close();
        f.close();
        continue;
    }
    save_year_avg=!String(year_dir.name()).equals(String(myRTC.year));
    float month_avgs=0.0;
    uint8_t num_of_months=0;
    bool save_month_avg=false;
    while (true) {
      File month_dir = year_dir.openNextFile();
      if (!month_dir) {
        break;
    }
    String month_avg_path=path+String(month_dir.name())+"/_avg";
    if(SD.exists(month_avg_path)){
        File f=SD.open(month_avg_path);
        Serial.println("avg month file found!" + month_avg_path);
        String line = f.readStringUntil('\n');

        month_dir.close();
        f.close();
        continue;
      }
      save_month_avg=!String(month_dir.name()).equals(String(myRTC.month));
      float month_avg_sum=0.0;
      uint8_t num_of_days=0;
      if(SD.exists(month_avg_path)){
        File f=SD.open(month_avg_path);
        Serial.println("avg month file found!" + month_avg_path);
        String line = f.readStringUntil('\n');
        month_avg_sum=line.toFloat();
        num_of_days=1;
        month_dir.close();
        f.close();
      }
      else{
      while(true){
        File file = month_dir.openNextFile();
        if (!file) {
          break;
        }
        month_avg_sum += getDayAvg(file);
        num_of_days++;
        file.close();
      }
      }
      month_dir.close();
      float month_avg=(num_of_days<=0) ? 0 : month_avg_sum/ (num_of_days*1.0);
      if(save_month_avg){
        File f=SD.open(month_avg_path,FILE_WRITE);
        f.println(String(month_avg));
        f.close();
      }
      month_avgs+=month_avg;
      num_of_months++;
      Serial.println(String(month_avgs)+" at "+String(month_dir.name()));
    }
    float year_avg=(num_of_months<=0) ? 0 : month_avgs / (num_of_months*1.0);
    Serial.println(String(year_avg)+" at "+String(year_dir.name()));
    if(save_year_avg){
      File f=SD.open(year_avg_path,FILE_WRITE);
      f.println(String(year_avg));
      f.close();
    }
    sendToBLE('\0',year_avg,false);
    year_dir.close();
    }
    type_dir.close();
    sendToBLE('h',-1);
    delay(200);
  }
  String readFromBLE(){
    if(BTserial.available())
      return BTserial.readString();
    return "";
  }
  bool isAckRecived(){
    if(BTserial.available()){
      String data = BTserial.readString();
      if (data=="1"){
        Serial.println("ACK Rec");
        return true;
      }
      else if(data.startsWith("disconnect")){
        isBLEConnected=0;
        return true;
      }
      else if(data.startsWith("connected")){
        isBLEConnected=true;
        return true;
      }
    }
    return false;
  }
  bool sendToBLE(char type,float data,bool flush=true){
    static String buffer="";
    flush = flush || buffer.length() + String(data).length()>14;
    String newData= ( (type=='\0') ? "":String(type))+String(data)+ ( (flush) ? "":",");
    buffer+=newData;
    if(!flush) return 1;
    Serial.println(buffer);
    BTserial.print(buffer);
    buffer="";
    long startWait=millis();
    while(!isAckRecived())
    {
      if(millis()-startWait>ACK_TIMEOUT_TIME){// time out waiting for ack
        isBLEConnected=0;
        Serial.println("time out");
        return 0;
      }
    }
    return 1;
  }
  void checkBLECommands(){
    String dataOnBLE=readFromBLE();
    if(dataOnBLE.length()<2) return;
    Serial.println(dataOnBLE);
    if(dataOnBLE.startsWith("connected")){
      isBLEConnected=1;
      return; 
    }
    if(dataOnBLE.startsWith("disconnect")){
      isBLEConnected=0;
      return; 
    }
    if(dataOnBLE[0]=='h'){
      if (dataOnBLE[1]=='d') {
        this->myRTC.updateTime();
        uint8_t from_day,to_day,current_day=myRTC.dayofmonth,current_month=myRTC.month;
        int current_year=myRTC.year;
        if(current_day<12){
          retrieveDataDays(dataOnBLE[2],current_year,current_month,0,current_day);
          current_month = (current_month==1) ? 12 : current_month-1;
          if(current_month==1){
            current_month=12;
            current_year-=1;
          }else{
            current_month-=1;
          }
          uint8_t days_in_current_month=days_in_month(current_year,current_month);
          retrieveDataDays(dataOnBLE[2],current_year,current_month,days_in_current_month-12,days_in_current_month);
        }
        else
          retrieveDataDays(dataOnBLE[2],current_year,current_month,current_day-12,current_day);
      }
      if(dataOnBLE[1]=='m'){
        int current_year=myRTC.year;
        retrieveDataMonths(dataOnBLE[2], current_year);
      }
      if(dataOnBLE[1]=='y'){
        retrieveDataYears(dataOnBLE[2]);
      }
    }
  }
};
Modules *modules;
void setup() {
  Serial.begin(9600);
  modules = new Modules();
}

void loop() {
  DHTData dht;
  String data;
  if (modules->readDHT(dht)) { // TODO: if the data is equal to the last read data dont enter
    if (DHTData::isTimeToSave()) {
      modules->writeToSD(HUMIDITY, dht.humidity);
      modules->writeToSD(TEMPREATURE, dht.tempreature);
      DHTData::lastSaved = millis();
    }
    if(modules->isBLEConnected){
      modules->sendToBLE(TEMPREATURE,dht.tempreature);
      modules->sendToBLE(HUMIDITY,dht.humidity);
    }
    
  }
  modules->checkBLECommands();
}
