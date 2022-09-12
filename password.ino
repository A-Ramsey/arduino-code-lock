#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <Keypad.h>

#define CONTRAST_PIN A5//WAS 6
#define RED_PIN A4
#define GREEN_PIN A3
#define BUZZER_PIN A0

//pins 0 and 1 can NOT be used on lcd display
const int rs = 5, en = 4, d4 = 3, d5 = 2, d6 = A1, d7 = A2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const byte ROWS = 4; 
const byte COLS = 4; 

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte upArrow[8] = {
  B00000,
  B00000,
  B00100,
  B01010,
  B10001,
  B00000,
  B00000,
};
byte downArrow[8] = {
  B00000,
  B00000,
  B10001,
  B01010,
  B00100,
  B00000,
  B00000,
};

byte rowPins[ROWS] = {13, 12, 11, 10}; 
byte colPins[COLS] = {9, 8, 7, 6};

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

String password;
String inpPassword = "";
boolean loggedIn = false;
unsigned long loggedInIntTime;
boolean resetPassword = false;

const int MENU_SIZE = 4;
const char one[] = "Pick An Item";
const char two[] = "Reset Password";
const char three[] = "Log Out";
const char* loggedInMenuText[MENU_SIZE] = {"Pick An Item", "Reset Password", "Log Out", "Show Password"};
int menuScrollPoint = 0;

void buzz(int toneOff = 100, int frq = 1000, int toneOn = 100) {
  tone(BUZZER_PIN, frq);
  delay(toneOn);
  noTone(BUZZER_PIN);
  delay(toneOff);
}

void writeStringToEEPROM(String str, int lenLoc = 1, int startLoc = 2) {
  int strLen = str.length();
  EEPROM.write(lenLoc, strLen);

  for (int i = 0; i < strLen; i++) {
    EEPROM.write(startLoc + i, str[i]);
  }
}

String readStringFromEEPROM(int lenLoc = 1, int startLoc = 2) {
  int strLen = EEPROM.read(lenLoc);

  String str = "";
  for (int i = 0; i < strLen; i++) {
    str += char(EEPROM.read(startLoc + i));
    
  }

  return str;
}

void lcdPassword() {
  lcd.clear();
  lcd.print("Enter Password:");
  lcd.setCursor(0,1);
}

void passwordCorrectActions() {
  loggedIn = true;
  loggedInIntTime = millis();
  inpPassword = "";
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, HIGH);
  lcd.clear();
  lcd.print("Password Correct!");
  delay(2000);
}

void cancelPasswordInp() {
  inpPassword = "";
  lcdPassword();
}

void addCharToPassword(char customKey) {
  inpPassword += customKey;
  updateDisplay();
}

void updateDisplay() {
    lcd.write("*");
}

void loggedInMenuDisplay() {
  //Show a menu
  int line = menuScrollPoint;
  String text = "";
  if (line < MENU_SIZE-1) {
    if (line != 0) {
      text += String(line);
      text += ".";
    }
    text += String(loggedInMenuText[line]);
  }
  line += 1;
  String text1 = "";
  if (line < MENU_SIZE) {
    if (line != 0) {
      text1 += String(line);
      text1 += ".";
    }
    text1 += String(loggedInMenuText[line]);
  }

  lcd.clear();
  lcd.print(text);
  lcd.setCursor(0, 1);
  lcd.print(text1);
}

void logOut() {
  digitalWrite(RED_PIN, HIGH);
  digitalWrite(GREEN_PIN, LOW);
  lcd.clear();
  lcd.print("Logging Out");
  delay(2000);
  loggedIn = false;
  lcdPassword();
  return;
}

void showPassword() {
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print(password);
  lcd.setCursor(0, 0);
  lcd.print("Showing for: ");
  for (int j = 0; j <= 10; j++) {
    lcd.setCursor(13, 0);
    lcd.print(String(10-j) + " ");
    delay(1000);
  }
  return;
}

void loggedInActions() {
  char keyPressed = customKeypad.waitForKey();
  switch (keyPressed) {
    case '1':
      resetPasswordStart();
      break;
    case '2':
      logOut();
      break;
     case '3':
      showPassword();
      return;
    case 'A':
      if (menuScrollPoint > 0) {
        menuScrollPoint-=1;
      } else {
        menuScrollPoint = MENU_SIZE - 2;
      }
      break;
    case 'B':
      //add 2 so we dont show empty space
      if (menuScrollPoint+2 < MENU_SIZE) {
        menuScrollPoint+=1;
      } else {
        menuScrollPoint = 0;
      }
      break;
    default:
      break;
  }
}

void keypadEvent(KeypadEvent key) {
  switch (customKeypad.getState()){
    case PRESSED:
      if (loggedIn) {
        loggedInIntTime = millis();
      }
      buzz(0);
      break;
    case RELEASED:
      break;
  }
}

void passwordInpActions() {
  char customKey = customKeypad.getKey();
  if (customKey){
    //actions for key press
    if (customKey == '*') {
      // * resets inpPassword
      cancelPasswordInp();
      return;
    } else if (customKey == '#' and loggedIn) {
      resetPasswordSave();
      return;
    }

    //ensure the password isnt anly longer than 16 chars
    if (inpPassword.length() == 16) {
      buzz();
      buzz();
      buzz();
    } else {
      addCharToPassword(customKey);
    }

    //in logged out anf the inputted password equals the password log in
    if (!loggedIn) {
      if (inpPassword.equals(password)) {
        passwordCorrectActions();
      }
    }
  }
}

void resetPasswordStart() {
  resetPassword = true;
  lcd.clear();
  lcd.print("New Password:");
  lcd.setCursor(0, 1);
  inpPassword = "";
}

void resetPasswordSave() {
  lcd.clear();
  lcd.print("Password Saved");
  writeStringToEEPROM(inpPassword);
  password = inpPassword;
  inpPassword = "";
  resetPassword = false;
  delay(2000);
}

void setup() {
  password = readStringFromEEPROM();
  analogWrite(CONTRAST_PIN, 50);
  lcd.begin(16, 2);
  pinMode(d6, OUTPUT);
  pinMode(d7, OUTPUT);
  lcd.createChar(0, upArrow);
  lcd.createChar(1, downArrow);
  lcdPassword();
  
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(RED_PIN, HIGH);

  customKeypad.addEventListener(keypadEvent);
  Serial.begin(9600);
}
  
void loop(){
  if (loggedIn) {
    if (resetPassword) {
      passwordInpActions();
    } else {
      loggedInMenuDisplay();
      loggedInActions();
    }
  } else {
    passwordInpActions();
  }
}
