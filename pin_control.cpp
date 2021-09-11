#include <pigpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <thread>
#include <iostream>
#include "sensor.hpp"
#include <string>
#include <cstddef>
#include <cstring>
#include <bits/stdc++.h>

namespace segments
{
    unsigned char slotOuts[] = {6, 13, 19, 26}; //Numery dotyczące czterech wyświetlaczy tworzących FJ5461BH
    unsigned char segmentOut[] = {20, 16, 8, 1, 12, 21, 25, 7};//Numery dotyczące segmentów FJ5461BH
    unsigned char letterToSegment[] = {
        0b01110111, //A
        0b01111100, //B
        0b00111001, //C
        0b01011110, //D
        0b01111001, //E
        0b01110001, //F
        0b00111101, //G
        0b01110110, //H
        0b00000110, //I
        0b00011110, //J
        0b01110101, //K
        0b00111000, //L
        0b00101001, //M
        0b00110111, //N
        0b00111111, //O
        0b01110011, //P
        0b01100111, //Q
        0b01010000, //R
        0b01101101, //S
        0b01111000, //T
        0b00111110, //U
        0b00101110, //V
        0b00101010, //W
        0b01110110, //X
        0b01101110, //Y
        0b01011011, //Z
    };

    unsigned char numberToSegment[] = {
        0b00111111, //0
        0b00000110, //1
        0b01011011, //2
        0b01001111, //3
        0b01100110, //4
        0b01101101, //5
        0b01111101, //6
        0b00000111, //7
        0b01111111, //8
        0b01101111, //9
    };

    void pickSlot(unsigned char slots)
    {
        for (int i = 0; i < sizeof(slotOuts) / sizeof(slotOuts[0]); i++)
            if ((slots >> i) & 1)//sprawdzamy na których bitach jest '1', aby wiedzieć, który z 4 wyświetlaczy zapalić
            {
                gpioSetMode(slotOuts[i], PI_OUTPUT);
                gpioWrite(slotOuts[i], 1);
            }
            else
                gpioSetMode(slotOuts[i], PI_INPUT);
    }

    void pickSegments(unsigned char segments)
    {
        for (int i = 0; i < sizeof(segmentOut) / sizeof(segmentOut[0]); i++)
            if ((segments >> i) & 1)//Bit po bicie sprawdzamy na których jest '1' oznaczające zapalenie się segmentu
            {
                gpioSetMode(segmentOut[i], PI_OUTPUT);
                gpioWrite(segmentOut[i], 0);
            }
            else
                gpioSetMode(segmentOut[i], PI_INPUT);
    }

    void pickCharacter(char character, bool dot = false)//Funkcja określajaca jaki znak: literę/cyfrę wyswietlić na wybranym segmencie, oraz czy wyswietlić go z kropką
        //Uwaga: wielkość liter nie ma znaczenia
    {   //Funkcja przetworzy znak char na zapalenie się odpowiednich segmentów przy użyciu tablicy letterToSegment[]
        if (character >= 'A' && character <= 'Z')//Warunek jeśli znak to duża litera
            pickSegments(letterToSegment[character - 'A'] | (dot ? 0b10000000 : 0));
        else if (character >= '0' && character <= '9')//Warunek jeśli znak to cyfra
            pickSegments(numberToSegment[character - '0'] | (dot ? 0b10000000 : 0));
        else if (character >= 'a' && character <= 'z')//Warunek jeśli znak to mała litera - wielkość liter nie ma znaczenia, zostaną wyświetlone odpowiednie segmenty
            pickSegments(letterToSegment[character - 'a'] | (dot ? 0b10000000 : 0));
        else//Warunek dla pozostałych przypadkow
            pickSegments((dot ? 0b10000000 : 0));
    }

