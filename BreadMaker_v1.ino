//#define SERIAL_INPUT_DBG

#ifdef SERIAL_INPUT_DBG

#define get_input(x) get_serial_input(x)

#endif

//#define NO_KEYPAD

#ifdef NO_KEYPAD

#define waitForStar(x) get_some_serial_input()

#else 

#define waitForStar(x) x.waitForKey()

#endif

/*
 The Heat Sensor Wire:
 * connected to pin 10
 * V2: connected to pin D10

 Heat Coil
 * Connect to pin 13
  
 Motor Control
 * Connected to pin A5 for now
 
 Buzzer
 * V2: connected to A6
 
 The LCD circuit:
 * hd44780 16x2
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5 V2: D2 v3: a4
 * LCD D5 pin to digital pin 4 V2: D3 v3: a5
 * LCD D6 pin to digital pin 3 V2: D4 v3: a6
 * LCD D7 pin to digital pin 2 V2: D5 v3: a7
 * LCD R/W pin to ground
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
 
 The Keypad circuit:
 * for now, rows: 9, 8, 7, 6
 * for now columns: A0, A1, A2
 *
 */

// Include standard library
#include <stdlib.h>
// Include temperature sensor library
#include <OneWire.h>
// include the LCD library
#include <LiquidCrystal.h>
// include keypad library
#include <Keypad.h>

// DS18S20 Temperature chip i/o (a 4.7K resistor is necessary)
OneWire ds(10);  // on pin 10

// Heat Coil relay (to connect heat coil to AC power) connected to pin 13 - an LED for now
int heat_coil = 13;
// Motor PWM controller connect to 3, 5, 6, 9, 10 or 11
int motor_PWM = 2; // For now, pin 2
// Buzzer!
int buzzer = 3;

/*********************************
 * Settings can be modified here *
 *********************************/
 
#define TemperatureTolerance 1 // 1 degree tolerance to start with
#define TextDelay 1000         // Time to display text (in milliseconds)
#define UpdateTime 1000        // Update time every second
#define ROWS 4 //four rows
#define COLS 3 //three columns
#define ON 255
#define OFF 0
#define FAST 240
#define MEDIUM 160
#define SLOW 80
#define printTwoLines(text1, text2)   lcd.setCursor(0,0);\
                                      lcd.print(F(text1));\
                                      lcd.setCursor(0,1);\
                                      lcd.print(F(text2));

// Define keypad structure (as found on Sparkfun)
char numberKeys[ROWS][COLS] = {
    { '1','2','3' },
    { '4','5','6' },
    { '7','8','9' },
    { '*','0','#' }
};


// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, A4, A5, A6, A7);

// Set up pins to keypad

byte rowPins[ROWS] = {6, 7, 8, 9}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {A0, A1, A2}; //connect to the column pinouts of the keypad

// Create new keypad, a number pad.
Keypad numpad = Keypad( makeKeymap(numberKeys), rowPins, colPins, sizeof(rowPins), sizeof(colPins) );

// Create some values to store:
// Preset mixing time: 5 minutes
int mix = 2;
int mix2 = 1;
// Rise time
int rise1 = 2;
// Rise temperature
int rise1_temp = 28;
// Knock time
int knock1 = 2;
// Second rise time
int rise2 = 2;
// Second rise temperature
int rise2_temp = 28;
// To keep time
float time_begin;
float time_end;
float time_left;
float time_last;
char time_left_string[10];
// General temperature variable
float temperature;

// Set a State variable
int myState = 0;
// Key value variable
char key;

// Initialize functions
// Beeper 
void beep(int, int, int);
// Print remaining time
void printTimeLeft(int);
// Temperature Control Fucntion
void temp_control(int);
// Temperature Sensor Function
float get_temp();
// Keypad Functions
#ifndef SERIAL_INPUT_DBG
int get_input(int k);
#endif
int get_serial_input(int k);

