#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "RTClib.h"
#include <Keypad.h>
#include "DFRobotDFPlayerMini.h"
#include <SPI.h>
#include "IRremote.h"


//LED Strip
#define RED_LED 6
#define BLUE_LED 5
#define GREEN_LED 7
const float maxBrightness = 255.00;
float gSetPoint = 30.00;
float rSetPoint = 255.00;
float bSetPoint = 0.00;
float gBrightness;
float rBrightness;
float bBrightness;

//IR module
const int receiver = 33;
IRrecv irrecv(receiver);
decode_results results;


//OLED module
U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 53, /* dc=*/ 2, /* reset=*/ 48);


//RTC module
RTC_DS1307 RTC;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
uint8_t fixedMinutes;
uint8_t fixedHours;
uint8_t fixedSeconds;
int fixedYear;
int fixedMonth;
int fixedDay;
char fixedDayOfWeek;
bool meridiem;


//Keypad module
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
}; // keypad grid
byte rowPins[ROWS] = {22, 23, 24, 25}; //pin assignments
byte colPins[COLS] = {26, 27, 28, 29}; //pin assignments
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS);
String keyInput;
String remoteInput;


//MP3 DFPlayer mini
DFRobotDFPlayerMini myDFPlayer;
#define DFPLAYER_PIN_TX 19
#define DFPLAYER_PIN_RX 18
int randomSong;
int busyState = 0;
int skipState = 0;


const int busyPin = 8;
const int skipButton = 30;
const int snoozeButton = 31;
const int snoozeLightPin = 32;
const int volumeUpButton = 34;
const int volumeDownButton = 35;
//const int alarmLightPin = 33;


//miscellaneous global variables
unsigned long startMillis;
unsigned long currentMillis;
unsigned long IRmillis;
uint8_t processSeconds;
uint8_t currentSecond;
bool displayVolumeState;
const unsigned long period = 1000;
const unsigned long miniPeriod = 500;
const unsigned long longPeriod = 2000;
const unsigned long microPeriod = 100;
unsigned long beginMillis;
bool secondFlash;
bool miniFlash;
uint8_t volume;
bool alarmState;
int pageCount;
int alarmCounter;
int alarmDelayCounter;
int snoozeDelayCounter;
int playlistCounter;
int songAmount;
bool alarmHasStarted;
bool alarmLightHasStarted;
bool playNextSong;
bool snoozeHasStarted;
int snoozeState = 0;
int snoozeStartingPoint;
int alarmStartingPoint;
int snoozeTracker;
int alarmLightTracker; 


//alarm global variables
String alarmHour0;
String alarmHour1;
String alarmMinute0;
String alarmMinute1;
bool alarmMeridiem;
String alarmHourStr;
int alarmHourInt;
String alarmMinuteStr;
int alarmMinuteInt;
int volumeSetting = 0;
int volumeButtonVal;
int volumeVal;
const int volumeDial = 1;
bool volumeUpState;
bool volumeDownState;
int currentVolume;
uint8_t processVolume;
String alarmDelay0;
String alarmDelay1;
String alarmDelayStr;
int alarmDelayInt;
int alarmTimeSeconds;
String snoozeDelay0;
String snoozeDelay1;
String snoozeDelayStr;
int snoozeDelayInt;
int snoozeTimeSeconds;
bool remoteState;


void setup() {
  u8g2.begin();
  u8g2.enableUTF8Print();
  Serial.begin(115200);
  Serial1.begin(9600);
  Serial.println("IR Receiver Button Decode"); 
  irrecv.enableIRIn();
  Wire.begin();
  RTC.begin();
  if (! RTC.isrunning()) {
    u8g2.drawStr(15, 10, "Clock is broken");
  }
  //RTC.adjust(DateTime(2022, 7, 3, 23, 55, 0)); //adjust real time clock (year, month, day, hour, minute, second) - consider that RTC uses military time

  if (myDFPlayer.begin(Serial1)) {
    Serial1.println("OK");
  } else {
    Serial1.println("Connecting to DFPlayer Mini failed!");
  }

  randomSeed(analogRead(0));


//setting variables to default starting values
  startMillis = millis();
  beginMillis = millis();
  IRmillis = millis();
  processSeconds = 0;
  displayVolumeState = false;
  secondFlash = false;
  miniFlash = false;
  alarmState = false;
  pageCount = 0;
  alarmCounter = 0;
  alarmDelayCounter = 0;
  snoozeDelayCounter = 0;
  playlistCounter = 1;
  alarmHour0 = "0";
  alarmHour1 = "6";
  alarmMinute0 = "0";
  alarmMinute1 = "0";
  alarmMeridiem = false;
  alarmDelay0 = "1";
  alarmDelay1 = "0";
  snoozeDelay0 = "0";
  snoozeDelay1 = "3";
  alarmHasStarted = false;
  alarmLightHasStarted = false;
  playNextSong = false;
  snoozeState = false;
  snoozeHasStarted = false;
  snoozeTracker = 0;
  alarmLightTracker = 0;
  remoteInput = "y";
  volumeButtonVal = 12;
  volumeUpState = false;
  volumeDownState = false;
  processVolume = 0;

  pinMode(busyPin, INPUT_PULLUP);
  pinMode(skipButton, INPUT_PULLUP);
  pinMode(snoozeButton, INPUT_PULLUP);
  pinMode(snoozeLightPin, OUTPUT);
  pinMode(volumeUpButton, INPUT_PULLUP);
  pinMode(volumeDownButton, INPUT_PULLUP);


}