    void printOnce(const char *text)//Funkcja wyświetlająca jednorazowo napis skłądający się maksymalnie z 4 znaków na wyświetlaczu
    {
        for (unsigned char i = 0; i < 4; i++)
        {
            pickSlot(0);//Pozwala to na początek nie wybrać żadnego z czterech wyświetlaczy
            if (*text == 0)
                break;
            pickCharacter(*text, *(text + 1) == '.');

            text++;
            while (*text == '.')
                text++;

            pickSlot(1 << i);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));//sleep wątku zapewnia nam niezauważalne dla ludzkiego oko przełączanie się 1 z czterech wyświetlaczy na drugi
            //W efekcie nasze oko zobaczy wyświetlany jednocześnie komunikat stworzony z maksymalnie czterech symboli i kropek
        }

        pickSlot(0);
    }

    std::string getSubText(std::string fullText, int index)//Funkcja potrzebna do obsługi napisów o długości większej niz 4 znaki.
        //Wykorzystywana w dalszym fragmencie w połączeniu zapewnia 'przesuwający napis' podobnie jak np. w reklamach 
    {
        size_t Size = strlen(fullText.c_str());//określ dlugość napisu
        std::string str;
        if (index < Size - 3)
        {
            str = fullText.substr(index, 4);
        }
        else
        {//dla trzech ostatnich znaków takiego długiego napisu pozwoli to na przeniesienie ostatnich liter na początek kolejnego powtórzenia napisu 
        //na początek kolejnego powtórzenia napisu zapewniając wybrany efekt

            if (index == Size - 3)
            {
                str = fullText.substr(index, 4) + fullText.substr(0, 1);
            }
            if (index == Size - 2)
            {
                str = fullText.substr(index, 4) + fullText.substr(0, 2);
            }
            if (index == Size - 1)
            {
                str = fullText.substr(index, 4) + fullText.substr(0, 3);
            }
        }
        return str;
    }

    float averageOfWindow(float arr[])//Oblicz średnią wielkość temperatury. Określ rozmiar tablicy okna do kontroli temperatury
    { //Gdzie zawsze wielkosc okna jest równa 15
        int n = 15;
        // Dla takiej tablicy zliczaj sumę
        int sum = 0;
        for (int i = 0; i < n; i++)
            sum += arr[i];

        return (float)sum / n;//Zwróć średnią arytmetyczną dla tablicy
    }

    float humAverage(float arr[])///Oblicz średnią wartość wilgotności. Określ rozmiar tablicy okna do kontroli jej
    { //Gdzie zawsze wielkość okna dla wilgotności jest równa 10
        int n = 10;
        // Zliczaj sumę elementów
        int sum = 0;
        for (int i = 0; i < n; i++)
            sum += arr[i];

        return (float)sum / n;//Zwróć średnią arytmetyczną
    }

    void init()//Funkcja inicjalizująca GPIO Rasbperry PI
    {
        if (gpioInitialise() == PI_INIT_FAILED)
            throw std::runtime_error("Cannot initialize gpio!");

        pickSlot(0);     //Argument 0 oznacza wybór żadnego z czterech wyświetlaczy
        pickSegments(0); //Argument 0 oznacza wybór żadnego z segmentów
    }
}

void buzzer(bool enable)//Funkcja obsługująca buzzer za pomocą GPIO.
{
    gpioSetMode(22, PI_OUTPUT);
    gpioWrite(22, enable);
}


