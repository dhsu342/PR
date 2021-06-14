#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <Keypad.h>
#include "Adafruit_NFCShield_I2C.h"

// LCD Messages //
#define MSG_READINGNFC "NFC:            "
#define MSG_READINGPAD "PAD( ):             "
#define MSG_BLANK "                "

#define MSG_INIT_UP "-- SmartPany  --"
#define MSG_INIT_DOWN "--- WELCOME! ---"
#define MSG_INIT_DELAY 3000

#define MSG_DOOR_CLOSED "-  Door Close  -"
#define MSG_DOOR_OPEN "-  Door Open!  -"

#define MSG_ENTER_CODE_1 "Enter new code: "
#define MSG_ENTER_CODE_2 "Reenter the code"

#define MSG_CODE_CHANGED_1 "Code successfull"
#define MSG_CODE_CHANGED_2 "    changed!    "
#define MSG_CODE_CHANGED_ERROR_1 "   ERR  CODES   "
#define MSG_CODE_CHANGED_ERROR_2 "   NOT  MACTH   "

#define CHAR_CLOSE 0
#define CHAR_OPEN 1

// KeyCode //
#define KEYPAD_CODE_LENGTH 5
char goodCode[] = "12345";

// Door control //
#define DOOR_OPEN true
#define DOOR_CLOSE false
#define SERVO_OPEN 2
#define SERVO_CLOSE 178

// Control variables //
bool readNFC = true;
bool readKeypad = true;
bool doorIsOpen = false; // state of the door
bool lNFC = true; // to check if it's nessary to check nfc

// Servo motor //
Servo myservo;

// NFC - RFID Module //
String lastNfc = ""; //String se le pasa por REFERENCIA a la funcion
#define IRQ (2)
#define RESET (3)
#define NFCTIMEOUT 10000
uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer to store the returned UID
uint8_t uidLength;                     // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
Adafruit_NFCShield_I2C nfc(IRQ, RESET);

// lcd i2c
byte lockClose[] = {
    0x0E,
    0x11,
    0x11,
    0x11,
    0x1F,
    0x1B,
    0x1B,
    0x1F};
byte lockOpen[] = {
    0x0E,
    0x11,
    0x10,
    0x10,
    0x1F,
    0x1B,
    0x1B,
    0x1F};
LiquidCrystal_I2C lcd(0x27, 16, 2);

// KEYPAD
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
const char hexaKeys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
byte rowPins[ROWS] = {39, 41, 43, 45}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {47, 49, 51, 53}; //connect to the column pinouts of the keypad

// Init KeyPad //
Keypad myKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
char kpCodeRead[1];

// AUX func //
void copyArray(char* src, char* dst, int len) { // change de destiantion array to copy from the source array
    for (int i = 0; i < len; i++) {
        *dst++ = *src++;
    }
}
bool equalStrings(char *a, char *b){// Check if two arrays are equal 

  Serial.println(a);
  Serial.println(b);
  
  for (int i = 0; i < (sizeof(a) / sizeof(a[0])) + 1; i = i + 1)
  {
    if (a[i] != b[i])
    {
      return false;
    }
  }
  return true;
}

// Write Func //

void door(bool openDoor = DOOR_CLOSE)
{
  if (openDoor)
  {
    myservo.write(SERVO_OPEN);
    doorIsOpen = true;
  }
  else
  {
    myservo.write(SERVO_CLOSE);
    doorIsOpen = false;
  }
}

void lcdPrint(String text, int row = 0, int col = 0)
{
  lcd.setCursor(col, row);
  lcd.print(text);
}

void lcdPrintChar(int character, int row = 0, int col = 0)
{
  lcd.setCursor(col, row);
  lcd.write(character);
}

// Read Func //

bool cardRead(){
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)    
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength,100);  
  if (success) {
    // Display that card found
    Serial.println("Found an ISO14443A card");
    for (uint8_t i=0; i < uidLength; i++) 
    {
      Serial.print(" 0x");Serial.print(uid[i], HEX); 
    }
    Serial.println("");
    return true;
    }
    return false;
}

void keyPadReadKey(char *result, int numChars, int firstOne=0)
{
  int count = firstOne;
  //result [0] = firstOne;
  while (count <= numChars)
  {
    char customKey = myKeypad.getKey();
    if (customKey)
    {
      result[count] = customKey;
      count += 1;
      lcdPrint(result, 1, 0);
    }
  }
  //result[numChars+1] = '\0';
}


// Global Func //