void loop() {

  if (irrecv.decode(&results)) {
    translateIR();
    if (currentMillis - IRmillis >= microPeriod) {
      irrecv.resume(); // receive the next value
      IRmillis = currentMillis;
    }
  }
  
//used to call the real time clock function once per second to prevent a constant call
//also used for the colon flash on the display
  currentMillis = millis();
  if (currentMillis - startMillis >= period) {
    realTimeClock();
    startMillis = currentMillis;
    secondFlash = !secondFlash;
    //Serial.println(currentMillis);
  }

  if (currentMillis - beginMillis >= longPeriod) {
    miniFlash = false;
    beginMillis = currentMillis;
  } else if (currentMillis - beginMillis >= miniPeriod) {
    miniFlash = true;
  }


  if (keyInput == "*" || pageCount > 5 || remoteInput == "FUNC/STOP") {
    pageCount = 0;
    remoteInput = "z";
    miniFlash = false;
    beginMillis = currentMillis;
  } else if (keyInput == "D" || remoteInput == "FAST FORWARD") {
    pageCount++;
    remoteInput = "z";
    if (pageCount == 5) {
      miniFlash = true;
      beginMillis = currentMillis - miniPeriod;
    } else {
      miniFlash = false;
      beginMillis = currentMillis;
    }
  } else if (pageCount < 0) {
    pageCount = 5;
    remoteInput = "z";
    miniFlash = true;
    beginMillis = currentMillis - miniPeriod;
  } else if (remoteInput == "FAST BACK" || keyInput == "C") {
    pageCount--;
    remoteInput = "z";
    miniFlash = false;
    beginMillis = currentMillis;
  }

  if (pageCount != 1) {
    alarmCounter = 0;
  } 
  
  if (pageCount != 2) {
    alarmDelayCounter = 0;
  } 
  
  if (pageCount != 3) {
    snoozeDelayCounter = 0;
  }

//read the keypad input
  char key = keypad.getKey();
  keyInput = key;
  alarmLogic();
  displayPage(); //determine the page to display
  convertAlarm(); //convert alarm string values to integers
  convertSnooze(); //convert snooze string values to integers
  convertAlarmDelay(); //convert alarm delay string values to integers
  playlistCheck(); //determine number of songs in each playlist
  checkAlarm(); //check if alarm should start
  endAlarm(); //check to see if alarm should end
  checkSkip(); //check to skip to next song
  checkSnooze(); //check if alarm will be snoozed
  displayAlarmLights(); //sets the brightness for each LED color on the LED strip
  checkVolume(); //changes volume value if set to use buttons
  displayVolume(); //checks if volume changes to determine what to display

  if (volumeSetting == 0) {
    currentVolume = volumeButtonVal;
  } else {
    currentVolume = map(volumeVal, 1, 1024, 1, 30); // scaling of potentiometer reading to use as volume level
  }

  volumeVal = analogRead(volumeDial);
  myDFPlayer.volume(currentVolume);
  busyState = digitalRead(busyPin);
  skipState = digitalRead(skipButton);
  snoozeState = digitalRead(snoozeButton);
  volumeUpState = digitalRead(volumeUpButton);
  volumeDownState = digitalRead(volumeDownButton);

  Serial.print(currentVolume);
  Serial.print(" ");
  Serial.print(processVolume);
  Serial.print(" ");
  Serial.print(processSeconds);
  Serial.print(" ");
  Serial.println(currentSecond);


}


void displayPage() {
  switch(pageCount) {
    case 0:
      callHomePage();
      break;
    case 1:
      callPage1();
      break;
    case 2:
      callPage2();
      break;
    case 3:
      callPage3();
      break;
    case 4:
      callPage4();
      break;
    case 5:
      callPage5();
      break;
  }
}


void callHomePage() {
    u8g2.clearBuffer();
    homePage();
    u8g2.sendBuffer();
}


void callPage1() {
    u8g2.clearBuffer();
    alarmPage();
    u8g2.sendBuffer();
}

void callPage2() {
    u8g2.clearBuffer();
    alarmDelayPage();
    u8g2.sendBuffer();
}

void callPage3() {
    u8g2.clearBuffer();
    snoozeDelayPage();
    u8g2.sendBuffer();
}

void callPage4() {
    u8g2.clearBuffer();
    playlistPage();
    u8g2.sendBuffer();
}

void callPage5() {
    u8g2.clearBuffer();
    volumePage();
    u8g2.sendBuffer();
}


