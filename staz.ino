#include <Wire.h>
#include "RTClib.h"
#include "Adafruit_SHT31.h"
#include "Adafruit_BMP085.h"
#include <SoftwareSerial.h>

// GSM
SoftwareSerial mySerial(3, 2);

// RTC
RTC_DS1307 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
bool rtc_control = true;

// BAROMETRO
Adafruit_BMP085 bmp;

#define seaLevelPressure_hPa 1013.25 // NB DA TARARE CON MISURAZIONI DI ALTRE STAZIONI VICINE NORMALIZZATE SLM
bool bar_control = true;

// VELOCITA VENTO: presuppone anemometro con contatto reed cioe ogni giro un contato, con un interrupt
conto ogni contatto sapendo che un contatto al secondo sono due virgola quattro metri al secondo unsigned long t1 = 0;
unsigned long t2 = 0;
float vel = 0;
unsigned int nPulseWind = 0; // numero di conteggi, variabile incrementale
unsigned int count10 = 0;    // numero di conteggi in 10 secondi
unsigned int count = 0;      // numero di conteggi al secondo
bool control = true;         // fa le cose sono quando questa variabile è vera
#define anemPin = 3;         // pin su cui abbiamo l interrupt

// TERMOMETRO & IGROMETRO
Adafruit_SHT31 sht31 = Adafruit_SHT31();
bool term_control = true;
float tempAverage = 0;
float humAverage = 0;
unsigned int tempCount = 0;
unsigned int humCount = 0;

float tempAverageDay = 0;
float humAverageDay = 0;
unsigned int tempCountDay = 0;
unsigned int humCountDay = 0;

int daySaved = 0;

// PRESSIONE
float pressAverage = 0;
unsigned int pressCount = 0;

float pressAverageDay = 0;
unsigned int pressCountDay = 0;

// TEMPORIZZAZIONI
unsigned int tAverage0 = millis();

// INTERRUPT ____________________________________________________________________________________________
void anemometer()
{
    nPulseWind++;
    if (control)
    {
        t1 = millis() / 1000;
        control = false;
    }
}

void contaGocce()
{
    // mmPioggia += mmGocciaSingola;
}

