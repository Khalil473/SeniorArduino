#include <SoftwareSerial.h>
#include <SD.h>
#include <virtuabotixRTC.h>
#include "DHT.h"

#define DHTPIN 4
#define DHTRESPONCE 2000
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
  virtuabotixRTC myRTC = virtuabotixRTC(RTCCLK, RTCDAT, RTCRST);
  SoftwareSerial BTserial=SoftwareSerial(HM10TX, HM10RX);
  Modules() {
    dht = new DHT(DHTPIN, DHTTYPE);
    dht->begin();
    myRTC.updateTime();
    isSDinitialized = SD.begin(SDCSPIN);
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

  int retrieveData(char &type, char &period) {
    String path = String(type);
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
    file.close();
    return (count == 0) ? 0 : sum / (count * 1.0);
  }


  void retrieveDataDays(char &type, int &year, uint8_t &month, uint8_t from_day = 0, uint8_t to_day = 0) {

    File dir = SD.open(String(type) + '/' + String(year) + '/' + String(month) + '/');
    to_day = (to_day == 0) ? days_in_month(year, month) : to_day;
    while (true) {
      File file = dir.openNextFile();
      if (!file) {
        break;
      }
      String name = String(file.name());
      uint8_t file_day = name.toInt();
      if (file_day < from_day || file_day > to_day) continue;
      float avg = getDayAvg(file);
    }
    dir.close();
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
        return true;
      }
    }
    return false;
  }
  bool sendToBLE(char type,float data){
    BTserial.println(String(type)+String(data));
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
  if (modules->readDHT(dht)) {
    if (DHTData::isTimeToSave()) {
      modules->writeToSD(HUMIDITY, dht.humidity);
      modules->writeToSD(TEMPREATURE, dht.tempreature);
      DHTData::lastSaved = millis();
    }
    modules->sendToBLE(TEMPREATURE,dht.tempreature);
    delay(1000);
    modules->sendToBLE(HUMIDITY,dht.humidity);
    
  }
}
