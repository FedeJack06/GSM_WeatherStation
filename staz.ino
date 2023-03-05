#include <Arduino.h>
#include <Wire.h>
#include "RTClib.h"
#include "Adafruit_SHT31.h"
#include "Adafruit_BMP085.h"
#include <SoftwareSerial.h>

#define seaLevelPressure_hPa 1013.25
SoftwareSerial mySerial(3, 2);

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
float tAverage0 = millis();

// CONTROLLI PER SENSORI
bool term_control = true;
bool rtc_control = true;
bool bar_control = true;

Adafruit_SHT31 sht31 = Adafruit_SHT31();
RTC_DS1307 rtc;
Adafruit_BMP085 bmp;

// FUNZIONI ___________________________________________________________________________
void anemometer()
{
    n++;
    if (control)
    {
        t1 = millis() / 1000;
        control = false;
    }
}

void updateSerial()
{
    delay(500);
    while (Serial.available()) // CREDO CHE QUESTO NON SERVA A NIENTE
    {
        mySerial.write(Serial.read()); // Forward what Serial received to Software Serial Port
    }
    while (mySerial.available())
    {
        Serial.write(mySerial.read()); // Forward what Software Serial received to Serial Port
    }
}

void send_mess(const char s)
{
    mySerial.println("AT+CMGF=1"); // Configuring TEXT mode
    updateSerial();
    mySerial.println("AT+CMGS=\"+393890954340\""); // change ZZ with country code and xxxxxxxxxxx with phone number to sms
    updateSerial();
    mySerial.print(s); // text content
    updateSerial();
    mySerial.write(26); //è l equivalente di Ctrl+Z ogni cosa terminata da ctrl z è trattato come un messaggio
}


// DATA E ORA ****************************************************************
void data_ora(){
    if (rtc_control)
    {
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


void setup()
{
    Serial.begin(9600);
    mySerial.begin(9600);
    delay(100);

    // GSM questi controlli sono tutti sbagliato, non capisco che cazzo ritorna il gsm
    mySerial.println("AT"); // Once the handshake test is successful, it will back to OK
    if (mySerial.read() != "OK")
    {
        Serial.print("puttana eva non funzia");
    }
    mySerial.println("AT+CSQ"); // Signal quality test, value range is 0-31 , 31 is the best
    if (mySerial.read() < 5)
    {
        Serial.print("puttana eva il segnale è basso");
    }
    mySerial.println("AT+CCID"); // Read SIM information to confirm whether the SIM is plugged
    /*
    if(mySerial.read() != "OK"){               NON SO COME CAZZO CONTROLLARE SE FUNZIONA
        Serial.print("puttana eva");
    }
    */
    mySerial.println("AT+CREG?"); // Check whether it has registered in the network
    if ((mySerial.read() != 1) || (mySerial.read() != 5))
    {
        Serial.print("puttana eva a che cazzo è collegato");
    }

    // TERMOMETRO ****************************************************************
    if (!sht31.begin(0x44))
    { // Set to 0x45 for alternate I2C address
        send_mess("il termometro non va puttana eva");
        term_control = false;
    }

    // RTC ***********************************************************************
    /*
    if (!rtc.begin()) { //la rtc ha range di lavoro 0-70 gradi, quindi nel casi si andasse sotto zero pesantemnte (inverno) come si fa?
      send_mess("Couldn't find RTC");
    }
    */
    if (!rtc.isrunning())
    {
        send_mess("RTC is NOT running!");
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
    if (!bmp.begin())
    {
        send_mess("Could not find a valid BMP085 sensor, check wiring!");
        bar_control = false;
    }
}

void loop()
{

    float tempAverage = 0;
    float humAverage = 0;
    float pressAverage = 0;

    int tempCount = 0;      //  MEGLIO USARE UN LONG?
    int humCount = 0;
    int pressCount = 0;


    // TEMPERATURA E UMIDITA ***********************************************************
    if (term_control)
    {

        float t = sht31.readTemperature();
        float h = sht31.readHumidity();

        if (!isnan(t))
        { // check if 'is not a number'
            //MEDIA
            tempAverage += t;
            tempCount++;

            Serial.print("Temp *C = ");
            Serial.print(t);
            Serial.print("\t");

        }
        else
        {
            Serial.println("Failed to read temperature");
            Serial.print("\t");
        }

        if (!isnan(h))
        { // check if 'is not a number'
            humAverage += h;
            humCount++;

            Serial.print("Hum. % = ");
            Serial.println(h);
            Serial.print("\t");
        }
        else
        {
            Serial.println("Failed to read humidity");
            Serial.print("\n");
        }
    }

    // ANEMOMETRO *******************************************************************

    // a 2.4 km/h lo switch si chiude 1 volta al secondo
    t2 = millis() / 1000;

    if ((t2 - t1) >= 10)
    {
        count10 = n;
        n = 0;
        control = true;
    }

    count = count10 / (t2 - t1) // numero di volte in cui si chiude lo switch in un secondo
    vel = 2.4 * count;

    Serial.print("Velocita: ");
    Serial.print(vel);
    Serial.print("\t");

    // BAROMETRO *********************************************************
    if (bar_control)
    {
        float pressure = bmp.readPressure();
        if (!isnan(pressure))
        {
            pressAverage += pressure;
            pressCount++;

            Serial.print("Pressione: ");
            Serial.print(pressure);
            Serial.print("\t");
        }
        else
        {
            Serial.Print("Unable to read pressure");
        }
    }

    if((millis() - tAverage0) > 600000) //600000 sono 10 minuti
    {
        //MANDA I DATI
        tempAverage = tempAverage / tempCount;
        humAverage = humAverage / humCount;
        pressAverage = pressAverage / pressCount;
        data_ora();
        tAverage0 = millis();
    }
    

    delay(200);
}