void setup() {
  
  // Set bit rate
  Serial.begin(9600);
  Serial.setTimeout(1000000);
  
  // Initialize heat coil as output
  pinMode(heat_coil, OUTPUT);
  // Initialize buzzer as output
  pinMode(buzzer, OUTPUT);
  // Initialize keypad
  numpad.begin( makeKeymap(numberKeys) );
  // Add event listener: triggers an event if the keypad is used
  numpad.addEventListener(keypadEvent_num);
  // Sets the amount of time button needs to be pressed to trigger the "HOLD" state
  numpad.setHoldTime(1000);
  
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  
}

void loop() {
  
  switch(myState){
    
    case 0:
      // Initialization message
      // Print a message to the LCD.
      beep(buzzer, 100, 53);
      printTwoLines("Hannah's", "BreadMaker <3");
      // Turn on the display:
      //lcd.display();
      delay(2000);
      // Turn off the display:
      lcd.clear();
      //lcd.noDisplay();
      delay(500);
      myState = 1;
      break;
      
    case 1:
      // Idle state
      lcd.print(F("Press * to Start!"));
      key = numpad.waitForKey(); //not sure if need to add getKey() after this
      while (key != '*'){
        key = numpad.waitForKey();
      }
      myState = 12;
      break;
    
    case 12:
      lcd.clear();
      //printTwoLines("To use default", "values, press 0.");
      //delay(2000);
      //lcd.clear();
      //printTwoLines("To customize,", "press 1.");
      //delay(2000);
      //lcd.clear();
      printTwoLines("Default = 0", "Customize = 1");
      key = numpad.waitForKey();
      if (key == '1'){
        lcd.clear();
        myState = 2;
      }
      else if (key == '0'){
        lcd.clear();
        myState = 20;
      }
      else {
        lcd.clear();
        printTwoLines("Please make", "a decision");
        delay(TextDelay);
        myState = 12;
      }
      // Turn off the display:
      lcd.clear();
      //lcd.noDisplay();
      break;
      
    case 2:
      // Ask for rise time
      printTwoLines("Enter Rise Time", "In minutes");
      delay(TextDelay);
      lcd.clear();
      printTwoLines("To erase,", "Press *!");
      delay(TextDelay);
      lcd.clear();
      lcd.print(F("Press # if done:"));
      delay(TextDelay);
      lcd.clear();
      // Get rise time
      lcd.print(F("Rise Time:"));
      lcd.setCursor(0, 1);
      // Store rise time as an int
      // As the time only goes up to a few hours, there are a max of 3 digits, so we input 2 into the function
      rise1 = get_input(3);
      // Allow time for user to see entered time
      //delay(500);
      // Turn off the display to move on
      lcd.clear();
      
      // Safety
      if (rise1>240){
        rise1 = 240;
      
        printTwoLines("The rise time", "is too long,");
        delay(TextDelay);
        lcd.clear();
        printTwoLines("It has been set", "to 4 hrs.");
        delay(TextDelay);
        lcd.clear();
      }
      
      // Get rise temperature
      if (rise1 == 0){
        printTwoLines("Sorry, you must", "enter a value.");
        delay(TextDelay);
        lcd.clear();
        myState = 2;
      }
      else{
        myState = 3;
      }
      break;
      
    case 3:
      // Get rise temperature
      printTwoLines("Enter Rise Temp", "In Celsius");
      delay(TextDelay);            
      lcd.clear();
      printTwoLines("To erase,", "Press *!");
      delay(TextDelay);
      lcd.clear();
      lcd.print(F("Press # if done"));
      delay(TextDelay);
      lcd.clear();
      lcd.print(F("Rise Temp:"));
      lcd.setCursor(0, 1);
      // Store rise time as an int
      // As the temp only goes up to a few degrees, there are a max of 2 digits
      rise1_temp = get_input(2);
      // Allow time for user to see entered time
      delay(500);
      // Turn off the display to move on
      lcd.clear();
      
      
      // Safety
      if (rise1_temp>50){
        rise1_temp = 50;
      
        printTwoLines("The temperature", "is set too high,");
        delay(TextDelay);
        lcd.clear();
        printTwoLines("It has been set", "to 50 (C).");
        delay(TextDelay);
        lcd.clear();
      }
      // Get rise temperature
      if (rise1_temp == 0){
        printTwoLines("Sorry, you must", "enter a value.");
        delay(TextDelay);
        lcd.clear();
        myState = 3;
      }
      else{
        myState = 4;
      }
      break;
      
    case 4:
      // Get knock time
      printTwoLines("Enter Knock Time", "In minutes");
      delay(TextDelay);
      lcd.clear();
      printTwoLines("To erase,", "Press *!");
      delay(TextDelay);
      lcd.clear();
      lcd.print(F("Press # if done"));
      delay(TextDelay);
      lcd.clear();
      lcd.print(F("Knock Time:"));
      lcd.setCursor(0, 1);
      // Store rise time as an int
      // Knock time shouldn't exceed 30 minutes
      knock1 = get_input(2);
      // Allow time for user to see entered time
      //delay(500);
      // Turn off the display to move on
      lcd.clear();
      
      // Safety
      if (knock1>30){
        knock1 = 30;
      
        printTwoLines("The knock time", "is too long,");
        delay(TextDelay);
        lcd.clear();
        printTwoLines("It has been set", "to 30 mins.");
        delay(TextDelay);
        lcd.clear();
      }
        
      // Get rise temperature
      if (knock1 == 0){
        printTwoLines("Sorry, you must", "enter a value.");
        delay(TextDelay);
        lcd.clear();
        myState = 4;
      }
      else{
        myState = 5;
      }
      break;
      
    case 5:
      // Get second rise time
      printTwoLines("Add Second Rise", "Time in minutes");
      delay(TextDelay);
      lcd.clear();
      printTwoLines("To erase,", "Press *!");
      delay(TextDelay);
      lcd.clear();
      lcd.print(F("Press # if done"));
      delay(TextDelay);
      lcd.clear();
      lcd.print(F("Rise Time 2:"));
      lcd.setCursor(0, 1);
      // Store rise time as an int
      // As the time only goes up to a few hours, there are a max of 3 digits
      rise2 = get_input(3);
      // Allow time for user to see entered time
      delay(500);
      // Turn off the display to move on
      lcd.clear();
      
      // Safety
      if (rise2>240){
        rise2 = 240;
      
        printTwoLines("The rise time", "is too long,");
        delay(TextDelay);
        lcd.clear();
        printTwoLines("It has been set", "to 4 hrs.");
        delay(TextDelay);
        lcd.clear();
      }
        
      // Get rise temperature
      if (rise2 == 0){
        printTwoLines("Sorry, you must", "enter a value.");
        delay(TextDelay);
        lcd.clear();
        myState = 5;
      }
      else{
        myState = 6;
      }
      break;
      
    case 6:
 // Get rise temperature
      printTwoLines("Add Second Rise", "Temp in Celsius");
      delay(TextDelay);
      lcd.clear();
      printTwoLines("To erase,", "Press *!");
      delay(TextDelay);      
      lcd.clear();
      lcd.print(F("Press # if done"));
      delay(TextDelay);
      lcd.clear();
      lcd.print(F("Rise Temp 2:"));
      lcd.setCursor(0, 1);
      // Store rise time as an int
      // As the temp only goes up to a few degrees, there are a max of 2 digits
      rise2_temp = get_input(2);
      // Allow time for user to see entered time
      delay(500);
      // Turn off the display to move on
      lcd.clear();
      
      
      // Safety
      if (rise2_temp>50){
        rise2_temp = 50;
      
        printTwoLines("The temperature", "is set too high,");
        delay(TextDelay);
        lcd.clear();
        printTwoLines("It has been set", "to 50 (C).");
        delay(TextDelay);
        lcd.clear();
      }
      // Get rise temperature
      if (rise2_temp == 0){
        printTwoLines("Sorry, you must", "enter a value.");
        delay(TextDelay);
        lcd.clear();
        myState = 6;
      }
      else{
        myState = 20;
      }
      break;
      
    case 20:
      lcd.clear();
      printTwoLines("Please add", "wet ingredients");
      delay(TextDelay);
      lcd.clear();
      printTwoLines("Print *", "when done!");
      key = numpad.waitForKey();
      while (key != '*'){
        key = numpad.waitForKey();
      }
      lcd.clear();
      myState = 7;
      break;
      
    case 7:
      lcd.clear();
      // First Mixing Stage! This includes a first kneading stage, the dough should be kneaded before rising for the first time
      lcd.print(F("Mixing Stage :)"));
        
      // Indicate time left
      // This returns the amount of time that has passed since the beginning of the program
      time_begin = millis();
      // This returns the amount of time that has passed since the beginning of the program
      time_end = millis();
      // Find time left
      lcd.setCursor(11,1);
      printTimeLeft(mix);
      // Loop through motor control until the set time runs out
      while((time_end - time_begin) < mix*60000) {
        // Control motor
        if ((time_end - time_begin) < mix*60000/4){
          // Clockwise spin, gradual speed increase to fast, short time
          analogWrite(motor_PWM, MEDIUM);
        }
        else if ((time_end-time_begin) < mix*60000/4*2){
          // Counter-clockwise spin, gradual speed increase to fast, short time
          analogWrite(motor_PWM, FAST);
        }
        else if ((time_end-time_begin) < mix*60000/4*3){
          // Clockwise spin, gradual speed increase to slow, longer
          analogWrite(motor_PWM, MEDIUM);
        }
        else {
          // Counter-clockwise spin, gradual speed increase to slow, longer
          analogWrite(motor_PWM, FAST);
        }
        // Indicate time left
        // This returns the amount of time that has passed since the beginning of the program
        time_end = millis();
        
        // Get time that has elapsed, print to LCD
        if ((time_end - time_last)>UpdateTime){
          lcd.setCursor(11,1);
          printTimeLeft(mix);
          time_last = time_end;
    
        }
      }
      // Turn motor off
      analogWrite(motor_PWM, OFF);
        
      // Indicate the process has finished
      lcd.clear();
      lcd.print(F("Done mixing!"));
      delay(TextDelay);
      lcd.clear();
      
      // Move on to next mixing stage
      myState = 21;
      break;
        
    case 21:
      lcd.clear();
      printTwoLines("Please add", "dry ingredients");
      delay(TextDelay);
      lcd.clear();
      printTwoLines("Print *", "when done!");
      key = numpad.waitForKey(); //not sure if need to add getKey() after this
      while (key != '*'){
        key = numpad.waitForKey();
      }
      lcd.clear();
      myState = 22;
      break;
      
    case 22:
      lcd.clear();
      // First Mixing Stage! This includes a first kneading stage, the dough should be kneaded before rising for the first time
      lcd.print(F("Mixing Stage 2"));
        
      // Indicate time left
      // This returns the amount of time that has passed since the beginning of the program
      time_begin = millis();
      // This returns the amount of time that has passed since the beginning of the program
      time_end = millis();
      // Find time left
      lcd.setCursor(11,1);
      printTimeLeft(mix2);
      // Loop through motor control until the set time runs out
      while((time_end - time_begin) < mix2*60000) {
          // Control motor
        if ((time_end - time_begin) < mix2*60000/4){
          // Clockwise spin, slow rotation
          analogWrite(motor_PWM, SLOW);
        }
        else if ((time_end-time_begin) < mix2*60000/4*2){
          // Counter-clockwise spin, gradual speed increase to fast, short time
          analogWrite(motor_PWM, MEDIUM);
        }
        else if ((time_end-time_begin) < mix2*60000/4*3){
          // Clockwise spin, gradual speed increase to slow, longer
          analogWrite(motor_PWM, SLOW);
        }
        else {
          // Counter-clockwise spin, gradual speed increase to slow, longer
          analogWrite(motor_PWM, SLOW);
        }
        // Indicate time left
        // This returns the amount of time that has passed since the beginning of the program
        time_end = millis();
          
        // Get time that has elapsed, print to LCD
        if ((time_end - time_last)>UpdateTime){
          lcd.setCursor(11,1);
          printTimeLeft(mix2);
          time_last = time_end;
    
        }
      }
      // Turn motor off
      analogWrite(motor_PWM, OFF);
        
      // Indicate the process has finished
      lcd.clear();
      lcd.print(F("Done mixing!"));
      delay(TextDelay);
      lcd.clear();
      
      // Move on to rising stage
      myState = 8;
      break;
      
    case 8:
      // First Rising Stage!
      lcd.clear();
      lcd.print(F("Rising Stage 1!"));
      delay(TextDelay);
      
      // Indicate time left
      // This returns the amount of time that has passed since the beginning of the program
      time_begin = millis();
      // This returns the amount of time that has passed since the beginning of the program
      time_end = millis();
      // Find time left, convert to minutes
      lcd.setCursor(11,1);
      printTimeLeft(rise1);
     
      // Loop through getting the temperature and turning on the coil until the set time runs out
      while((time_end - time_begin) < rise1*60000) {
        // Control temperature
        temp_control(rise1_temp);
          
        // Indicate time left
        // This returns the amount of time that has passed since the beginning of the program
        time_end = millis();
        
        // Get time that has elapsed, print to LCD
        if ((time_end - time_last)>UpdateTime){
          lcd.setCursor(0,1);
          lcd.print(temperature);
          lcd.print("C ");
          lcd.setCursor(11,1);
          printTimeLeft(rise1);
          time_last = time_end;
        }
      }
        
      // Indicate the process has finished
      lcd.clear();
      lcd.print(F("Done rising!"));
      lcd.display();
      delay(TextDelay);
      lcd.clear();
                        
      // Move on to knock stage
      myState = 9;
      break;
      
    case 9:
      // Knock Stage
      lcd.clear();
      // This stage has the purpose of evening out the fluffiness of the bread: the bubbles are knocked down, then more are allowed to be made.
      lcd.print(F("Knocking Stage!"));
      
      // Indicate time left
      // This returns the amount of time that has passed since the beginning of the program
      time_begin = millis();
      // This returns the amount of time that has passed since the beginning of the program
      time_end = millis();
      // Find time left, convert to minutes
      lcd.setCursor(11,1);
      printTimeLeft(knock1);
               
      // Loop through motor control until the set time runs out
      while((time_end - time_begin) < knock1*60000) {
        // Control motor
        // Clockwise spin, gradual speed increase to fast, short time
        analogWrite(motor_PWM, SLOW);
        
        // Get time that has elapsed, print to LCD
        // This returns the amount of time that has passed since the beginning of the program
        time_end = millis();
        
        // Get time that has elapsed, print to LCD
        if ((time_end - time_last)>UpdateTime){
          lcd.setCursor(11,1);
          printTimeLeft(knock1);
          time_last = time_end;
        }
      }
      // Turn motor off
      analogWrite(motor_PWM, OFF);
        
      // Indicate the process has finished
      lcd.clear();
      lcd.print(F("Done knocking!"));
      lcd.display();
      lcd.clear();
      delay(TextDelay);
        
      // Move on to second rise time
      myState = 10;
      break;
      
    case 10:
      // Second Rise Time
      lcd.clear();
      lcd.print(F("Rising Stage 2!"));
              
      // Indicate time left
      // This returns the amount of time that has passed since the beginning of the program
      time_begin = millis();
      // This returns the amount of time that has passed since the beginning of the program
      time_end = millis();
      // Find time left
      lcd.setCursor(11,1);
      printTimeLeft(rise2);
               
      // Loop through getting the temperature and turning on the coil until the set time runs out
      while((time_end - time_begin) < rise2*60000) {
        // Control temperature
        temp_control(rise2_temp);
        
        time_end = millis();
        // Get time that has elapsed, print to LCD
        if ((time_end - time_last)>UpdateTime){
          lcd.setCursor(0,1);
          lcd.print(temperature);
          lcd.print("C ");
          lcd.setCursor(11,1);
          printTimeLeft(rise2);
          time_last = time_end;
        }
      }
        
      // Indicate the process has finished
      lcd.clear();
      lcd.print(F("Done rising!"));
      delay(TextDelay);
      lcd.clear();
        
      // Move on to final stage
      myState = 11;
      break;
      
    case 11:
      // Final Stage!!
      lcd.clear();
      // Some buzzer
      printTwoLines("Your dough is", "ready to cook!");
      delay(10000);
      lcd.clear();
      myState = 0;
      break;    
      
    default:
      // Some default
      myState = 0;
      break;
    }
}