void led(unsigned char color)//Funkcja obsługująca diode LED za pomocą GPIO.
{
{
    gpioSetMode(0, PI_OUTPUT);
    gpioSetMode(5, PI_OUTPUT);
    gpioSetMode(11, PI_OUTPUT);

    gpioWrite(0, color == 1);
    gpioWrite(5, color == 2);
    gpioWrite(11, color == 3);
}

void ledMix(float r, float g, float b)//Funkcja zapewniajaca mrugnięcie diody w wybranym kolorze w formie RGB
{   
    float rp = r / (r + g + b);
    float gp = g / (r + g + b);
    float bp = b / (r + g + b);
    float brightness = rp * r + gp * g + bp * b;
    unsigned onTime = 10 * brightness;
    unsigned offTime = 10 - onTime;

    led(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(offTime));
    led(1);
    std::this_thread::sleep_for(std::chrono::milliseconds((int)(onTime * rp)));
    led(2);
    std::this_thread::sleep_for(std::chrono::milliseconds((int)(onTime * gp)));
    led(3);
    std::this_thread::sleep_for(std::chrono::milliseconds((int)(onTime * bp)));
    led(0);
}

void debugSegment()//Funkcja testująca wyświetlacz pozwala zobaczyć czy wszystkie segmenty działają poprawnie
{
    segments::pickSlot(0b1111);

    for (int i = 0; i < 8; i++)
    {
        segments::pickSegments(1 << i);
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }

    segments::pickSegments(0b11111111);
    for (int i = 0; i < 4; i++)
    {
        segments::pickSlot(1 << i);
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }

    segments::pickSlot(0);
}

void debugLED()//funkcja testująca diodę LED pozwalająca szybko przetestować czy każda z mniejszych diód działa poprawnie
{
    for (int i = 0; i < 3; i++)
    {
        led(i + 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
    led(0);
}

void debugBuzzer()//Funkcja testująca buzzer na czas 100ms.
{
    buzzer(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    buzzer(false);
}

void debug()//Za pomocą tej funkcji wywołujemy funkcje testowe wszystkich kluczowych elementów.
{
    debugSegment();
    debugLED();
    debugBuzzer();
}

int main()//Funkcja główna programu
{
    hdt11::init();//inicjujemy kod sensora HDT11
    segments::init();//inicjujemy GPIO Raspberry PI
    buzzer(false);//Upewniamy się, że buzzer jest wyłączony
    led(0);//A także, że dioda nie świeci
    debug();//Wykonujemy  testy diody, buzzera i wyświetlacza jako estetyczna inicjacja programu

    std::string text = "INIT";//Napis placeholder INIT, gdzie zmienna text jest bardzo ważna i przechowuje odpowiedni tekst, który ma zostać wyświetlony. 

    std::thread segmentThread([&text]() {//Tworzymy specjalny wątek dla wyświetlacza 8-segmentowego, którego argumentem jest nasz tekst
        while (true)//i w pętli wykonujemy
            segments::printOnce(text.c_str());//wyświetlanie odpowiedniego tekstu zależnie co w danej chwili znajduje się pod zmienną text.
    });

    unsigned char r = 0, g = 0, b = 0;//pod zmienne określające kolejno wartości od 0 do 255 dla diod led: red, green, blue wartości 0, które użyjemy w wątku
    std::thread ledThread([&r, &g, &b]() {//Wątek dla diody led
        while (true)//w pętli przekazujemy wartości
            ledMix(r / 255.0, g / 255.0, b / 255.0);//odpowiednio sformatowane wielkości określające kolory pomniejszych diod.
    });

    std::string textPUT = "PUT POZNAN ";//Napis informujący o uczelni a jednocześnie test wyświetlania napisów dłuższych niż 4 znaki
    size_t SizeT = strlen(textPUT.c_str());//Długość napisu
    bool initialText = true;//Zmienna mająca na celu zapewnienie wyświetlenia powyższego napisu tylko raz.


    //[Temperature service section] sekcja zmiennych  powiadomień dotyczących temperatury 
    std::string textTooHot = "cool the place ";//Napis przy utrzymaniu się zbyt dużej temperatury
    size_t SizetH = strlen(textTooHot.c_str());//Długość napisu analogicznie jak wcześniej
    std::string textTooCold = "heat up ";//Napis przy zbyt niskiej utrzymywanej temperaturze
    size_t SizetC = strlen(textTooCold.c_str());
    std::string textIsPerfect = "its perfect ";//Napis pojawiający się dla zakresu temperatur zdrowego wzrostu
    size_t SizeP = strlen(textIsPerfect.c_str());

    int lenCounter = 0;//Licznik długości 'tablicy okna' dla temperatur
    float avg;//średnia wartość temperatury w oknie
    float avgArray[15] = {0};//Tablica okna temperatur, początkowo wypełniona zerami
    int tempHighCounter = 0;//Licznik odpowiadający za kontrolę czy temperatura jest większa od maxAcceptedTemp
    int tempLowCounter = 0;//Licznik odpowiadający za kontrolę czy temperatura jest mniejsza od minAcceptedTemp
    int tempPerfectCounter = 0;//Licznik odpowiadający za kontrolę czy temperatura jest pomiędzy minGoodTemp a maxGoodTemp

    float maxAcceptedTemp = 36.0f;//Zmienna uniwersalna zalezna od hodowanej rosliny - maksymalna tolerowana temperatura do wzrostu
    float minAcceptedTemp = 26.0f;//Zmienna uniwersalna zalezna od hodowanej rosliny - minimalna tolerowana temperatura do wzrostu

    float maxGoodTemp = 30.5f;//Zmienna uniwersalna zalezna od hodowanej rosliny - dolna granica odpowiedniego zakresu temperatury
    float minGoodTemp = 29.0f;//Zmienna uniwersalna zalezna od hodowanej rosliny - górna granica odpowiedniego zakresu temperatury

    //Humidity service section
    int humCounter = 0;//Licznik długości 'tablicy okna' dla wilgotności
    float avgHum;//średnia wartość wilgotności w oknie
    float avgHumArray[10] = { 0 };//Tablica okna wilgotności, początkowo wypełniona zerami
    int humCriticalCounter = 0;//Licznik odpowiadający za kontrolę wilgotności czy nie jest poniżej dolnej akceptowalnej granicy
    float minAcceptedHumidity = 15.0f;//Zmienna uniwersalna zalezna od hodowanej rosliny - dolna granica wilgotności akceptowana do zdrowego wzrostu
    std::string textHumWarning = "too dry ";//Napis pojawiający się gdy ostrzegający licznik osiągnie odpowiednią wartość
    size_t SizeWarning = strlen(textHumWarning.c_str());


    while (true)//Główna pętla programu
    {
        try
        {
            //put poznan initial text
            if (initialText == true) {
                
                for (int j = 0; j < SizeT; j++)
                {
                    text = segments::getSubText(textPUT, j);
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
                initialText = false;
            }

            hdt11::sensorData data = hdt11::readRetry(32);
           
            std::cout << "Temperature " << data.temperature << "\nHumidity " << data.humidity << "\n\n";

            text = "H." + std::to_string(data.humidity);
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));

            text = "T." + std::to_string(data.temperature);
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));

            //dopoki nie zapelnimy musi być wypełnianie tablicy
            if (lenCounter < 15)
            {
                
                avgArray[lenCounter] = float(data.temperature);
                lenCounter++;
		std::cout << "Temperature array:" << std::endl;
                for (auto x = std::end(avgArray); x != std::begin(avgArray);)
                {
                    std::cout << *--x << ' ';
                }
                std::cout << std::endl;
            }
            else
            {
		std::cout << "Temperature array:" << std::endl;
                for (auto x = std::end(avgArray); x != std::begin(avgArray);)
                {
                    std::cout << *--x << ' ';
                    avg = segments::averageOfWindow(avgArray);
                }
                std::cout << std::endl;
                std::cout << "Avg Temp:" << avg << std::endl;
                //miejsce na warunki:

                //warunek zbyt duzej temperatury otoczenia
                if (avg > maxAcceptedTemp)
                { //warunek mozna smialo zmieniac, potem przewidywane 25
                    if (tempHighCounter < 52)
                    {
                        tempHighCounter++;
                        std::cout << "High count:" << tempHighCounter << std::endl;
                    }

                    if (tempHighCounter >= 15)
                    {
                        r = 200;
                        g = 102;
                        b = 10;
                        if (tempHighCounter % 5 == 0)
                        {
                            for (int j = 0; j < SizetH; j++)
                            {
                                text = segments::getSubText(textTooHot, j);
                                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                            }
                        }

                        if (tempHighCounter >= 50)
                        {
                            //jak juz tak dlugo=tyle razy z rzedu temperatura  zla to
                            std::cout << "*beep* np. np co 15 sekund " << std::endl;
                        } //alarm to alarm tylko w krytycznym momencie aby roslinka przezyla
                    }
                }
                else
                {
                    tempHighCounter = 0;
                    if (tempLowCounter == 0)
                    {
                        r = 0;
                        g = 0;
                        b = 0;
                    }
                }

                //warunek zbyt malej temperatury otoczenia
                if (avg < minAcceptedTemp)
                { //potem przewidywane 15
                    if (tempLowCounter < 52)
                    {
                        tempLowCounter++; //wstepne zabezpieczenie przed przekreceniem licznika xD. po co wiekszy niz 60
                        std::cout << "Low Count:" << tempLowCounter << std::endl;
                    }

                    if (tempLowCounter >= 15)
                    {

                        r = 0;
                        g = 0;
                        b = 255;
                        if (tempLowCounter % 5 == 0)
                        {
                            for (int j = 0; j < SizetC; j++)
                            {
                                text = segments::getSubText(textTooCold, j);
                                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                            }
                        }
                        if (tempLowCounter >= 50)
                        {
                            //jak juz tak długo = tyle razy z rzędu temperatura  zla to
                            std::cout << "*beep* np. na 5 sekund " << std::endl;
                        }
                    }
                }
                else
                {
                    tempLowCounter = 0;
                    if (tempHighCounter == 0)
                    {
                        r = 0;
                        g = 0;
                        b = 0;
                    }
                }

                //temperatura bajeczka
                if (avg <= maxGoodTemp && avg >= minGoodTemp)
                { 
                    if (tempPerfectCounter < 52)
                    {
                        tempPerfectCounter++; 
                    }

                    if (tempPerfectCounter >= 5)
                    {
                        if (tempPerfectCounter % 2 == 0) {
                            ledMix(0 / 255.0, 255 / 255.0, 0 / 255.0);//zielone mignięcie
                        }
                        
                        if (tempPerfectCounter % 10 == 0)
                        {
                            for (int j = 0; j < SizetC; j++)
                            {
                                text = segments::getSubText(textIsPerfect, j);
                                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                            }
                        }
                      
                    }
                }
                else
                {
                    tempPerfectCounter = 0;
                    if (tempHighCounter == 0 && tempLowCounter == 0)
                    {
                        r = 0;
                        g = 0;
                        b = 0;
                    }
                }
    

                //[Zmiana tablicy temp, znika najstarszy] - ważne miejsce przed ktorym muszą dziać sie rzeczy
                for (int k = 0; k < 14; k++)
                {

                    float temp = avgArray[k + 1];
                    avgArray[k] = temp;
                }
                avgArray[14] = float(data.temperature);

            }


            //Obsluga zbyt małej wilgotnosci. Drugie okno o mniejszym rozmiarze
            //dopoki nie zapelnimy musi być wypełnianie tablicy
            if (humCounter < 10)
            {

                avgHumArray[humCounter] = float(data.humidity);

                humCounter++;
                std::cout << "Humidity array:" << std::endl;
                for (auto x = std::end(avgHumArray); x != std::begin(avgHumArray);)
                {
                    std::cout << *--x << ' ';
                }
                std::cout << std::endl;
            }
            else {
                std::cout << "Humidity array:" << std::endl;
                for (auto x = std::end(avgHumArray); x != std::begin(avgHumArray);)
                {
                    std::cout << *--x << ' ';
                    avgHum = segments::humAverage(avgHumArray);
                }
                std::cout << std::endl;
                std::cout << "Avg Hum:" << avgHum << std::endl;

                //Miejsce na warunek pink led + text
                //temperatura bajeczka
                if (avgHum < minAcceptedHumidity)
                {
                    if (humCriticalCounter < 101)
                    {
                        humCriticalCounter++;
                        std::cout << "Warning counter:" << humCriticalCounter << std::endl;
                    }

                    if (humCriticalCounter >= 5)
                    {
                        if (humCriticalCounter % 2 == 0) {
                            ledMix(255 / 255.0, 153 / 255.0, 255 / 255.0);
                        }

                        if (humCriticalCounter % 10 == 0)
                        {
                            for (int j = 0; j < SizeWarning; j++)
                            {
                                text = segments::getSubText(textHumWarning, j);
                                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                            }
                        }

                    }
                }
                else
                {
                    humCriticalCounter = 0;
                }



                 //[Zmiana tablicy humidity, znika najstarszy] - ważne miejsce przed ktorym muszą dziać sie rzeczy
                for (int k = 0; k < 9; k++)
                {

                    float temp = avgHumArray[k + 1];
                    avgHumArray[k] = temp;
                }
                avgHumArray[9] = float(data.humidity);
            }

           
        }
        catch (std::exception &ex)
        {

            std::cout << ex.what() << "\n";
            text = "ERROR";
            r = 255;
            g = 0;
            b = 0;
            buzzer(true);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            buzzer(false);
        }
    }
    return 0;
}
