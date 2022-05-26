#include <Keypad.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

#define LEDR 4
#define LEDG 3
#define LEDB 6
#define PIRS 2
#define SERVO_LOCK_POS  11
#define SERVO_UNLOCK_POS 101
#define SERVO 8
#define BUZZER 7

#define NOTE_E5  659
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_G5  784
#define NOTE_GS4 415
#define NOTE_G4  392
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_AS5 932
#define NOTE_A5  880
#define NOTE_D3  147
#define NOTE_C4  262
#define NOTE_C3  131
#define NOTE_C2  65
#define NOTE_A4  440
#define NOTE_B4  494

const byte numRows = 4;
const byte numCols = 4;

char keymap[numRows][numCols] =
{
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[numRows] = {A0, A1, A2, A3};
byte colPins[numCols] = {13, 12, 11, 10};

Keypad keypad = Keypad(makeKeymap(keymap), rowPins, colPins, numRows, numCols);
LiquidCrystal_I2C lcd(0x27, 20, 4);
Servo servo;

unsigned long interval = 300000;
int calibrationTime = 30;
unsigned long lastDetectTime;

bool locked = false;
bool stateChanged = true;
bool motionDetected = true;

char PIN[5] = "0000";

void lock() {
  servo.write(SERVO_LOCK_POS);
}

void unlock() {
  servo.write(SERVO_UNLOCK_POS);
}

void setup()
{
  /*
    for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
    }
  */

  Serial.begin(9600);

  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);

  pinMode(BUZZER, OUTPUT);

  pinMode(PIRS, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIRS), interrupt_routine, HIGH);

  lcd.init();
  lcd.backlight();

  servo.attach(SERVO);

  if (EEPROM.read(34) == '1') {
    for (int j = 0; j < 5; ++j) {
      PIN[j] = EEPROM.read(j + 30);
    }

    char state = EEPROM.read(35);
    if (state == 'L') {
      locked = true;
      lock();
    } else {
      locked = false;
      unlock();
    }
  }

  lcd.setCursor(0, 0);
  lcd.print("Safe is starting");

  delay(calibrationTime * 1000);

  lcd.clear();

  lastDetectTime = millis();
}

void interrupt_routine() {
  Serial.println("INTERRUPTION");

  motionDetected = true;
  lastDetectTime = millis();
}

void playSong(int *song) {
  for (int j = 0; j < sizeof(song) / sizeof(song[0]); ++j) {
    tone(BUZZER, song[j]);

    delay(200);

    noTone(BUZZER);
  }
}

void lightLED(int R, int G, int B) {
  digitalWrite(LEDR, R);
  digitalWrite(LEDG, G);
  digitalWrite(LEDB, B);
}

void sleep() {
  motionDetected = false;

  lightLED(LOW, LOW, LOW);

  lcd.clear();
  lcd.noBacklight();
}

void wakeUp() {
  lcd.backlight();
  if (!locked) {
    stateChanged = true;
  }

  lightLED(HIGH, HIGH, HIGH);
}

void menu() {
  lcd.setCursor(0, 0);
  lcd.print("* - Lock");

  lcd.setCursor(0, 1);
  lcd.print("# - PIN change");

  stateChanged = false;
}

void lockSafe() {
  lock();
  locked = true;
  EEPROM.write(35, 'L');

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Locking...");

  delay(2000);

  stateChanged = false;
  lcd.clear();
  lastDetectTime = millis();
}

bool readPIN(char *newPIN) {
  int j = 0;
  char key;

  while (j < 4) {
    key = keypad.getKey();

    if (key != NO_KEY && key != '*' && key != '#') {
      newPIN[j++] = key;
      lcd.print('*');
    }

    if (key != NO_KEY) {
      lastDetectTime = millis();
    }

    if (millis() - lastDetectTime >= interval) {
      return false;
    }

    key = NO_KEY;
  }

  return true;
}

void changeSafePIN() {
  char newPIN[5];
  bool execute = true;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter new PIN");
  lcd.setCursor(0, 1);

  execute = readPIN(newPIN);

  if (execute) {
    delay(500);

    EEPROM.write(34, '1');

    strncpy(PIN, newPIN, 4);

    for (int j = 0; j < 5; ++j) {
      EEPROM.write(j + 30, PIN[j]);
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PIN changed");

    stateChanged = true;

    int song[] = {NOTE_D5, NOTE_A4, NOTE_B4, NOTE_A4, NOTE_C4, NOTE_D5, NOTE_A5};
    playSong(song);

    delay(500);

    lcd.clear();
    lastDetectTime = millis();
  }
}

void unlockSafe() {
  char newPIN[5];
  bool execute = true;

  lcd.setCursor(0, 0);
  lcd.print("Enter PIN");
  lcd.setCursor(0, 1);

  execute = readPIN(newPIN);

  if (execute) {
    lcd.clear();
    if (!strncmp(PIN, newPIN, 4)) {
      unlock();
      locked = false;
      stateChanged = true;
      EEPROM.write(35, 'U');

      lcd.setCursor(0, 0);
      lcd.print("Acces granted");
      lcd.setCursor(0, 1);
      lcd.print("Unlocking...");

      lightLED(LOW, HIGH, LOW);

      int song[] = {NOTE_A5, NOTE_A5, NOTE_AS5, NOTE_A5, NOTE_G5, NOTE_G5};
      playSong(song);
    } else {
      stateChanged = false;

      lcd.setCursor(0, 0);
      lcd.print("Acces denied");
      lcd.setCursor(0, 1);
      lcd.print("Invalid PIN");

      lightLED(HIGH, LOW, LOW);

      int song[] = {NOTE_GS4, NOTE_G4, NOTE_D4, NOTE_E4, NOTE_GS4, NOTE_G4, NOTE_D4};
      playSong(song);
    }

    delay(500);

    lightLED(HIGH, HIGH, HIGH);

    lcd.clear();
    lastDetectTime = millis();
  }
}

void loop()
{
  char key = keypad.getKey();
  if (key != NO_KEY) {
    motionDetected = true;
    lastDetectTime = millis();
  }

  key = NO_KEY;

  if (millis() - lastDetectTime >= interval) {
    sleep();
  } else {
    if (motionDetected) {
      wakeUp();
    }

    if (!locked && stateChanged) {
      menu();
    }

    key = keypad.getKey();
    if (key != NO_KEY && key == '*' && !locked) {
      lockSafe();
    } else if (key != NO_KEY && key == '#' && !locked) {
      changeSafePIN();
    } else if (locked) {
      unlockSafe();
    }
  }
}