// Function to print remaining time 
void printTimeLeft(int operation){
  // Find time left, convert to minutes
  time_left = (float)operation - (float)(((time_end - time_begin))/60000);
  // If the time left is over an hour, don't bother adding seconds (just hours and minutes)
  if (time_left > 60){
    int hours = (int) time_left/60;
    // Print hours left
    lcd.print(itoa(hours, time_left_string, 10));
    lcd.print("h");
    if ((time_left - hours)*60 < 10){
      lcd.print('0');
    }
    // Print minutes
    lcd.print(itoa((int)(time_left - hours)*60, time_left_string, 10));
    lcd.print("m");
  }
  else{
    // Less than an hour left, write minutes and seconds
    int minutes = (int) time_left;
    lcd.print(itoa(minutes, time_left_string, 10));
    lcd.print(":");
    if ((time_left - minutes)*60 < 10){
      lcd.print('0');
    }
    lcd.print(ltoa((time_left - minutes)*60, time_left_string, 10));
  }
}
            
            
// Beep function
void beep(int pin, int delaytime, int freq){
  int time_init = millis();
  int time_now;
  while ((time_now-time_init)<delaytime){
    digitalWrite(pin, HIGH);
    delay(1/(freq*1000));
    digitalWrite(pin, LOW);
    delay(1/(freq*1000));
    time_now = millis();
  }
}