void realTimeClock() {
  DateTime now = RTC.now();
  int hours = now.hour();
  int minutes = now.minute();
  int seconds = now.second();
  int year = now.year();
  int month = now.month();
  int day = now.day();
  fixedMinutes = minutes;
  fixedSeconds = seconds;
  fixedYear = year;
  fixedMonth = month;
  fixedDay = day;
  fixedDayOfWeek = now.dayOfTheWeek();

//RTC library reads the hours 0-23 - this essentially changes the time from military time to standard time
  if (hours > 12) {
    fixedHours = hours - 12;
  } else if (hours == 0) {
    fixedHours = 12;
  } else {
    fixedHours = hours;
  }

//uses the original "military time" to determine the meridiem
  if (hours >= 12) {
    meridiem = true;
  } else {
    meridiem = false;
  }

}

void playlistCheck() {

  switch (playlistCounter) {
    case 1:
      songAmount = 93;
      myDFPlayer.EQ(DFPLAYER_EQ_BASS);
      break;
    case 2:
      songAmount = 79;
      myDFPlayer.EQ(DFPLAYER_EQ_BASS);
      break;
    case 3:
      songAmount = 202;
      myDFPlayer.EQ(DFPLAYER_EQ_ROCK);
      break;
    case 4:
      songAmount = 120;
      myDFPlayer.EQ(DFPLAYER_EQ_ROCK);
      break;
    case 5:
      songAmount = 28;
      myDFPlayer.EQ(DFPLAYER_EQ_POP);
      break;
    case 6:
      songAmount = 2;
      myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
      break;
    case 7:
      songAmount = 85;
      myDFPlayer.EQ(DFPLAYER_EQ_BASS);
      break;
    case 8:
      songAmount = 32;
      myDFPlayer.EQ(DFPLAYER_EQ_BASS);
      break;
    case 9:
      songAmount = 125;
      myDFPlayer.EQ(DFPLAYER_EQ_ROCK);
      break;
  }
} //amount of songs on playlist + 1


void checkSkip() {
  if(skipState == HIGH) {
    myDFPlayer.stop();
    playNextSong = true;
  }
}


void alarmLogic() {
  if (pageCount == 0) {
    if ((keyInput == "A" || (remoteInput == "POWER")) && alarmState == true) {
      alarmState = false;
      remoteInput = "z";
    } else if ((keyInput == "A" || remoteInput == "POWER") && alarmState == false) {
      alarmState = true;
      remoteInput = "z";
    }
  }
}


//next functions set different fonts for the display
void u8g2_7Seg(void) {
  u8g2.setFont(u8g2_font_7_Seg_33x19_mn);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}


