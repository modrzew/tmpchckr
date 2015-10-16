#include <LiquidCrystal.h>
#include <math.h>
#include <EEPROM.h>

// Constants set by user
const int memoryLength = 1020; // please substract 4 bytes from what's available for safety reasons, kthx
const int saveEvery = 1800; // save every X seconds
const byte lightChangeThreshold = 35; // percentage
const byte clearSignalPin = 6;
const byte readMemoryPin = 7;

// Other variables
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
int seconds = 0;
int lightLevels;
int lightLevelsCount = 10;
int pointer = 0; // pointer to EEPROM byte
boolean memoryDepleted = false;


void setup() {
    lcd.begin(16, 2);
    setupLCD();
    Serial.begin(9600);
    lightLevels = new int[lightLevelsCount];
    for (int i=0; i<lightLevelsCount; i++) {
        lightLevels[i] = -999;
    }
}


void loop() {
    // Handle input
    int clearSignal = digitalRead(clearSignalPin);
    int readMemorySignal = digitalRead(readMemoryPin);
    if(clearSignal == HIGH) {
        reset();
    } else if (readMemorySignal == HIGH) {
        readMemory();
    }
    // EEPROM depleted?
    if (memoryDepleted) {
        delay(1000);
        return;
    } else if (isMemoryDepleted()) {
        handleMemoryDepletion();
        delay(1000);
        return;
    }

    // Light
    int lightLevel = analogRead(1) / 10;
    lcd.setCursor(2, 0);
    lcd.print(lightLevel);
    // Temperature
    analogRead(2);
    delay(100);
    temperature temperature = (int)(round(5.0 * analogRead(2)));
    lcd.setCursor(8, 0);
    lcd.print(round(temperature/10));
    // Minutes, pointer
    lcd.setCursor(4, 1);
    lcd.print(pointer/3);
    displayTime(seconds, 1, 12);

    // Finish up
    tick();
    if (shouldSave(lightLevel, temperature)) {
        save(lightLevel, temperature);
    } else {
        // Shift light levels
        for (int i=1; i<lightLevelsCount; i++) {
            lightLevels[i] = lightLevels[i-1];
        }
        lightLevels[i] = lightLevel;
    }
    delay(1000);
}


/**
 * Sets up LCD screen for displaying standard information
 * @return {[type]} [description]
 */
void setupLCD() {
    lcd.clear();
    // Light
    lcd.setCursor(0, 0);
    lcd.print("L   %");
    // Temperature
    lcd.setCursor(6, 0);
    lcd.print("T   . 'C");
    // Stored values counter
    lcd.setCursor(0, 1);
    lcd.print("CNT ");
}


/**
 * Adds values to internal timer
 */
void tick() {
    seconds++;
}

/**
 * Displays given time on LCD at given point
 */
void displayTime(int seconds, int row, int column) {
    int minutes = seconds / 60;
    seconds = seconds % 60;
    lcd.setCursor(11, 1);
    // Too few minutes?
    if (minutes == 0) {
        lcd.print(" ");
    }
    lcd.print(minutes);
    lcd.print(":");
    if (seconds == 0) {
        lcd.print("0");
    }
    lcd.print(seconds);
}

/**
 * Indicates whether values should be stored in memory
 */
boolean shouldSave(int light, int temperature) {
    // Beginning?
    if (pointer == 0) {
        return true;
    }
    // Too old result?
    if (seconds > saveEvery) {
        return true;
    }
    // Light changed in previous readings?
    for (int i=0; i<lightLevelsCount; i++) {
        if (lightLevels[i] >= 0 && abs(light - lightLevels[i]) > lightChangeThreshold) {
            return true;
        }
    }
    return false;
}


/**
 * Stores current values in EEPROM
 */
void save(int light, int temperature) {
    int minutes = seconds / 60;
    // Minutes since last save
    EEPROM.write(pointer, (byte)minutes);
    pointer++;
    // Light
    EEPROM.write(pointer, (byte)light);
    pointer++;
    // Temperature
    EEPROM.write(pointer, (byte)(temperature - 100)); // -100, because we're storing values from 10"C to 35"C in memory
    pointer++;
    seconds = 0;
    // Fill lightLevels with current value
    for (int i=0; i<lightLevelsCount; i++) {
        lightLevels[i] = light;
    }
}


/**
 * Clears EEPROM
 *
 * Displays warning for 5 seconds, giving user ability to back off.
 * After that, all values in EEPROM will be set to 255.
 */
void reset() {
    int i, clearSignal;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Clearing memory");
    lcd.setCursor(0, 1);
    lcd.print("Countdown: ");
    for (i=5; i>=0; i++) {
        lcd.setCursor(11, 1);
        lcd.print(i);
        delay(1000);
        clearSignal = digitalRead(clearSignalPin);
        if (clearSignal == LOW) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Aborting.");
            delay(2000);
            setupLCD();
            return;
        }
    }
    int percent, lastPercent;
    lcd.setCursor(0, 1);
    lcd.print("Progress:   ");
    for (i=0; i<memoryLength; i++) {
        EEPROM.write(i, 255);
        percent = (i / memoryLength);
        if (percent != lastPercent) {
            lcd.setCursor(10, 1);
            lcd.print(percent);
            lcd.print("%");
            lastPercent = percent;
        }
    }
    // Next loop should save
    memoryDepleted = false;
    pointer = 0;
    setupLCD();
}


/**
 * Iterates over EEPROM and writes it on Serial output
 */
void readMemory() {
    int i = 0, howMany;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Reading memory");
    howMany = EEPROM.read(i);
    for(i++; i<howMany*3; i++) {
        Serial.write(EEPROM.read(i));
    }
    // Clean up
    if(!memoryDepleted) {
        setupLCD();
    }
}


/**
 * Indicates whether memory has been depleted.
 *
 * Also changes value of memoryDepleted variable.
 */
boolean isMemoryDepleted() {
    memoryDepleted = pointer >= memoryLength;
    return memoryDepleted;
}


/**
 * Display friendly notice to the user that this program won't go further
 */
void handleMemoryDepletion() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Memory depleted");
    lcd.setCursor(0, 1);
    lcd.print("Readings: ");
    lcd.print(pointer/3);
    delay(1000);
}