// Function to control temperature
void temp_control(int set_temp) {
  // Initialize variables
  float temp = 0;
        
  // Control Temperature
  // Check if temperature above the set temperature, otherwise heat up
  // For this portion of code: can replace second "get_temp" with "OneWire  ds(2);" before "temp= get_temp"
  // to recreate the 'ds' variable everytime the loop is restarted (instead of having a gobal variable).
  // This is important because the function "get_temp" will otherwise keep looking for 'all' the temperature
  // sensors until it reaches 0. So a second "get_temp" is neceesary to 'absorb' the returned 0 when the
  // "get_temp" function realizes there is only one temperature sensor. Alternatively having a local variable
  // 'ds' also solves this problem.
  temp = get_temp();
  get_temp();
  //Serial.print(temp);
  if (temp == 0) {
    // Indicates some error
    Serial.print("Some Error Ocurred...\n");
  }
  else if (temp < (set_temp-TemperatureTolerance)) {
    // Turn on led
    digitalWrite(heat_coil, HIGH);   // turns the heating element on
  }
  else if (temp > (set_temp+TemperatureTolerance)){
    digitalWrite(heat_coil, LOW);    // turns the heating element off
  }
}

#ifndef SERIAL_INPUT_DBG
// Create function to store the keypad intput as some integer for time, temperature, etc.
int get_input(int k){
  // k indicates the allowed number of numbers to store (2 for temperature, 3 for time)
  // Initiate counter
        int i = 0;
        char key;
        char key_string[5] = {NULL};
        // NULL terminate the string
        // Get first key
        key = numpad.waitForKey();
        int output;
        
        // Get Time: loop through keys pressed as long as the 'start over' key is not pressed and number of keys pressed does not exceed maximum
        while (i < k+1){
          // When "enter" '#' is pressed need to fill in the rest of the string with NULLs
          if(key == '#'){
            while (i < k){
            // Set last characters in the array to null
            key_string[i] = NULL;
            i++; // can I do this?
            }
            i++;
          }
          else if (key == '*'){
            if (i != 0){
              // When "erase" '*' is pressed then remove one i value and move cursor back by one
              i--;
              lcd.setCursor(i, 1);
              lcd.print(' ');
              lcd.setCursor(i, 1);
            }
            // Ignore erase command if there is nothing to erase...
            key = numpad.waitForKey();
          }
          else{
            if (i == k){
              i++;
            }
            else{
              lcd.print(key);
              key_string[i] = key;
              i++;
              key = numpad.waitForKey();
            }
          }
        }
        
        // Store output as an int and return it
        output = atoi(key_string); // Check syntax
        delay(500);
        return output;
}
#endif

