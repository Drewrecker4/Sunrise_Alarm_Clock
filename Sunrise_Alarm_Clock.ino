#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "RTClib.h"
#include <Keypad.h>
#include "DFRobotDFPlayerMini.h"
#include <SPI.h>
#include "IRremote.h"

//  I/O Mapping
const uint8_t   oled_dc     = 2;
const uint8_t   blue_led    = 5;
const uint8_t   red_led     = 6;
const uint8_t   green_led   = 7;
const uint8_t   busy_pin    = 8;
const uint8_t   dfplayer_rx = 18;
const uint8_t   dfplayer_tx = 19;
const uint8_t   row_1       = 22;
const uint8_t   row_2       = 23;
const uint8_t   row_3       = 24;
const uint8_t   row_4       = 25;
const uint8_t   col_1       = 26;
const uint8_t   col_2       = 27;
const uint8_t   col_3       = 28;
const uint8_t   col_4       = 29;
const uint8_t   skip_btn    = 30;
const uint8_t   snooze_btn  = 31;
const uint8_t   snooze_lt   = 32;
const uint8_t   receiver    = 33;
const uint8_t   vol_up_btn  = 34;
const uint8_t   vol_dn_btn  = 35;
const uint8_t   oled_reset  = 48;
const uint8_t   oled_cs     = 53;

//  Initialize Variables
//  Global Variables
//  LED Strip
const uint8_t   max_brightness      = 255;
uint8_t         green_sp            = 30;
uint8_t         red_sp              = 255;
uint8_t         blue_sp             = 0;
uint8_t         green_brightness    = 0;
uint8_t         red_brightness      = 0;
uint8_t         blue_brightness     = 0;

//  RTC Module
char    days_of_week[7][12] = { "Sunday",
                                "Monday",
                                "Tuesday",
                                "Wednesday",
                                "Thursday",
                                "Friday",
                                "Saturday"};
uint8_t fixed_minutes;
uint8_t fixed_hours;
uint8_t fixed_seconds;
word    fixed_year;
uint8_t fixed_month;
uint8_t fixed_day;
char    fixed_day_of_week;
bool    meridiem;

//  Keypad Module
const uint8_t rows  = 4;
const uint8_t cols  = 4;
char          keys[rows][cols] = {{'1','2','3','A'},
                                 {'4','5','6','B'},
                                 {'7','8','9','C'},
                                 {'*','0','#','D'}};
uint8_t       row_pins[rows]    = {row_1, row_2, row_3, row_4};
uint8_t       col_pins[cols]    = {col_1, col_2, col_3, col_4};
String        key_input;
String        remote_input;

// MP3 Module
uint8_t     random_song;
bool        busy_state;
bool        skip_state;

//  Library Structs
IRrecv          irrecv(receiver);
decode_results  results;
U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI u8g2(U8G2_R0, oled_cs, oled_dc, oled_reset); //OLED Module
RTC_DS1307      RTC;
Keypad          keypad  = Keypad(makeKeymap(keys), row_pins, col_pins, rows, cols);
DFRobotDFPlayerMini myDFPlayer;

void setup()
{
  initialize_variables();
}

void loop()
{
  
}

void initialize_variables()
{

}