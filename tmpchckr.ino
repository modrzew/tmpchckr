#include <LiquidCrystal.h>
#include <math.h>
#include <EEPROM.h>

// Constants set by user
const int memoryLength = 1020; // please substract 4 bytes from what's available for safety reasons, kthx
const int saveEvery = 300; // save every X seconds
const byte lightChangeThreshold = 20; // percentage
const byte clearSignalPin = 6;
const byte readMemoryPin = 7;

// Other variables
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
int seconds = 0;
int lightLevelsCount = 10;
int lightLevels[10];
int pointer = 0; // pointer to EEPROM byte
boolean memoryDepleted = false;


void setup() {
    lcd.begin(16, 2);
    setupLCD();
    Serial.begin(9600);
    for (int i=0; i<lightLevelsCount; i++) {
        lightLevels[i] = -999;
    }
    pinMode(clearSignalPin, OUTPUT);
    pinMode(readMemoryPin, OUTPUT);
    // Read pointer value
    pointer = EEPROMReadInt(0);
}


void loop() {
    // Handle input
    int clearSignal = digitalRead(clearSignalPin);
    int readMemorySignal = digitalRead(readMemoryPin);
    String command;
    if(Serial.available() > 0) {
        command = Serial.readString();
        command.trim();
    }
    if(clearSignal == HIGH) {
        reset(false);
    } else if (command.equals("CLEAR")) {
        reset(true);
    } else if (readMemorySignal == HIGH || command.equals("READ")) {
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
    int temperature = (int)(round(3.3 * analogRead(2)));
    lcd.setCursor(8, 0);
    lcd.print(round(temperature/10));
    lcd.setCursor(11, 0);
    lcd.print(temperature%10);
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
        lightLevels[0] = lightLevel;
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
    if (minutes < 10) {
        lcd.print(" ");
    }
    lcd.print(minutes);
    lcd.print(":");
    if (seconds < 10) {
        lcd.print("0");
    }
    lcd.print(seconds);
}

/**
 * Indicates whether values should be stored in memory
 */
boolean shouldSave(int light, int temperature) {
    // Beginning?
    if (pointer == 2) {
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
    // Pointer
    EEPROMWriteInt(0, pointer);
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
void reset(boolean serial) {
    int i, clearSignal;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Clearing memory");
    lcd.setCursor(0, 1);
    if(serial) {
        lcd.print("Proceed?");
        while(Serial.available() <= 0) {
            delay(100);
        }
        String command = Serial.readString();
        command.trim();
        if(!command.equals("YES")) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Aborting.");
            delay(2000);
            setupLCD();
            return;
        }
    } else {
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
    }
    lcd.setCursor(0, 1);
    lcd.print("Clearing...");
    for (i=0; i<memoryLength; i++) {
        EEPROM.write(i, 255);
    }
    // Next loop should save
    memoryDepleted = false;
    pointer = 2;
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
    delay(100);
    howMany = EEPROMReadInt(i);
    for(i+=2; i<howMany; i++) {
        Serial.print(EEPROM.read(i));
        if(i != howMany - 1) {
            Serial.print(",");
        }
        delay(10);
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


//This function will write a 2 byte integer to the eeprom at the specified address and address + 1
void EEPROMWriteInt(int p_address, int p_value)
{
    byte lowByte = ((p_value >> 0) & 0xFF);
    byte highByte = ((p_value >> 8) & 0xFF);

    EEPROM.write(p_address, lowByte);
    EEPROM.write(p_address + 1, highByte);
}

//This function will read a 2 byte integer from the eeprom at the specified address and address + 1
unsigned int EEPROMReadInt(int p_address)
{
    byte lowByte = EEPROM.read(p_address);
    byte highByte = EEPROM.read(p_address + 1);

    return ((lowByte << 0) & 0xFF) + ((highByte << 8) & 0xFF00);
}
