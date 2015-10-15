#include <LiquidCrystal.h>
#include <math.h>
#include <EEPROM.h>

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
int minutes = 0, seconds = 0;
int lastLightLevel = -999, currentLightLevel = 0;
int currentTemperature = -999;
long pointer = 0; // pointer to EEPROM byte

void setup() {
    lcd.begin(16, 2);
    lcd.setCursor(0, 0);
    lcd.print("L   % ");
    lcd.setCursor(6, 0);
    lcd.print("T    C");
    lcd.setCursor(0, 1);
    lcd.print("PNT ");
    lcd.setCursor(8, 1);
    lcd.print("TM ");
    Serial.begin(9600);
}

void loop() {
    // EEPROM depleted?
    if (pointer >= 1020) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("EEPROM depleted");
        lcd.setCursor(0, 1);
        lcd.print("Last value: ");
        lcd.print(pointer);
        delay(10000);
        return;
    }
    // Light
    currentLightLevel = analogRead(1) / 10;
    lcd.setCursor(2, 0);
    lcd.print(currentLightLevel);
    // Temperature
    analogRead(2);
    delay(100);
    currentTemperature = (int)(round(5.0 * analogRead(2) - 100));
    lcd.setCursor(8, 0);
    lcd.print(round(currentTemperature/10 + 10));
    // Minutes, pointer
    lcd.setCursor(4, 1);
    lcd.print(pointer);
    lcd.setCursor(11, 1);
    lcd.print(minutes);
    lcd.setCursor(13, 1);
    lcd.print(":");
    lcd.print(seconds);
    tick();
    if (shouldSave()) {
        save();
    }
    lastLightLevel = currentLightLevel;
    delay(1000);
}


void tick() {
    seconds++;
    if(seconds >= 60) {
        minutes++;
        seconds = 0;
    }
}

boolean shouldSave() {
    // Beginning?
    if (lastLightLevel == -999) {
        return true;
    }
    // Too old result?
    if (minutes > 30) {
        return true;
    }
    // Light changed?
    if (lastLightLevel >= 0 && abs(currentLightLevel - lastLightLevel) > 35) {
        return true;
    }
    return false;
}

boolean save() {
    // Minutes since last save
    EEPROM.write(pointer, minutes);
    pointer++;
    // Light
    EEPROM.write(pointer, currentLightLevel);
    pointer++;
    // Temperature
    EEPROM.write(pointer, currentTemperature);
    pointer++;
    minutes = 0;
    seconds = 0;
}