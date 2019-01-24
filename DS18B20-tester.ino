// DS18B20 tester for Arduino (with optional LCD from Nokia 5110)
// written by Petr Stehlik in December 2018
// GPL v3
//
// Needs 3 resistors:
// 1) connect resistor 270 Ohm (220 - 470 is OK) between pins A0 and A1
// 2) connect resistor 270 Ohm (220 - 470 is OK) between pins A2 and A3
// 3) connect resistor 2k2 (2k2 - 5k6 will do) between pin A3 and +5 V
//
// Connect the tested sensor as follows:
// - pin 1 of DS18B20 (GND) to GND of Arduino
// - pin 2 of DS18B20 (Data) to pin A2 of Arduino
// - pin 3 of DS18B20 (Vdd) to pin A0 of Arduino
//

#define HAVE_DISPLAY    false

#include <OneWire.h>
#include <DallasTemperature.h>

#if HAVE_DISPLAY
# include "Joy_LCD5110.h"
Joy_LCD5110 lcd(/*SCLK*/6, /*Din*/7, /*DC*/8, /*RESET*/ 9);  // display from Nokia 5110
# define lcd_setCursor(c, r)  lcd.setCursor(c, r)
# define lcd_setInverse(x)    lcd.setInverse(x)
# define lcd_clearRow()       lcd.clearRow()
# define lcd_print(x)         lcd.print(x)
# define lcd_printh(x)        lcd.print(x, HEX)
# define lcd_printat(c, r, x) { lcd.setCursor(c, r); lcd.print(x); lcd.clearRow(); }
#else
# define lcd_setCursor(c, r)
# define lcd_setInverse(x)
# define lcd_clearRow()       Serial.println()
# define lcd_print(x)         Serial.print(x)
# define lcd_printh(x)        Serial.print(x, HEX)
# define lcd_printat(c, r, x) Serial.println(x)
#endif

#define ONEWIRE_PIN   A3
#define ONEWIRE_SENSE A2
#define POWER_PIN     A1
#define POWER_SENSE   A0

OneWire oneWire(ONEWIRE_PIN);
DallasTemperature sensors(&oneWire);

// forward declarations are not required in Arduino IDE
// char *ftoa(char *a, double f, byte precision);
// void printTemperature(float value);
// void printDeviceAddress(void);

void lcd_header()
{
#if HAVE_DISPLAY
    lcd.setInverse(false);
    lcd.clearScreen();
    lcd.setTextSize(1);
    lcd.setInverse(true);
    lcd.print("DS18B20 tester");
    lcd.clearRow();
    lcd.setInverse(false);
#endif
}

void setup(void)
{
    pinMode(POWER_SENSE, INPUT);
    pinMode(ONEWIRE_SENSE, INPUT);
    pinMode(POWER_PIN, OUTPUT);
    digitalWrite(POWER_PIN, LOW);
#if !HAVE_DISPLAY
    Serial.begin(115200);
#endif
}

void loop(void)
{
    lcd_header();

    digitalWrite(POWER_PIN, HIGH);
    delay(100);

    // power short circuit test
    unsigned u = analogRead(POWER_SENSE);
    lcd_printat(0, 5, u);
    if (u >= 1020) {
        // power is OK
    }
    else if (u > 969) {
        lcd_printat(0, 1, "Vdd high curr.");
    }
    else {
        lcd_printat(0, 1, "Vdd short circ");
        delay(5000);
    }

    // data short circuit test
    u = analogRead(ONEWIRE_SENSE);
    lcd_printat(6*6, 5, u);
    if (u >= 1022) {
        // 1wire bus is OK
    }
    else if (u > 969) {
        lcd_printat(0, 2, "Bus high curr.");
    }
    else {
        lcd_printat(0, 2, "Bus short circ");
        delay(5000);
    }

    sensors.begin();

    if (sensors.getDeviceCount() > 0) {

        // test of parasitic power mode
        digitalWrite(POWER_PIN, LOW);
        delay(100);

        sensors.begin();

        if (sensors.getDeviceCount() == 0) {
            lcd_setInverse(true);
            lcd_printat(0, 3, "Parasite fail!");
            delay(5000);
            return;
        }

        printDeviceAddress();
        sensors.requestTemperatures();

        float tempC = sensors.getTempCByIndex(0);
        printTemperature(tempC);
        delay(2500);

        lcd_printat(0, 1, "Family test");
        for(byte cnt = 0; cnt < 20; cnt++) {
            lcd_printat(12*6, 1, cnt);
            test_family();
        }
    }
    else {
        lcd_printat(0, 3, "  No sensor");
    }
    delay(500);
}

void dead_end()
{
    lcd_setInverse(true);
    lcd_printat(0, 5, "Reset to cont");
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
        lcd_printat(0, 3, "No Sensor!");
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
        lcd_printat(0, 3, "ID N/A");
        dead_end();
    }

    if (deviceAddress[0] != 0x28) {
        lcd_printat(0, 3, "Family error");
        dead_end();
    }

    lcd_setCursor(0, 3);
    for (uint8_t i = 1; i < 7; i++) {
        if (deviceAddress[i] < 16) lcd_print("0");
        lcd_printh(deviceAddress[i]);
    }
    lcd_clearRow();
}

// function to print the temperature for a device
void printTemperature(float tempC)
{
    char buf[15];
    ftoa(buf, tempC, 2);
    strcat(buf, "\134C");
    lcd_printat(0, 4, buf);
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
