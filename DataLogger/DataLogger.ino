#include <SPI.h>
#include <SD.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include "DHT.h"

#define DHTPIN 4
#define DHTRESPONCE 2000
#define DHTTYPE DHT22

#define SDCSPIN 10

#define RTCDAT 7
#define RTCCLK 5
#define RTCRST 9

struct DHTData
{
  float tempreature;
  float humidity;
  static long lastRead, lastSaved;
  DHTData()
  {
  }
  DHTData(float temp, float hum)
  {
    this->tempreature = temp;
    this->humidity = hum;
  }
};
long DHTData::lastRead;
long DHTData::lastSaved;

struct Modules
{
  DHT *dht;
  bool isSDinitialized;
  RtcDS1302<ThreeWire> *rtc;

  Modules()
  {
    dht = new DHT(DHTPIN, DHTTYPE);
    ThreeWire myWire(RTCDAT, RTCCLK, RTCRST);
    rtc = new RtcDS1302<ThreeWire>(myWire);
    dht->begin();
    rtc->Begin();
    isSDinitialized = SD.begin(SDCSPIN);
    RtcDateTime compiled(__DATE__, __TIME__);
    if (!rtc->IsDateTimeValid())
      rtc->SetDateTime(compiled);
    if (rtc->GetIsWriteProtected())
      rtc->SetIsWriteProtected(false);
    if (!rtc->GetIsRunning())
      rtc->SetIsRunning(true);
    if (rtc->GetDateTime() < compiled)
      rtc->SetDateTime(compiled);
  }

  bool readDHT(DHTData &currentData)
  {
    if (millis() - DHTData::lastRead > DHTRESPONCE)
    {
      DHTData::lastRead = millis();
      float humd = dht->readHumidity();
      float temp = dht->readTemperature();
      if (isnan(temp) || isnan(humd))
        return false;
      else
      {
        currentData.humidity = humd;
        currentData.tempreature = temp;
        return true;
      }
    }
    return false;
  }
  File prepareToWrite(char &type)
  {
    RtcDateTime current = rtc->GetDateTime();
    String path = String(type) + '/' + String(current.Year()) + '/' + String(current.Month()) + '/';
    if (!SD.exists(path))
    {
      SD.mkdir(path);
    }
    File f = SD.open(path + String(current.Day()), FILE_WRITE);
    f.print(String(current.Hour()) + ':' + String(current.Minute()) + ":" + String(current.Second()) + ' ');
    return f;
  }
  bool writeToSD(char type, char *data)
  {
    File file = prepareToWrite(type);
    if (!file)
      return false;
    file.print(data);
    file.println();
    file.close();
    return true;
  }
  bool writeToSD(char type, float data)
  {
    File file = prepareToWrite(type);
    if (!file)
      return false;
    file.print(data);
    file.println();
    file.close();
    return true;
  }
};
Modules *modules;
void setup()
{
  Serial.begin(9600);
  modules = new Modules();
}

void loop()
{
  DHTData dht;

  if (modules->readDHT(dht))
  {
    modules->writeToSD('h', dht.humidity);
    modules->writeToSD('t', dht.tempreature);
  }
}