// DATA E ORA _____________________________________________________________________________________________
void data_ora()
{
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

// INIZIALIZZAZIONI _____________________________________________________________________________________________________________

void initRTC()
{
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
}

void initTemp()
{
    if (!sht31.begin(0x44))
    { // Set to 0x45 for alternate I2C address
        send_mess("il termometro non va puttana eva");
        term_control = false;
    }
}

void initBarometro()
{
    if (!bmp.begin())
    {
        send_mess("Could not find a valid BMP085 sensor, check wiring!");
        bar_control = false;
    }
}

void initWindSpeed()
{
    attachInterrupt(digitalPinToInterrupt(anemPin), anemometer, HIGH);
}

void initGSM()
{
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
}

// FUNZIONI IN LOOP__________________________________________________________________________________________________________________
void readTempHum()
{
    if (term_control)
    {
        float t = sht31.readTemperature();
        float h = sht31.readHumidity();

        if (!isnan(t))
        { // check if 'is not a number'
            // MEDIA
            tempAverage += t;
            tempCount++;

            tempAverageDay += t;
            tempCountDay++;

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

            humAverageDay += h;
            humCountDay++;

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
}

void readPressure()
{
    if (bar_control)
    {
        float pressure = bmp.readPressure();
        if (!isnan(pressure))
        {
            pressAverage += pressure;
            pressCount++;

            pressAverageDay += pressure;
            pressCountDay++;

            Serial.print("Pressione: ");
            Serial.print(pressure);
            Serial.print("\t");
        }
        else
        {
            Serial.Print("Unable to read pressure");
        }
    }
}

void readWindSpeed()
{
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
}

void readWindDir()
{
}

void readPioggia()
{
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
    mySerial.write(26); // è l equivalente di Ctrl+Z ogni cosa terminata da ctrl z è trattato come un messaggio
}

void resetAverage(int avg)
{
    if (avg == 1)
    {
        tempAverage = 0;
        humAverage = 0;
        pressAverage = 0;
        tempCount = 0;
        humCount = 0;
        pressCount = 0;
    }

    if (avg == 2)
    {
        tempAverageDay = 0;
        humAverageDay = 0;
        pressAverageDay = 0;
        tempCountDay = 0;
        humCountDay = 0;
        pressCountDay = 0;
    }
}

void send_data(float windSpeed, float temp, float hum, float press, float dayAvgWind, float dayAvgTemp, float dayAvgHum, float dayAvgPress) // COPIATO DA PAOLO ALIVERTI
{
    // MI PREPARO ALLA CHIMATA HTTP
    sim800.println("AT+CFUN=1"); // full funzioni
    resp("OK");
    sim800.println("AT+CGATT=1"); // collegati a gprs
    resp("OK");
    sim800.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
    resp("OK");
    sim800.println("AT+SAPBR=3,1,\"APN\",\"TM\"");
    resp("OK");
    sim800.println("AT+SAPBR=1,1");
    resp("OK");
    sim800.println("AT+HTTPINIT"); // Init HTTP service
    resp("OK");
    sim800.println("AT+HTTPPARA=\"CID\",1");
    resp("OK");
    sim800.println("AT+HTTPPARA=\"REDIR\",1"); // Auto redirect
    resp("OK");

    // CHIMATA HTTP, nel link dovranno esserci tutte le variabili in ingresso
    sim800.println("AT+HTTPPARA=\"URL\",\"http://www.zeppelinmaker.it/service.php?sens=" + String(v) + "\"");
    resp("OK");
    sim800.println("AT+HTTPACTION=0"); // call
    resp("OK");
    delay(5000);
    sim800.println("AT+HTTPREAD");
    resp("OK");
}

int resp(String txt)
{ // PAOLO ALIVERTI MA CHE CAZZO FAI
    int n = 0;
    String ret = "";
    bool LOOP = true;
    while (LOOP)
    {

        if (sim800.available())
        {
            char c = sim800.read();
            if (c == '\n')
            {
                Serial.println("RX:" + ret + "#");
                if (ret == txt)
                {
                    Serial.println("---");
                    LOOP = false;
                    n = 1;
                }
                else if (ret == "OK")
                {
                    LOOP = false;
                    n = 0;
                }
                else if (ret == "ERROR")
                {
                    LOOP = false;
                    n = -1;
                }

                ret = "";
            }
            else if (c == '\r')
            {
            }
            else
            {
                ret += c;
            }
        }
    }
    delay(1000);
    return n;
}

//__________________________________________________________________________________________________________________________________
void setup()
{
    Serial.begin(9600);
    mySerial.begin(9600);

    DateTime now = rtc.now();
    daySaved = now.day();


    // initRTC; //scommentare solo per impostare l'ora.
    initTemp();
    initBarometro();
    initWindSpeed();
    initGSM();
}

void loop()
{
    readTempHum;
    readPressure;
    readWindSpeed;

    // media in 1000 millisecondi
    if ((millis() - tAverage0) >= 1000)
    {
        tAverage0 = millis();
        Serial.println("_________________________________________________________");
        data_ora();
        Serial.print("\t");
        Serial.print(tempAverage, 6);
        Serial.print("\t");
        Serial.print(tempCount, 6);
        Serial.print("\t");
        Serial.print((tempAverage / tempCount), 6);
        Serial.print("\t");
        // Serial.print((humAverage / humCount), 6);
        // Serial.print("\t");
        // Serial.println((pressAverage / pressCount),6);
        Serial.print("\n");
        resetAverage(1);
    }

    // media giornaliera
    DateTime now = rtc.now();
    unsigned int day = now.day();
    if (day != daySaved)
    {
        tAverageDay = millis();
        Serial.println("_________________________________________________________");
        data_ora();
        Serial.print("\t");
        Serial.print(tempAverageDay, 6);
        Serial.print("\t");
        Serial.print(tempCountDay, 6);
        Serial.print("\t");
        Serial.print((tempAverageDay / tempCountDay), 6);
        Serial.print("\t");
        // Serial.print((humAverageDay / humCountDay), 6);
        // Serial.print("\t");
        // Serial.println((pressAverageDay / pressCountDay),6);
        Serial.print("\n");
        resetAverage(2);
    }

    delay(1000);
}