// Function to control LCD screen
void scrollAndPrint( char * text ){
  
  lcd.setCursor(16,0);
  lcd.print(text);
  int i = 0;
  
  while (text[i] != NULL){
    lcd.scrollDisplayLeft();
    delay(500);
    i++;
  }
  
  lcd.clear();
  
}

// OneWire DS18S20, DS18B20, DS1822 Temperature Example
//
// http://www.pjrc.com/teensy/td_libs_OneWire.html
//
// The DallasTemperature library can do all this work for you!
// http://milesburton.com/Dallas_Temperature_Control_Library

float get_temp() {
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;
  
  ds.reset_search();
  if ( !ds.search(addr)) {
    //Serial.println("No more addresses.");
    //Serial.println();
    ds.reset_search();
    delay(250);
    return 0;
  }
  
  //Serial.print("ROM =");
  for( i = 0; i < 8; i++) {
    //Serial.write(' ');
    //Serial.print(addr[i], HEX);
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
      //Serial.println("CRC is not valid!");
      return 0;
  }
  Serial.println();
 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      //Serial.println("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      //Serial.println("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      //Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      //Serial.println("Device is not a DS18x20 family device.");
      return 0;
  } 

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  //Serial.print("  Data = ");
  //Serial.print(present, HEX);
  //Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    //Serial.print(data[i], HEX);
    //Serial.print(" ");
  }
  //Serial.print(" CRC=");
  //Serial.print(OneWire::crc8(data, 8), HEX);
  //Serial.println();

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;
  //Serial.print("  Temperature = ");
  //Serial.print(celsius);
  //Serial.print(" Celsius, ");
  //Serial.print(fahrenheit);
  //Serial.println(" Fahrenheit");
  temperature = celsius;
  return celsius;
}

// Some sample ISR code
// Taking care of some special events.
void keypadEvent_num(KeypadEvent key){
    switch (numpad.getState()){
    case PRESSED:
        if (key == '#') {
          // When the pound key is pressed, start over.
          myState = 0;
        }
        else {
          // Indicate a key has been pressed
        }
        break;

    case RELEASED:
        if (key == '*') {
          // Start over as soon as key is released            
        }
        else {
          // Store the value of the key somewhere
        }
        break;

    case HOLD:
        if (key == '*') {
        // Special erase case?
        }
        break;
    }
}

int get_serial_input(int k){
  char buffer[5] = {0};
  
  Serial.println("Input Here");
  Serial.readBytesUntil('#', buffer, k+1);
  Serial.print(buffer);
  Serial.println(" was read");
  lcd.setCursor(0, 1);
  lcd.print(atoi(buffer));
  
  return atoi(buffer);
}

int get_some_serial_input(){
  while (!Serial.available()){
  }
  return Serial.read();
}
