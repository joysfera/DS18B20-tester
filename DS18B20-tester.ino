// DS18B20 tester
// written by Petr Stehlik in December 2018
// GPL v3

#include <OneWire.h>
#include <DallasTemperature.h>
#include "Joy_LCD5110.h"

#define ONEWIRE_PIN   A3
#define ONEWIRE_SENSE A2
#define POWER_PIN     A1
#define POWER_SENSE   A0

OneWire oneWire(ONEWIRE_PIN);
DallasTemperature sensors(&oneWire);

Joy_LCD5110 lcd(/*SCLK*/6, /*Din*/7, /*DC*/8, /*RESET*/ 9);

char *ftoa(char *a, double f, byte precision);
void printTemperature(byte index, byte row);
void printDeviceAddress(byte index);

void lcd_header()
{
    lcd.setInverse(false);
    lcd.clearScreen();
    lcd.setTextSize(1);
    lcd.setInverse(true);
    lcd.print("DS18B20 tester");
    lcd.clearRow();
    lcd.setInverse(false);
}

#define lcd_print(c, r, x) { lcd.setCursor(c, r); lcd.print(x); lcd.clearRow(); }

void setup(void)
{
    pinMode(POWER_SENSE, INPUT);
    pinMode(ONEWIRE_SENSE, INPUT);
    pinMode(POWER_PIN, OUTPUT);
    digitalWrite(POWER_PIN, LOW);
}

void loop(void)
{
    lcd_header();

    digitalWrite(POWER_PIN, HIGH);
    delay(100);

    // power short circuit test
    unsigned u = analogRead(POWER_SENSE);
    lcd_print(0, 5, u);
    if (u >= 1020) {
        // power is OK
    }
    else if (u > 969) {
        lcd_print(0, 1, "Vdd high curr.");
    }
    else {
        lcd_print(0, 1, "Vdd short circ");
        delay(5000);
    }

    // data short circuit test
    u = analogRead(ONEWIRE_SENSE);
    lcd_print(6*6, 5, u);
    if (u >= 1022) {
        // 1wire bus is OK
    }
    else if (u > 969) {
        lcd_print(0, 2, "Bus high curr.");
    }
    else {
        lcd_print(0, 2, "Bus short circ");
        delay(5000);
    }

    sensors.begin();

    if (sensors.getDeviceCount() > 0) {

        // test of parasitic power mode
        digitalWrite(POWER_PIN, LOW);
        delay(100);

        sensors.begin();

        if (sensors.getDeviceCount() == 0) {
            lcd.setInverse(true);
            lcd_print(0, 3, "Parasite fail!");
            delay(5000);
            return;
        }

        printDeviceAddress();
        sensors.requestTemperatures();

        float tempC = sensors.getTempCByIndex(0);
        printTemperature(tempC);
        delay(2500);

        lcd_print(0, 1, "Family test");
        for(byte cnt = 0; cnt < 20; cnt++) {
            lcd_print(12*6, 1, cnt);
            test_family();
        }
    }
    else {
        lcd_print(0, 3, "  No sensor");
    }
    delay(500);
}

void dead_end()
{
    lcd.setInverse(true);
    lcd_print(0, 5, "Reset to cont");
    while(true) ;
}

void test_family()
{
    digitalWrite(POWER_PIN, LOW);
    digitalWrite(ONEWIRE_PIN, LOW);
    delay(5000);  // wait to discharge the DS18B20 internal capacitor
    digitalWrite(POWER_PIN, HIGH);
    delay(100);
    sensors.begin();
    if (sensors.getDeviceCount() == 0) {
        lcd_print(0, 3, "No Sensor!");
        dead_end();
    }

    printDeviceAddress();
    sensors.requestTemperatures();

    float tempC = sensors.getTempCByIndex(0);
    printTemperature(tempC);
}

// function to print a device address
void printDeviceAddress()
{
    byte deviceAddress[8];
    if (! sensors.getAddress(deviceAddress, 0)) {
        lcd_print(0, 3, "ID N/A");
        dead_end();
    }

    if (deviceAddress[0] != 0x28) {
        lcd_print(0, 3, "Family error");
        dead_end();
    }

    lcd.setCursor(0, 3);
    for (uint8_t i = 1; i < 7; i++) {
        if (deviceAddress[i] < 16) lcd.print("0");
        lcd.print(deviceAddress[i], HEX);
    }
    lcd.clearRow();
}

// function to print the temperature for a device
void printTemperature(float tempC)
{
    char buf[15];
    ftoa(buf, tempC, 2);
    strcat(buf, "\134C");
    lcd.setCursor(0, 4);
    lcd.print(buf);
    lcd.clearRow();
}

char *ftoa(char *a, double f, byte precision)
{
    long p[] = {0,10,100,1000,10000,100000,1000000,10000000,100000000};

    char *ret = a;
    long heiltal = (long)f;
    itoa(heiltal, a, 10);
    while(*a != '\0') a++;
    *a++ = '.';
    long desimal = abs((long)((f - heiltal) * p[precision]));
    byte pp = precision - 1;
    while(pp > 0 && desimal < p[pp]) {
        *a++ = '0';
        pp--;
    }
    itoa(desimal, a, 10);
    return ret;
}

