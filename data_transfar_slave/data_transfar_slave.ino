//Sender code
#include <SoftwareSerial.h>
#include <HX711_ADC.h>
#include <EEPROM.h>

SoftwareSerial link(7, 8); // Rx, Tx

#define LOADCELL_DOUT_PIN 2
#define LOADCELL_SCK_PIN 6
#define CALVAL_EEPROMADRESS 0
#define HX711_RESPONCE_TIME 1000
#define TARE_TIMEOUT_TIME 4000


HX711_ADC scale(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
long scale_last_read;
bool scale_ready_to_read;




// code for scale interuput

void dataReadyISR() {
  if (scale.update()) {
    scale_ready_to_read = true;
  }
}


void setup() 
{
  link.begin(9600); //setup software serial
  Serial.begin(9600);    //setup serial monitor


  //scale setup

  scale_ready_to_read=false;
  scale.begin();
  scale.start(2000, true);
  float calibrationValue;
  EEPROM.get(CALVAL_EEPROMADRESS, calibrationValue);
  if (scale.getTareTimeoutFlag()) {
  Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
  while (1);
  }
  else{
    scale.setCalFactor(calibrationValue); // set calibration value (float)
    Serial.println("Startup is complete");
  }
  scale_last_read=millis();
  attachInterrupt(digitalPinToInterrupt(LOADCELL_DOUT_PIN), dataReadyISR, FALLING);
}
long readHX711(float &data){
    if(millis() - scale_last_read > HX711_RESPONCE_TIME && scale_ready_to_read){
      data=scale.getData();
      scale_last_read=millis();
      scale_ready_to_read=false;
      return true;
    }
    return false;
  }
bool tareScale(){
  scale.tareNoDelay();
  long start_wait=millis();
  while(!scale.getTareStatus()){
    if(millis() - start_wait >= TARE_TIMEOUT_TIME){
      return false;
    }
  }
  return true;
}

float data;
void loop()  
{  
/*if (link.available())
    Serial.write(link.read());
if (Serial.available())
    link.write(Serial.read()); */ 
if(readHX711(data)){
   link.println(data);
}
}