void changeCode(){

  int numchars = KEYPAD_CODE_LENGTH -1 ;
  // ask for new code and save
  lcdPrint(MSG_ENTER_CODE_1, 0, 0);
  char code1[KEYPAD_CODE_LENGTH+1] = "00000";
  lcdPrint(code1, 1, 0);
  keyPadReadKey(code1, numchars);
  Serial.println(code1);
  
  // ask for code again to confirm

  lcdPrint(MSG_ENTER_CODE_2, 0, 0);
  char code2[KEYPAD_CODE_LENGTH + 1]="00000";
  lcdPrint(code2, 1, 0);
  keyPadReadKey(code2, numchars);
  Serial.println(code2);

  // if match, change code and retor
  // else return to "open" state
if (equalStrings(code1, code2))
  {
    copyArray(code1, goodCode,KEYPAD_CODE_LENGTH+1);
    Serial.println(goodCode);
    lcdPrint(MSG_CODE_CHANGED_1, 0, 0);
    lcdPrint(MSG_CODE_CHANGED_2, 1, 0);
    delay(2000);

  }else{
    lcdPrint(MSG_CODE_CHANGED_ERROR_1, 0, 0);
    lcdPrint(MSG_CODE_CHANGED_ERROR_2, 1, 0);
    delay(2000);
  }
}


void waitUserInputToOpen()
{
  while (true)
  {
    char firstKey = myKeypad.getKey();
    if (firstKey)
    {

      int numchars = KEYPAD_CODE_LENGTH - 1; // we have the fisrt one
      //kpCodeRead[numchars + 1]; // RE-defino un array de char de cuantos +1 para meter el \0
      kpCodeRead[KEYPAD_CODE_LENGTH + 1];
      char kpCodeRead[] = "00000";
      kpCodeRead[0] = firstKey;
      lcdPrint(kpCodeRead, 1, 0);
      int firstCharacterToAdd = 1;
      keyPadReadKey(kpCodeRead, numchars, firstCharacterToAdd);
      Serial.println(kpCodeRead);
      if (equalStrings(kpCodeRead, goodCode))
      {
        door(DOOR_OPEN);
        break;
      }
    }
    else
    {

      if (cardRead()){
        door(DOOR_OPEN);
        delay(1000);
        break;
      }
      /* lNFC = true;

      leer_NFC();
      if (lastNfc != "")
      {
        lastNfc = "";
        door(DOOR_OPEN);
        lNFC = false;
        break;
      } */
    }
  }
  delay(200);
}


void waitUserInputToClose()
{

  while (true)
  {
    char firstKey = myKeypad.getKey();
    if (firstKey)
    {
      if (firstKey=='*'){

        //quieren cambiar de codigo
        //LLamar a la funcion de cambio
        changeCode();

        // salir de aqui

              break;


      }
      int numchars = KEYPAD_CODE_LENGTH - 1; // we have the fisrt one
      //kpCodeRead[numchars + 1]; // RE-defino un array de char de cuantos +1 para meter el \0
      kpCodeRead[KEYPAD_CODE_LENGTH + 1];
      char kpCodeRead[] = "00000";
      kpCodeRead[0] = firstKey;
      lcdPrint(kpCodeRead, 1, 0);
      int firstCharacterToAdd = 1;
      keyPadReadKey(kpCodeRead, numchars, firstCharacterToAdd);
      Serial.println(kpCodeRead);
      if (equalStrings(kpCodeRead, goodCode))
      {
        door(DOOR_CLOSE);
        break;
      }
    }
    else
    {
      if (cardRead()){
        door(DOOR_CLOSE);
        delay(1000);
        break;
      }
    }
  }
  delay(200);
}

// MAIN //

void setup()
{
  Serial.begin(115200);
  // SERVO //
  myservo.attach(8);
  myservo.write(SERVO_CLOSE);
  // NFC //
  delay(50);
  nfc.begin();
  nfc.setPassiveActivationRetries(0x01);
  nfc.SAMConfig();
  // LCD //
  lcd.init(); // initialize the lcd
  lcd.backlight();
  lcd.createChar(CHAR_CLOSE, lockClose);
  lcd.createChar(CHAR_OPEN, lockOpen);
  lcd.clear();
  // WELCOME MESSAGE //
  lcdPrint(MSG_INIT_UP);
  lcdPrint(MSG_INIT_DOWN, 1);
  lcdPrintChar(CHAR_CLOSE, 0, 12);
  delay(MSG_INIT_DELAY);
}

void loop()
{
  if (doorIsOpen)
  {
    lcdPrint(MSG_DOOR_OPEN, 0, 0); //-  Door Close  -
    lcdPrintChar(CHAR_OPEN,0,2);
    lcdPrintChar(CHAR_OPEN,0,15);
    lcdPrint(MSG_BLANK, 1);
    waitUserInputToClose();
  }
  else
  {

    lcdPrint(MSG_DOOR_CLOSED, 0, 0); // -  Door Open!  -
    lcdPrintChar(CHAR_CLOSE,0,2);
    lcdPrintChar(CHAR_CLOSE,0,15);
    lcdPrint(MSG_BLANK, 1);
    waitUserInputToOpen();
  }
}