void u8g2_Text8(void) {
  u8g2.setFont(u8g2_font_helvB08_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}


void u8g2_Text5(void) {
  u8g2.setFont(u8g2_font_micro_mr);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}


void u8g2_Symbols(void) {
  u8g2.setFont(u8g2_font_unifont_t_symbols);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

void u8g2_smallSymbols() {
  u8g2.setFont(u8g2_font_6x12_t_symbols);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}


//home page display and logic
void homePage() {
  u8g2_7Seg();

  if (fixedHours >= 10) {
    u8g2.setCursor(15, 13);
    u8g2.print(fixedHours);
  } else {
    u8g2.drawStr(15, 13, " ");
    u8g2.setCursor(35, 13);
    u8g2.print(fixedHours);
  }

  if (secondFlash == 1) {
    u8g2.print(":");
    u8g2.drawStr(55, 13, ":");
  } else {
    u8g2.print(" ");
    u8g2.drawStr(55, 13, " ");
  }

  if (fixedMinutes >= 10) {
    u8g2.setCursor(62, 13);
    u8g2.print(fixedMinutes);
  } else {
    u8g2.drawStr(62, 13, "0");
    u8g2.setCursor(82, 13);
    u8g2.print(fixedMinutes);
  }
  
  u8g2_Text8();
  
  if (meridiem == true) {
    u8g2.drawStr(104, 15, "PM");
  } else {
    u8g2.drawStr(104, 15, "AM");
  }

  u8g2.setCursor(12, 53);
  u8g2.print(daysOfTheWeek[fixedDayOfWeek]);
  u8g2.print(", ");  
  u8g2.print(fixedMonth);
  u8g2.print("/");
  u8g2.print(fixedDay);
  u8g2.print("/");
  u8g2.print(fixedYear);

  u8g2_Symbols();
  if (alarmState == true) {
    u8g2.drawUTF8(0, 0, "\u23F0"); //unicode alarm clock symbol
  } else {
    u8g2.drawStr(0, 0, " ");
  }

  //u8g2_smallSymbols();

  //display volume level
  if (processVolume != currentVolume) {
    u8g2_Text8();
    if (currentVolume < 10) {
      u8g2.setCursor(93, 0);
      u8g2.print("          ");
      u8g2.print(currentVolume);
    } else if (currentVolume >= 10) {
      u8g2.setCursor(93, 0);
      u8g2.print("        ");
      u8g2.print(currentVolume);
    }
  } else if (processVolume == currentVolume) {
    if (currentVolume >= 24) {
      u8g2_smallSymbols();
      u8g2.drawUTF8(93, 0, "\u2581");
      u8g2.drawUTF8(100, 0, "\u2582");
      u8g2.drawUTF8(107, 0, "\u2583");
      u8g2.drawUTF8(114, 0, "\u2584");
      u8g2.drawUTF8(121, 0, "\u2585");
    } else if (currentVolume >= 18) {
      u8g2_smallSymbols();
      u8g2.drawUTF8(93, 0, "\u2581");
      u8g2.drawUTF8(100, 0, "\u2582");
      u8g2.drawUTF8(107, 0, "\u2583");
      u8g2.drawUTF8(114, 0, "\u2584");
    } else if (currentVolume >= 12) {
      u8g2_smallSymbols();
      u8g2.drawUTF8(93, 0, "\u2581");
      u8g2.drawUTF8(100, 0, "\u2582");
      u8g2.drawUTF8(107, 0, "\u2583");
    } else if (currentVolume >= 6) {
      u8g2_smallSymbols();
      u8g2.drawUTF8(93, 0, "\u2581");
      u8g2.drawUTF8(100, 0, "\u2582");
    } else {
      u8g2_smallSymbols();
      u8g2.drawUTF8(93, 0, "\u2581");
    }
  }
}


void alarmPage() {
  setAlarmPage();
  u8g2_7Seg();

  u8g2.setCursor(15, 10);
  
  if (alarmCounter == 0 && miniFlash == 0) {
    u8g2.print(" ");
  } else {
    u8g2.print(alarmHour0);
  }

  if (alarmCounter == 1 && miniFlash == 0) {
    u8g2.print(" ");
  } else {
    u8g2.print(alarmHour1);
  }
  
  u8g2.print(":");
  u8g2.setCursor(62, 10);

  if (alarmCounter == 2 && miniFlash == 0) {
    u8g2.print(" ");  
  } else {
    u8g2.print(alarmMinute0);
  }

  if (alarmCounter == 3 && miniFlash == 0) {
    u8g2.print(" ");
  } else {
    u8g2.print(alarmMinute1);
  }
  
  u8g2_Text8();
  u8g2.setCursor(104, 12);

  if (alarmCounter == 4 && miniFlash == 0) {
    u8g2.print("  ");
  } else {
    if (alarmMeridiem == false) {
      u8g2.print("AM");
    } else if (alarmMeridiem == true) {
      u8g2.print("PM");
    }
  }

  u8g2.drawStr(30, 53, "Set Alarm");
  u8g2_Symbols();
  u8g2.drawUTF8(85, 50, "\u23F0");
}


void setAlarmPage() {
  if ((keyInput == "#" || remoteInput == "ST/REPT") && alarmCounter >= 4) {
    alarmCounter = 0;
    remoteInput = "z";
    miniFlash = false;
    beginMillis = currentMillis;
  } else if ((keyInput == "#" || remoteInput == "ST/REPT") && alarmCounter < 4) {
    alarmCounter++;
    remoteInput = "z";
    miniFlash = false;
    beginMillis = currentMillis;
  }

//check keypad input
  if (alarmCounter == 0 && (keyInput == "0" || keyInput == "1")) {
    alarmHour0 = keyInput;
    alarmCounter++;
    miniFlash = false;
    beginMillis = currentMillis;
  } else if (alarmCounter == 1 && alarmHour0 == "0" && (keyInput == "1" || keyInput == "2" || keyInput == "3" || keyInput == "4" || keyInput == "5" || keyInput == "6" || keyInput == "7" || keyInput == "8" || keyInput == "9")) {
    alarmHour1 = keyInput;
    alarmCounter++;
    miniFlash = false;
    beginMillis = currentMillis;
  } else if (alarmCounter == 1 && alarmHour0 == "1" && (keyInput == "0" || keyInput == "1" || keyInput == "2")) {
    alarmHour1 = keyInput;
    alarmCounter++;
    miniFlash = false;
    beginMillis = currentMillis;
  } else if (alarmCounter == 2 && (keyInput == "0" || keyInput == "1" || keyInput == "2" || keyInput == "3" || keyInput == "4" || keyInput == "5")) {
    alarmMinute0 = keyInput;
    alarmCounter++;
    miniFlash = false;
    beginMillis = currentMillis;
  } else if (alarmCounter == 3 && (keyInput == "0" || keyInput == "1" || keyInput == "2" || keyInput == "3" || keyInput == "4" || keyInput == "5" || keyInput == "6" || keyInput == "7" || keyInput == "8" || keyInput == "9")) {
    alarmMinute1 = keyInput;
    alarmCounter++;
    miniFlash = false;
    beginMillis = currentMillis;
  } else if (alarmCounter == 4 && keyInput == "0") {
    alarmMeridiem = false;
    alarmCounter = 0;
    miniFlash = false;
    beginMillis = currentMillis;
  } else if (alarmCounter == 4 && keyInput == "1") {
    alarmMeridiem = true;
    alarmCounter = 0;
    miniFlash = false;
    beginMillis = currentMillis;
  }

//check remote input
  if (alarmCounter == 0 && (remoteInput == "0" || remoteInput == "1")) {
    alarmHour0 = remoteInput;
    alarmCounter++;
    remoteInput = "z";
    miniFlash = false;
    beginMillis = currentMillis;
  } else if (alarmCounter == 1 && alarmHour0 == "0" && (remoteInput == "1" || remoteInput == "2" || remoteInput == "3" || remoteInput == "4" || remoteInput == "5" || remoteInput == "6" || remoteInput == "7" || remoteInput == "8" || remoteInput == "9")) {
    alarmHour1 = remoteInput;
    alarmCounter++;
    remoteInput = "z";
    miniFlash = false;
    beginMillis = currentMillis;
  } else if (alarmCounter == 1 && alarmHour0 == "1" && (remoteInput == "0" || remoteInput == "1" || remoteInput == "2")) {
    alarmHour1 = remoteInput;
    alarmCounter++;
    remoteInput = "z";
    miniFlash = false;
    beginMillis = currentMillis;
  } else if (alarmCounter == 2 && (remoteInput == "0" || remoteInput == "1" || remoteInput == "2" || remoteInput == "3" || remoteInput == "4" || remoteInput == "5")) {
    alarmMinute0 = remoteInput;
    alarmCounter++;
    remoteInput = "z";
    miniFlash = false;
    beginMillis = currentMillis;
  } else if (alarmCounter == 3 && (remoteInput == "0" || remoteInput == "1" || remoteInput == "2" || remoteInput == "3" || remoteInput == "4" || remoteInput == "5" || remoteInput == "6" || remoteInput == "7" || remoteInput == "8" || remoteInput == "9")) {
    alarmMinute1 = remoteInput;
    alarmCounter++;
    remoteInput = "z";
    miniFlash = false;
    beginMillis = currentMillis;
  } else if (alarmCounter == 4 && remoteInput == "0") {
    alarmMeridiem = false;
    alarmCounter = 0;
    remoteInput = "z";
    miniFlash = false;
    beginMillis = currentMillis;
  } else if (alarmCounter == 4 && remoteInput == "1") {
    alarmMeridiem = true;
    alarmCounter = 0;
    remoteInput = "z";
    miniFlash = false;
    beginMillis = currentMillis;
  }

  if (alarmHour0 == "1" && alarmHour1 > "2") {
    alarmHour1 = "0";
  }

  if (alarmHour0 == "0" && alarmHour1 == "0") {
    alarmHour1 = "1";
  }
}


void convertAlarm() {
  alarmHourStr = alarmHour0 + alarmHour1;
  alarmHourInt = alarmHourStr.toInt();
  alarmMinuteStr = alarmMinute0 + alarmMinute1;
  alarmMinuteInt = alarmMinuteStr.toInt();
}


void convertSnooze() {
  snoozeDelayStr = snoozeDelay0 + snoozeDelay1;
  snoozeDelayInt = snoozeDelayStr.toInt();
  snoozeTimeSeconds = snoozeDelayInt * 60;
}

void convertAlarmDelay() {
  alarmDelayStr = alarmDelay0 + alarmDelay1;
  alarmDelayInt = alarmDelayStr.toInt();
  alarmTimeSeconds = alarmDelayInt * 60;
}


void alarmDelayPage() {
  setAlarmDelayPage();
  u8g2_7Seg();

  u8g2.setCursor(15, 10);
  
  if (alarmDelayCounter == 0 && miniFlash == 0) {
    u8g2.print(" ");
  } else {
    u8g2.print(alarmDelay0);
  }

  if (alarmDelayCounter == 1 && miniFlash == 0) {
    u8g2.print(" ");
  } else {
    u8g2.print(alarmDelay1);
  }

  
  u8g2_Text8();
  u8g2.setCursor(62, 33);
  u8g2.print("Minutes");
  u8g2.drawStr(18, 53, "Set Alarm Delay");
  u8g2_Symbols();
  u8g2.drawUTF8(104, 53, "\u25B6");
}


void setAlarmDelayPage() {
    if ((keyInput == "#" || remoteInput == "ST/REPT") && alarmDelayCounter >= 1) {
    alarmDelayCounter = 0;
    remoteInput = "z";
    miniFlash = false;
    beginMillis = currentMillis;
  } else if ((keyInput == "#" || remoteInput == "ST/REPT") && alarmDelayCounter < 1) {
    alarmDelayCounter++;
    remoteInput = "z";
    miniFlash = false;
    beginMillis = currentMillis;
  }

//check keypad input
  if (alarmDelayCounter == 0 && (keyInput == "0" || keyInput == "1" || keyInput == "2" || keyInput == "3" || keyInput == "4" || keyInput == "5")) {
    alarmDelay0 = keyInput;
    alarmDelayCounter++;
    miniFlash = false;
    beginMillis = currentMillis;
  } else if (alarmDelayCounter == 1 && (keyInput == "0" || keyInput == "1" || keyInput == "2" || keyInput == "3" || keyInput == "4" || keyInput == "5" || keyInput == "6" || keyInput == "7" || keyInput == "8" || keyInput == "9")) {
    alarmDelay1 = keyInput;
    alarmDelayCounter = 0;
    miniFlash = false;
    beginMillis = currentMillis;
  }

//check remote input
  if (alarmDelayCounter == 0 && (remoteInput == "0" || remoteInput == "1" || remoteInput == "2" || remoteInput == "3" || remoteInput == "4" || remoteInput == "5")) {
    alarmDelay0 = remoteInput;
    alarmDelayCounter++;
    remoteInput = "z";
    miniFlash = false;
    beginMillis = currentMillis;
  } else if (alarmDelayCounter == 1 && (remoteInput == "0" || remoteInput == "1" || remoteInput == "2" || remoteInput == "3" || remoteInput == "4" || remoteInput == "5" || remoteInput == "6" || remoteInput == "7" || remoteInput == "8" || remoteInput == "9")) {
    alarmDelay1 = remoteInput;
    alarmDelayCounter = 0;
    remoteInput = "z";
    miniFlash = false;
    beginMillis = currentMillis;
  }
}


void snoozeDelayPage() {
  setSnoozeDelayPage();
  u8g2_7Seg();

  u8g2.setCursor(15, 10);
  
  if (snoozeDelayCounter == 0 && miniFlash == 0) {
    u8g2.print(" ");
  } else {
    u8g2.print(snoozeDelay0);
  }

  if (snoozeDelayCounter == 1 && miniFlash == 0) {
    u8g2.print(" ");
  } else {
    u8g2.print(snoozeDelay1);
  }

  
  u8g2_Text8();
  u8g2.setCursor(62, 33);
  u8g2.print("Minutes");
  u8g2.drawStr(15, 53, "Set Snooze Delay");
  u8g2_Symbols();
  u8g2.drawUTF8(106, 53, "\u25D0");
}


void setSnoozeDelayPage() {
    if ((keyInput == "#" || remoteInput == "ST/REPT") && snoozeDelayCounter >= 1) {
    snoozeDelayCounter = 0;
    remoteInput = "z";
    miniFlash = false;
    beginMillis = currentMillis;
  } else if ((keyInput == "#" || remoteInput == "ST/REPT") && snoozeDelayCounter < 1) {
    snoozeDelayCounter++;
    remoteInput = "z";
    miniFlash = false;
    beginMillis = currentMillis;
  }

//check keypad input
  if (snoozeDelayCounter == 0 && (keyInput == "0" || keyInput == "1" || keyInput == "2" || keyInput == "3" || keyInput == "4" || keyInput == "5")) {
    snoozeDelay0 = keyInput;
    snoozeDelayCounter++;
    miniFlash = false;
    beginMillis = currentMillis;
  } else if (snoozeDelayCounter == 1 && (keyInput == "0" || keyInput == "1" || keyInput == "2" || keyInput == "3" || keyInput == "4" || keyInput == "5" || keyInput == "6" || keyInput == "7" || keyInput == "8" || keyInput == "9")) {
    snoozeDelay1 = keyInput;
    snoozeDelayCounter = 0;
    miniFlash = false;
    beginMillis = currentMillis;
  }

//check remote input
  if (snoozeDelayCounter == 0 && (remoteInput == "0" || remoteInput == "1" || remoteInput == "2" || remoteInput == "3" || remoteInput == "4" || remoteInput == "5")) {
    snoozeDelay0 = remoteInput;
    snoozeDelayCounter++;
    remoteInput = "z";
    miniFlash = false;
    beginMillis = currentMillis;
  } else if (snoozeDelayCounter == 1 && (remoteInput == "0" || remoteInput == "1" || remoteInput == "2" || remoteInput == "3" || remoteInput == "4" || remoteInput == "5" || remoteInput == "6" || remoteInput == "7" || remoteInput == "8" || remoteInput == "9")) {
    snoozeDelay1 = remoteInput;
    snoozeDelayCounter = 0;
    remoteInput = "z";
    miniFlash = false;
    beginMillis = currentMillis;
  }
}


void playlistPage() {
  setPlaylistPage();
  u8g2_Text8();
  u8g2.setCursor(18, 25);

  if (miniFlash == 0) {
    u8g2.print("                  ");
  } else if (playlistCounter == 1) {
    u8g2.print("1. Classic Country");
  } else if (playlistCounter == 2) {
    u8g2.print("   2. Good Vibes  ");
  } else if (playlistCounter == 3) {
    u8g2.print("  3. Classic Rock ");
  } else if (playlistCounter == 4) {
    u8g2.print("    4. Hard Rock  ");
  } else if (playlistCounter == 5) {
    u8g2.print("        5. LoFi   ");
  } else if (playlistCounter == 6) {
    u8g2.print("    6. Christmas  ");
  } else if (playlistCounter == 7) {
    u8g2.print("         7. Rap   ");
  } else if (playlistCounter == 8) {
    u8g2.print("      8. Country  ");
  } else if (playlistCounter == 9) {
    u8g2.print("    9. Hair Metal ");
  }

  
  u8g2.drawStr(13, 53, "Pick Your Playlist");
  u8g2_Symbols();
  u8g2.drawUTF8(107, 50, "\u00B6");
}


void setPlaylistPage() {
  if ((keyInput == "#" || remoteInput == "ST/REPT") && playlistCounter <= 8) {
    playlistCounter++;
    remoteInput = "z";
    miniFlash = true;
    beginMillis = currentMillis - miniPeriod;
  } else if (((keyInput == "#" || remoteInput == "ST/REPT") && playlistCounter >= 9) || (keyInput == "1" || remoteInput == "1")) {
    playlistCounter = 1;
    remoteInput = "z";
    miniFlash = true;
    beginMillis = currentMillis - miniPeriod;
  } else if (keyInput == "2" || remoteInput == "2") {
    playlistCounter = 2;
    remoteInput = "z";
    miniFlash = true;
    beginMillis = currentMillis - miniPeriod;
  } else if (keyInput == "3" || remoteInput == "3") {
    playlistCounter = 3;
    remoteInput = "z";
    miniFlash = true;
    beginMillis = currentMillis - miniPeriod;
  } else if (keyInput == "4" || remoteInput == "4") {
    playlistCounter = 4;
    remoteInput = "z";
    miniFlash = true;
    beginMillis = currentMillis - miniPeriod;
  } else if (keyInput == "5" || remoteInput == "5") {
    playlistCounter = 5;
    remoteInput = "z";
    miniFlash = true;
    beginMillis = currentMillis - miniPeriod;
  } else if (keyInput == "6" || remoteInput == "6") {
    playlistCounter = 6;
    remoteInput = "z";
    miniFlash = true;
    beginMillis = currentMillis - miniPeriod;
  } else if (keyInput == "7" || remoteInput == "7") {
    playlistCounter = 7;
    remoteInput = "z";
    miniFlash = true;
    beginMillis = currentMillis - miniPeriod;
  } else if (keyInput == "8" || remoteInput == "8") {
    playlistCounter = 8;
    remoteInput = "z";
    miniFlash = true;
    beginMillis = currentMillis - miniPeriod;
  } else if (keyInput == "9" || remoteInput == "9") {
    playlistCounter = 9;
    remoteInput = "z";
    miniFlash = true;
    beginMillis = currentMillis - miniPeriod;
  }
}


void checkAlarm () {
  DateTime now = RTC.now();
  
  if (alarmState == true) {
    if ((fixedHours == alarmHourInt) && (fixedMinutes == alarmMinuteInt) && (fixedSeconds == 0)) {
      alarmLightHasStarted = true;
      alarmStartingPoint = now.second();
    }
  }

  if (alarmLightHasStarted == true && alarmStartingPoint != now.second() && alarmHasStarted == false) {
    alarmLightTracker++;
    gBrightness = alarmLightTracker * (gSetPoint / alarmTimeSeconds);
    rBrightness = alarmLightTracker * (rSetPoint / alarmTimeSeconds);
    bBrightness = alarmLightTracker * (bSetPoint / alarmTimeSeconds);
    alarmStartingPoint = now.second();
  }

  if (alarmLightHasStarted == true && alarmLightTracker >= alarmTimeSeconds) {
    if ((busyState == HIGH || playNextSong == true || remoteInput == "PAUSE") && snoozeHasStarted == false) {
      alarmHasStarted = true;
      randomSong = random(1,songAmount);
      myDFPlayer.playFolder(playlistCounter,randomSong);
      remoteInput = "z";
      delay(1000);
      playNextSong = false;
    }
  }

  if (alarmLightHasStarted == true && alarmTimeSeconds == 0) {
    gBrightness = gSetPoint;
    rBrightness = rSetPoint;
    bBrightness = bSetPoint;
  }
}


void displayAlarmLights() {
  if (gBrightness <= maxBrightness && rBrightness <= maxBrightness && bBrightness <= maxBrightness) {
    analogWrite(GREEN_LED, gBrightness);
    analogWrite(RED_LED, rBrightness);
    analogWrite(BLUE_LED, bBrightness);
  }
}


void checkSnooze() {
  DateTime now = RTC.now();
  
  if (alarmHasStarted == true && (snoozeState == HIGH || remoteInput == "EQ")) {
    snoozeHasStarted = true;
    myDFPlayer.stop();
    digitalWrite(snoozeLightPin, HIGH);
    snoozeStartingPoint = now.second();
    remoteInput = "z";
  }

  if (snoozeHasStarted == true && snoozeStartingPoint != now.second()) {
    snoozeTracker++;
    snoozeStartingPoint = now.second();
  }

  if (snoozeHasStarted == true && snoozeTracker >= snoozeTimeSeconds) {
    snoozeHasStarted = false;
    digitalWrite(snoozeLightPin, LOW);
    randomSong = random(1,songAmount);
    myDFPlayer.playFolder(playlistCounter,randomSong);
    delay(500);
    playNextSong = false;
    snoozeTracker = 0;
  }
}


void endAlarm() {
  if (alarmState == false) {
    myDFPlayer.stop();
    alarmHasStarted = false;
    alarmLightHasStarted = false;
    snoozeHasStarted = false;
    digitalWrite(snoozeLightPin, LOW);
    playNextSong = false;
    snoozeTracker = 0;
    alarmLightTracker = 0;
    gBrightness = 0.00;
    rBrightness = 0.00;
    bBrightness = 0.00;
  }
}


void translateIR() {


  switch(results.value){
    case 0xFFFFFFFF:
      remoteInput = "REPEAT";
      //Serial.println(" REPEAT");
      break;  
    case 0xFFA25D:
      remoteInput = "POWER";
      //Serial.println("POWER"); 
      break;
    case 0xFFE21D: 
      remoteInput = "FUNC/STOP";
      //Serial.println("FUNC/STOP"); 
      break;
    case 0xFF629D: 
      remoteInput = "VOL+";
      //Serial.println("VOL+"); 
      break;
    case 0xFF22DD:
      remoteInput = "FAST BACK";
      //Serial.println("FAST BACK");    
      break;
    case 0xFF02FD:
      remoteInput = "PAUSE";
      //Serial.println("PAUSE");    
      break;
    case 0xFFC23D:
      remoteInput = "FAST FORWARD";
      //Serial.println("FAST FORWARD");   
      break;
    case 0xFFE01F:
      remoteInput = "DOWN";
      //Serial.println("DOWN");    
      break;
    case 0xFFA857:
      remoteInput = "VOL-";
      //Serial.println("VOL-");    
      break;
    case 0xFF906F:
      remoteInput = "UP";
      //Serial.println("UP");    
      break;
    case 0xFF9867:
      remoteInput = "EQ";
      //Serial.println("EQ");    
      break;
    case 0xFFB04F:
      remoteInput = "ST/REPT";
      //Serial.println("ST/REPT");    
      break;
    case 0xFF6897:
      remoteInput = "0";
      //Serial.println("0");    
      break;
    case 0xFF30CF:
      remoteInput = "1";
      //Serial.println("1");    
      break;
    case 0xFF18E7:
      remoteInput = "2";
      //Serial.println("2");    
      break;
    case 0xFF7A85:
      remoteInput = "3";
      //Serial.println("3");    
      break;
    case 0xFF10EF:
      remoteInput = "4";
      //Serial.println("4");    
      break;
    case 0xFF38C7:
      remoteInput = "5";
      //Serial.println("5");    
      break;
    case 0xFF5AA5:
      remoteInput = "6";
      //Serial.println("6");    
      break;
    case 0xFF42BD:
      remoteInput = "7";
      //Serial.println("7");    
      break;
    case 0xFF4AB5:
      remoteInput = "8";
      //Serial.println("8");    
      break;
    case 0xFF52AD:
      remoteInput = "9";
      //Serial.println("9");    
      break;


  default:
    remoteInput = "REPEAT";

  
  }

}


void volumePage() {
  
  setVolumePage();
  u8g2_Text8();
  u8g2.setCursor(18, 25);

  if (miniFlash == 0) {
    u8g2.print("                  ");
  } else if (volumeSetting == 0) {
    u8g2.print("      1. Buttons  ");
  } else if (volumeSetting == 1) {
    u8g2.print("        2. Knob   ");
  }

  
  u8g2.drawStr(22, 53, "Volume Controls");
}


void setVolumePage() {
  if (((keyInput == "#" || remoteInput == "ST/REPT") && volumeSetting == 0) || (keyInput == "2" || remoteInput == "2")) {
    volumeSetting = 1;
    remoteInput = "z";
    miniFlash = true;
    beginMillis = currentMillis - miniPeriod;
  } else if (((keyInput == "#" || remoteInput == "ST/REPT") && volumeSetting == 1) || (keyInput == "1" || remoteInput == "1")) {
    volumeSetting = 0;
    remoteInput = "z";
    miniFlash = true;
    beginMillis = currentMillis - miniPeriod;
  }
}


void checkVolume() {
  
  if (volumeSetting == 0) {
    if ((volumeUpState == true || remoteInput == "VOL+") && volumeButtonVal < 30) {
      volumeButtonVal++;
      remoteInput = "z";
      delay(100);
    } else if ((volumeDownState == true || remoteInput == "VOL-") && volumeButtonVal > 1) {
      volumeButtonVal--;
      remoteInput = "z";
      delay(100);
    }
  }
}


void displayVolume() {

  DateTime now = RTC.now();

  currentSecond = now.second();

  if (processVolume != currentVolume && displayVolumeState == false) {
    processSeconds = now.second();
    displayVolumeState = true;
  }
  
  if (displayVolumeState == true && processSeconds <= 56) {
    if (currentSecond >= (processSeconds + 3)) {
      processVolume = currentVolume;
      displayVolumeState = false;
      processSeconds = now.second();
    }
  } else if (displayVolumeState == true && processSeconds == 57) {
    if (currentSecond == 0) {
      processVolume = currentVolume;
      displayVolumeState = false;
      processSeconds = now.second();
    }
  } else if (displayVolumeState == true && processSeconds == 58) {
    if (currentSecond == 1) {
      processVolume = currentVolume;
      displayVolumeState = false;
      processSeconds = now.second();
    }
  } else if (displayVolumeState == true && processSeconds == 59) {
    if (currentSecond == 2) {
      processVolume = currentVolume;
      displayVolumeState = false;
      processSeconds = now.second();
    }
  }
}



//future fixes/features
//improvement - center the date on home page using string lengths
//improvement - display the current song playing on home page instead of date (probably not reasonable with amount of memory available)
//addition - make a page to set defaults and store to memory? (ex: default 1 - 6AM, 10min delay, 5min snooze, classic rock)
//addition - remote down arrow and/or push button to replay song?
//bug - alarm potentially not working sometimes when setting alarm overnight??? may have accidentally fixed?
//bug - clock randomly acts up when mp3 is playing ... have not seen this issue in a while, may have accidentally fixed?
//idea - connection piece to make the keypad removable... keep the clock looking nice with the 4 main buttons, while only needing the keypad as a backup to the remote
