 //Sender Code
 #include <SoftwareSerial.h>
 SoftwareSerial link(3, 2); // Rx, Tx
  

  void setup() 
  {
    link.begin(9600); //setup software serial
    Serial.begin(9600);    //setup serial monitor
  }

  void loop()  
  {  
  if (link.available())
      Serial.write(link.read());
  if (Serial.available())
      link.write(Serial.read());  
   
  }