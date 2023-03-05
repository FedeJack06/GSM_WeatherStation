#include <Arduino.h>
#include <Wire.h>
#include "RTClib.h"
#include "Adafruit_SHT31.h"
#include "Adafruit_BMP085.h"


// RTC
RTC_DS1307 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
bool rtc_control = true;


//BAROMETRO
Adafruit_BMP085 bmp;

#define seaLevelPressure_hPa 1013.25
bool bar_control = true;


// VELOCITA VENTO
float t1 = 0;
float t2 = 0;
float vel = 0;
unsigned int n = 0;
unsigned int count10 = 0;
unsigned int count = 0;
bool control = true;
unsigned int anem = 3;


// TERMOMETRO & IGROMETRO
Adafruit_SHT31 sht31 = Adafruit_SHT31();
bool term_control = true;


//MEDIE 
float tAverage0 = millis();
float tempAverage = 0;
float humAverage = 0;
float pressAverage = 0;
int tempCount = 0;  //  MEGLIO USARE UN LONG?
int humCount = 0;
int pressCount = 0;


// FUNZIONI ___________________________________________________________________________
void anemometer() {
  n++;
  if (control) {
    t1 = millis() / 1000;
    control = false;
  }
}


// DATA E ORA ********************************************
void data_ora() {
  if (rtc_control) {
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
  }
}



void setup() {
  Serial.begin(9600);

  // TERMOMETRO ****************************************************************
  if (!sht31.begin(0x44)) {
    Serial.println("Couldn't find SHT31");
    term_control = false;
  }

  // RTC ***********************************************************************
  if (!rtc.begin()) {  // la rtc ha range di lavoro 0-70 gradi, quindi nel casi si andasse sotto zero pesantemnte (inverno) come si fa?
    Serial.println("Couldn't find RTC");
    rtc_control = false;
  }
  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    rtc_control = false;
  }

  // ANEMOMETRO *******************************************************************
  attachInterrupt(digitalPinToInterrupt(anem), anemometer, HIGH);

  // BAROMETRO *********************************************************
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
    bar_control = false;
  }
}

void loop() {
  // TEMPERATURA E UMIDITA ***********************************************************
  if (term_control) {
    float t = sht31.readTemperature();
    float h = sht31.readHumidity();

    if (!isnan(t)) {  // check if 'is not a number'
      tempAverage += t;
      tempCount++;
      Serial.print("Temp *C = ");
      Serial.print(t);
      Serial.print("\n");
      delay(200);

    } else {
      Serial.println("Failed to read temperature");
      Serial.print("\t");
    }

    if (!isnan(h)) {  // check if 'is not a number'
      humAverage += h;
      humCount++;
      /*
      Serial.print("Hum. % = ");
      Serial.println(h);
      Serial.print("\n");
      */
    } else {
      Serial.println("Failed to read humidity");
      Serial.print("\n");
    }
  }

  // ANEMOMETRO *******************************************************************
  /*
  // a 2.4 km/h lo switch si chiude 1 volta al secondo
  t2 = millis() / 1000;

  if ((t2 - t1) >= 10)
  {
    count10 = n;
    n = 0;
    control = true;
  }

  count = count10 / (t2 - t1); // numero di volte in cui si chiude lo switch in un secondo
  vel = 2.4 * count;

  Serial.print("Velocita: ");
  Serial.print(vel);
  Serial.print("\t");
*/
  // BAROMETRO *********************************************************
  if (bar_control) {
    float pressure = bmp.readPressure();
    if (!isnan(pressure)) {
      pressAverage += pressure;
      pressCount++;

      Serial.print("Pressione: ");
      Serial.print(pressure);
      Serial.print("\t");
    } else {
      Serial.print("Unable to read pressure");
    }
  }

  if ((millis() - tAverage0) >= 1000) {
    Serial.println("_________________________________________________________");
    data_ora();
    Serial.print("\t");
    Serial.print(tempAverage, 6);
    Serial.print("\t");
    Serial.print(tempCount, 6);
    Serial.print("\t");
    Serial.print((tempAverage / tempCount), 6);
    Serial.print("\t");
    //Serial.print((humAverage / humCount), 6);
    //Serial.print("\t");
    //Serial.println((pressAverage / pressCount),6);
    tempAverage = 0;
    humAverage = 0;
    pressAverage = 0;
    tempCount = 0;
    humCount = 0;
    pressCount = 0;
    delay(1000);
    tAverage0 = millis();
  }

  
}