#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "RTClib.h"
#include <Keypad.h>
#include "DFRobotDFPlayerMini.h"
#include <SPI.h>
#include "IRremote.h"

//  I/O Mapping
const byte  red_led     = 6;
const byte  blue_led    = 5;
const byte  green_led   = 7;
const byte  receiver    = 33;
const byte  oled_cs     = 53;
const byte  oled_dc     = 2;
const byte  oled_reset  = 48;
const byte  row_1       = 22;
const byte  row_2       = 23;
const byte  row_3       = 24;
const byte  row_4       = 25;
const byte  col_1       = 26;
const byte  col_2       = 27;
const byte  col_3       = 28;
const byte  col_4       = 29;
const byte  dfplayer_tx = 19;
const byte  dfplayer_rx = 18;
const byte  busy_pin    = 8;
const byte  skip_btn    = 30;
const byte  snooze_btn  = 31;
const byte  snooze_lt   = 32;
const byte  vol_up_btn  = 34;
const byte  vol_dn_btn  = 35;

//  Initialize Variables
//void initialize_variables() {
  //  LED Strip
  const byte  max_brightness      = 255;
  byte        green_sp            = 30;
  byte        red_sp              = 255;
  byte        blue_sp             = 0;
  byte        green_brightness    = 0;

  //  RTC Module
  char        days_of_week[7][12] = {"Sunday",
                                     "Monday",
                                     "Tuesday",
                                     "Wednesday",
                                     "Thursday",
                                     "Friday",
                                     "Saturday"};
  byte        fixed_minutes;
  byte        fixed_hours;
  byte        fixed_seconds;
  word        fixed_year;
  byte        fixed_month;
  byte        fixed_day;
  char        fixed_day_of_week;
  bool        meridiem;

  //  Keypad Module
  const byte  rows  = 4;
  const byte  cols  = 4;
  char        keys[rows][cols] = {{'1','2','3','A'},
                                   {'4','5','6','B'},
                                   {'7','8','9','C'},
                                   {'*','0','#','D'}};
  byte        row_pins[rows]    = {row_1, row_2, row_3, row_4};
  byte        col_pins[cols]    = {col_1, col_2, col_3, col_4};
  String      key_input;
  String      remote_input;

  // MP3 Module
  byte        random_song;
  bool        busy_state;
  bool        skip_state;
//}

//  Library Structs
IRrecv          irrecv(receiver);
decode_results  results;
U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI u8g2(U8G2_R0, oled_cs, oled_dc, oled_reset); //OLED Module
RTC_DS1307      RTC;
Keypad          keypad  = Keypad(makeKeymap(keys), row_pins, col_pins, rows, cols);
DFRobotDFPlayerMini myDFPlayer;

void setup()
{
  //initialize_variables();
}

void loop()
{
  
}
