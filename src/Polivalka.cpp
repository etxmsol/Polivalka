// ******************************************************************
//
// Implementation of Arduino's setup and loop
//
// Date: 2015-11-05
// Author: Mikhail Soloviev
//
// ******************************************************************
#include "Arduino.h"
#include "SD.h"
#include <RTClib.h>


static const char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};


const uint16_t HYGROMETER_PIN = 8;			// moisture input (digital)
const uint16_t HYGROMETER_ANALOG_PIN = 54;	//
const uint16_t THRESHOLD_DRYNESS = 120;
const uint16_t MAGNET_VALVE_PIN = 9;		// valve control output
static const uint8_t ALARM_LED_PIN = 3;

RTC_DS1307 rtc;

// change this to match your SD shield or module;
// Arduino Ethernet shield: pin 4
// Adafruit SD shields and modules: pin 10
// Sparkfun SD shield: pin 8
const int chipSelect = 10;


void setup()
{
	// debugging channel

	Serial1.begin(9600);

	while (!Serial1)
	{
	  delay(100);
	}

	Serial1.println("Welcome to Polivalka 0.1");

	Serial1.println("Configuring soil moisture sensor pin");

	pinMode(HYGROMETER_PIN, INPUT);

	Serial1.println("Configuring magnetic valve control pin");

	pinMode(HYGROMETER_PIN, INPUT);
	pinMode(HYGROMETER_ANALOG_PIN, INPUT);
	digitalWrite(MAGNET_VALVE_PIN, LOW);

	// ALRAM
	pinMode(ALARM_LED_PIN, OUTPUT);
	digitalWrite( ALARM_LED_PIN, LOW );



	// The Real Time Clock

	rtc.begin();			// returns bool, but is never false

	if(rtc.isrunning())
		Serial1.println("RTC is running");
	else
	{
		Serial1.println("RTC is NOT running");
		rtc.adjust(DateTime(__DATE__, __TIME__));		// setup the current date and time initially
	}

    DateTime now = rtc.now();

    Serial1.print(now.year(), DEC);
    Serial1.print('/');
    Serial1.print(now.month(), DEC);
    Serial1.print('/');
    Serial1.print(now.day(), DEC);
    Serial1.print(" (");
    Serial1.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial1.print(") ");
    Serial1.print(now.hour(), DEC);
    Serial1.print(':');
    Serial1.print(now.minute(), DEC);
    Serial1.print(':');
    Serial1.print(now.second(), DEC);
    Serial1.println();

	// On the Ethernet Shield, CS is pin 4. It's set as an output by default.
	// Note that even if it's not used as the CS pin, the hardware SS pin
	// (10 on most Arduino boards, 53 on the Mega) must be left as an output
	// or the SD library functions will not work.
	pinMode(SS, OUTPUT);


	if ( !SD.begin(chipSelect, 11, 12, 13) )
	{
		Serial1.println("SD card not found");
		digitalWrite( ALARM_LED_PIN, HIGH );

	}
	else
	{
		Serial1.println("SD card OK");
	}

}

const uint16_t dailyWaterLimit = 25;
uint16_t consumedToday = 0;

const char fileName[] = "water.txt";

int today = 0;

bool log(const char * buf)
{
	Serial1.print( "opening file " );
	Serial1.println( fileName );
	File f = SD.open( fileName, FILE_WRITE);

	if (f)
	{
		if( !f.println(buf) )
		{
			digitalWrite( ALARM_LED_PIN, HIGH );

			Serial1.println( "Failed to log. Is the SD inserted? Do not forget to reset after insertion" );
			return false;
		}

		f.close();
	} else {
	// if the file didn't open, print an error:
		digitalWrite( ALARM_LED_PIN, HIGH );

		Serial1.print("error opening ");
		Serial1.print( fileName );
		Serial1.println(" for writing");
		return false;
	}
	return true;
}

// The loop function is called in an endless loop
void loop()
{
	DateTime now = rtc.now();

	if(!today)
	{
		today = now.day();
	}

	if(today != now.day())
	{
		consumedToday = 0;		// reset daily safety limit
		today = now.day();
	}

	char buf[128];

	int dryness = 0;

	for(int i = 0; i < 5; i++)
	{
		dryness += analogRead(HYGROMETER_ANALOG_PIN);
		delay(1000);
	}

	dryness /= 5;

	sprintf(buf, "%d%02d%02d %02d:%02d:%02d DRYNESS = %d", now.year(), now.month(), now.day(), now.hour(),
			now.minute(), now.second(), dryness);

	log(buf);

	Serial1.println( buf );


	if(consumedToday < dailyWaterLimit && dryness >= THRESHOLD_DRYNESS)
	{

		sprintf(buf, "%d%02d%02d %02d:%02d:%02d VALVE OPEN", now.year(), now.month(), now.day(), now.hour(),
				now.minute(), now.second());

		log(buf);

		Serial1.println( buf );

		digitalWrite(MAGNET_VALVE_PIN, HIGH);
		delay(3000);
		digitalWrite(MAGNET_VALVE_PIN, LOW);

		consumedToday += 3;

		sprintf(buf, "%d%02d%02d %02d:%02d:%02d VALVE SHUT. Water time consumed: %d sec", now.year(), now.month(), now.day(), now.hour(),
				now.minute(), now.second(), consumedToday);

		log(buf);

		Serial1.println( buf );

		if(consumedToday > dailyWaterLimit)
		{
			log("Daily limit consumed. Resting till tomorrow");
			Serial1.print("Daily limit consumed. Resting till tomorrow");
		}
	}
	delay(3600000L);	// sleep for one hour

}
