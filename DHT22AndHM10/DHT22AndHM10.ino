#include <SoftwareSerial.h>
#include "DHT.h"
#define DHTPIN 4
#define DHTTYPE DHT22
SoftwareSerial BTserial(2, 3);
DHT dht(DHTPIN, DHTTYPE);
long startTime;
void setup()
{
    Serial.begin(9600);
    Serial.println("Enter AT commands:");

    // HC-06 default serial speed is 9600
    BTserial.begin(9600);
    dht.begin();
    startTime=millis();
}

void loop()
{

    if (BTserial.available()) Serial.write(BTserial.read());

    if ((millis()-startTime)>2000)
    {
        float h = dht.readHumidity();
        float t = dht.readTemperature();
        if (isnan(h) || isnan(t))
          Serial.println(F("Failed to read from DHT sensor!"));

        else{
        BTserial.print(h);
        BTserial.print(" ");
        BTserial.print(t);
        Serial.println(t);
          }
        startTime=millis();

    }

}
