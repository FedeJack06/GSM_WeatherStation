#include <Arduino.h>
#include <Wire.h>
#include "RTClib.h"
#include "Adafruit_SHT31.h"
#include "Adafruit_BMP085.h"


#define seaLevelPressure_hPa 1013.25


unsigned char temp = A0;
unsigned int anem = 3;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
unsigned int n = 0;
unsigned int count10 = 0;
unsigned int count = 0;
bool control = true;
float t1 = 0;
float t2 = 0;
float vel = 0;


Adafruit_SHT31 sht31 = Adafruit_SHT31();
RTC_DS1307 rtc;
Adafruit_BMP085 bmp;


//FUNZIONI ___________________________________________________________________________

void anemometer(){
  n++;
  if(control){
    t1 = millis()/1000;
    control = false;
  }
}


void setup(){
  Serial.begin(9600);

  //TERMOMETRO ****************************************************************
  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate I2C address
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }

  //RTC ***********************************************************************
  if (! rtc.begin()) { //la rtc ha range di lavoro 0-70 gradi, quindi nel casi si andasse sotto zero pesantemnte (inverno) come si fa?
    Serial.println("Couldn't find RTC");
    while (1);
  }
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  //ANEMOMETRO *******************************************************************
  attachInterrupt(digitalPinToInterrupt(anem),anemometer,HIGH);

  //BAROMETRO *********************************************************
  if (!bmp.begin()) 
  {
	  Serial.println("Could not find a valid BMP085 sensor, check wiring!");
	  while (1) {}
  }

}

void loop(){



//DATA E ORA ****************************************************************
  DateTime now = rtc.now();

  Serial.print(now.day(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.year(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(") ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.print("\t");



//TEMPERATURA E UMIDITA ***********************************************************
  float t = sht31.readTemperature();
  float h = sht31.readHumidity();

  if (! isnan(t)) {  // check if 'is not a number'
    Serial.print("Temp *C = "); 
    Serial.print(t); 
    Serial.print("\t");

  } else { 
    Serial.println("Failed to read temperature");
    Serial.print("\t");
  }
  
  if (! isnan(h)) {  // check if 'is not a number'
    Serial.print("Hum. % = ");
    Serial.println(h);
    Serial.print("\t");
  } else { 
    Serial.println("Failed to read humidity");
    Serial.print("\n");
  }


//ANEMOMETRO *******************************************************************

  //a 2.4 km/h lo switch si chiude 1 volta al secondo 
  t2 = millis()/1000;

  if((t2-t1) >= 10)
  {
    count10 = n;
    n = 0;
    control = true;
  }

  count = count10/(t2-t1) //numero di volte in cui si chiude lo switch in un secondo
  vel = 2.4*count;

  Serial.print("Velocita: ");
  Serial.print(vel);
  Serial.print("\t");

//BAROMETRO *********************************************************
  float pressure = bmp.readPressure();

  if(!isnan(pressure))
  {
    Serial.print("Pressione: ");
    Serial.print(pressure);
    Serial.print("\t");
  }
  else
  {
    Serial.Print("Unable to read pressure");
  }  
  




  delay(1000);

}