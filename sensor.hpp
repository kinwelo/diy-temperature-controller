#ifndef sensor_hpp
#define sensor_hpp
#include <pigpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <unistd.h>
#include <sys / time.h>
#include <sys / mman.h>
#include <sys / stat.h>
#include <sys / wait.h>
#include <sys / ioctl.h>
#include <asm / ioctl.h>

void delayMicrosecondsHard(unsigned int howLong)
//Funkcja wykorzystywna do opóŸnienia w mikrosekundach
{
	struct timeval tNow, tLong, tEnd;

	gettimeofday(&tNow, NULL);
	tLong.tv_sec  = howLong  / 1000000;
	tLong.tv_usec  = howLong  % 1000000;
	timeradd(&tNow, &tLong, &tEnd);

	while (timercmp(&tNow, &tEnd, < ))
		gettimeofday(&tNow, NULL);
}

void delayMicroseconds(unsigned int howLong)
//Funkcja, któr¹ wykonujemy opóŸnienie w mikrosekundach
{
	struct timespec sleeper;
	unsigned int uSecs  = howLong  % 1000000;
	unsigned int wSecs  = howLong  / 1000000;

	    if (howLong  == 0)
			return;
		else if (howLong  < 100)
			delayMicrosecondsHard(howLong);
	else
	{
	sleeper.tv_sec  = wSecs;
	sleeper.tv_nsec  = (long)(uSecs  * 1000L);
	nanosleep(&sleeper, NULL);
	}
}

void delayMiliseconds(unsigned int howLong)
//Funkcja, któr¹ wykonujemy opóŸnienie w milisekundach
{
	struct timespec sleeper, dummy;

	sleeper.tv_sec  = (time_t)(howLong  / 1000);
	sleeper.tv_nsec  = (long)(howLong  % 1000) * 1000000;

	nanosleep(&sleeper, &dummy);
}


//Uniwersalny kod dla czujników DHT11 lub DHT22
namespace hdt11
{


	struct sensorData
	//Struktura, w której zapisujemy pomiary temperatury i wilgotnoœci
		{
			float temperature;
			float humidity;
		};
//Dodajemy odpowiednie sta³e
const unsigned pin  = 4;
const unsigned maxtimings  = 85;

void init()//Funkcja inicjalizuj¹ca GPIO Rasbperry PI
{
	if (gpioInitialise() == PI_INIT_FAILED)
		throw std::runtime_error("Cannot initialize gpio!");
}

sensorData read()//Najwa¿niejsza funkcja odczytuj¹ca pomiary 
{
	uint8_t laststate  = 1;
	uint8_t counter  = 0;
	uint8_t j  = 0, i;
	float   f;
	int dht11_dat[5] = { 0, 0, 0, 0, 0 };//Tablica w której zapisywane s¹ pomiary

dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;
//Odpowiednio resetujemy ka¿dy element tablicy
		//Dzia³amy na odpowiednich funkcjach GPIO
		gpioSetMode(pin, PI_OUTPUT);
		gpioWrite(pin, 0);
		delayMiliseconds(18);
		gpioWrite(pin, 1);
		delayMicroseconds(40);
		gpioSetMode(pin, PI_INPUT);

		for (i  = 0; i  < maxtimings; i++)
			//Kluczowa pêtla, w której odczytujemy pomiary i zapisujemy je w odpowiednim formacie
					{
						counter  = 0;
						while (gpioRead(pin) == laststate)
						{
							counter++;
							delayMicroseconds(1);
							if (counter  == 255)
								break;
						}
						laststate  = gpioRead(pin);

						if (counter  == 255)
							break;

						if ((i  >= 4) && (i  % 2 == 0))
						{
							dht11_dat[j  / 8] <<= 1;
							if (counter  > 50)
								dht11_dat[j  / 8] |= 1;
							j++;
						}
					}
		//Warunek je¿eli po wykonaniu pêtli wszystkie elementy 
		tablicy zosta³y zmierzone prawid³owo
				if ((j  >= 40) && (dht11_dat[4] == ((dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xFF)))
				{

					sensorData result;
					result.humidity  = dht11_dat[0] + dht11_dat[1] / 10.0;
					//dht11_dat[0] przechowuje liczbê ca³kowit¹ pomiaru wilgotnoœci,
					 dht11_dat[1] zaœ liczbê po przecinku
								result.temperature  = dht11_dat[2] + dht11_dat[3] / 10.0;
					 //dht11_dat[2] przechowuje liczbê ca³kowit¹ pomiaru temperatury,
					  dht11_dat[3] zaœ liczbê po przecinku
								 return result;//Zwróæ wyniki w postaci struktury
							 }
							 else//W przeciwnym wypadku nale¿y prawdopodobnie poprawiæ po³o¿enie czujnika
								 throw std::runtime_error("Checksum missmatch");
						 }

						 sensorData readRetry(unsigned retries)
							 //Funkcja wykonuj¹ca próby ponownego odczytania danych z czujnika
								 {
									 while (true)
									 {
										 try
										 {
											 return read();
										 }
										 catch (...)
										 {
											 if (retries  == 0)
												 throw;
											 retries--;
											 std::this_thread::sleep_for(std::chrono::milliseconds(100));
										 }
									 }
								 }
}

#endif