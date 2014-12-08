/*
 The Heat Sensor Wire:
 * connected to pin 10
 * using parasitic power for now
 
 Heat Coil
 * Connect to pin 13
 The LCD circuit:
 * hd44780 16x2
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
 
 The Keypad circuit:
 * for now, rows: 9, 8, 7, 6
 * for now columns: A0, A1, A2
 *
 */

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

#define ROWS 4 //four rows
#define COLS 3 //three columns

// Define keypad structure (as found on Sparkfun)
char numberKeys[ROWS][COLS] = {
    { '1','2','3' },
    { '4','5','6' },
    { '7','8','9' },
    { '*','0','#' }
};


// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// Set up pins to keypad

byte rowPins[ROWS] = {9, 8, 7, 6}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {A2, A1, A0}; //connect to the column pinouts of the keypad

// Create new keypad, a number pad.
Keypad numpad = Keypad( makeKeymap(numberKeys), rowPins, colPins, sizeof(rowPins), sizeof(colPins) );

// Create some values to store:
// Preset mixing time: 5 minutes
int mix = 5;
// Rise time
int rise1 = 30;
// Rise temperature
int rise1_temp = 40;
// Knock time
int knock1 = 5;
// Second rise time
int rise2 = 30;
// Second rise temperature
int rise2_temp = 40;
// To keep time
unsigned long time_begin;
unsigned long time_end;
float time_left;


// Set a State variable
int myState = 0;
// Key value variable
char key;

// Initialize functions
// Temperature Control Fucntion
void temp_control(int set_temp);
// Temperature Sensor Function
float get_temp();
// Keypad Functions
int get_input(int k);

void setup() {
  
  // Set bit rate
  Serial.begin(9600);
  
  // Initialize heat coil as output
  pinMode(heat_coil, OUTPUT);
  // Initialize keypad
  numpad.begin( makeKeymap(numberKeys) );
  // Add event listener: triggers an event if the keypad is used
  numpad.addEventListener(keypadEvent_num);
  // Sets the amount of time button needs to be pressed to trigger the "HOLD" state
  numpad.setHoldTime(100);
  
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  
}

void loop() {
  
  switch(myState){
    
      case 0:
        // Initialization message
        // Print a message to the LCD.
        lcd.print("Hannah's BreadMaker <3");
        // Turn on the display:
        lcd.display();
        delay(500);
        // Turn off the display:
        lcd.noDisplay();
        delay(500);
        myState = 1;
        
      case 1:
        // Idle state
        lcd.print("Press * to Start!");
        lcd.display(); //not sure if I need this
        key = numpad.waitForKey(); //not sure if need to add getKey() after this
        if (key == '*'){
          myState = 2;
        }
        // Turn off the display:
        lcd.noDisplay();
        
      case 2:
        // Ask for rise time
        lcd.print("To start over press *./n");
        lcd.display();
        delay(1000);
        lcd.noDisplay();
    
        // Get rise time
        lcd.print("Enter Rise Time (min),/n");
        lcd.print("Followed by #: /n");
        lcd.display(); // Do I need to write this?
        
       // Store rise time as an int
        // As the time only goes up to a few hours, there are a max of 3 digits, so we input 2 into the function
        rise1 = get_input(2);
        
        // Turn off the display to move on
        lcd.noDisplay();
        
        // Get rise temperature
        myState = 3;
        
      case 3:
        // Ask for rise temperature
        lcd.print("Enter Rise Temp (C),/n");
        lcd.print("Followed by #: /n");
        lcd.display(); // Do I need to write this?
        
        // Store rise temp as an int
        // As the temperature only goes up to 99 C, there are a max of 2 digits, so we input 1 into the function
        rise1_temp = get_input(1);
        
        // Turn off the display to move on
        lcd.noDisplay();
        
        // Get knock time
        myState = 4;
        
      case 4:
        // Ask for knock time
        lcd.print("Enter Knock Time (min),/n");
        lcd.print("Followed by #: /n");
        lcd.display(); // Do I need to write this?
        
        // Store knock time as an int
        // As the time only goes up to a few hours, there are a max of 3 digits, so we input 2 into the function
        knock1 = get_input(2);
        
        // Turn off the display to move on
        lcd.noDisplay();
        
        // Get second rise time
        myState = 5;
        
      case 5:
        // Ask for second rise time
        lcd.print("Enter Second Rise Time (min),/n");
        lcd.print("Followed by #: /n");
        lcd.display(); // Do I need to write this?
        
        // Store second rise time as an int
        // As the time only goes up to a few hours, there are a max of 3 digits, so we input 2 into the function
        rise2 = get_input(2);
        
        // Turn off the display to move on
        lcd.noDisplay();
        
        // Get second rise temp
        myState = 6;
        
      case 6:
        // Ask for second rise temp
        lcd.print("Enter Second Rise Temp (C),/n");
        lcd.print("Followed by #: /n");
        lcd.display(); // Do I need to write this?
        
        // Store second rise temp as an int
        // As the temperature only goes up to 99 C, there are a max of 2 digits, so we input 1 into the function
        rise2_temp = get_input(1);
        
        // Turn off the display to move on
        lcd.noDisplay();
        
        // Move on to next stage: Start Mixing
        myState = 7;
      
      case 7:
        // First Mixing Stage!
        lcd.print("Mixing!/n");
        lcd.print("Time Left:");
        // Indicate time left
        lcd.display();
        
        // Indicate time left
        // This returns the amount of time that has passed since the beginning of the program
        time_begin = millis();
        // This returns the amount of time that has passed since the beginning of the program
        time_end = millis();
        // Find time left, convert to minutes
        time_left = (float)mix - (float)(((time_end - time_begin))/60000);
        lcd.print('%.2f', time_left); // can I do this?
        lcd.display();
               
        // Loop through motor control until the set time runs out
        while((time_end - time_begin) < mix) {
          // Control motor
        
          // Clockwise spin, gradual speed increase to fast, short time
        
          // Counter-clockwise spin, gradual speed increase to fast, short time
        
          // Clockwise spin, gradual speed increase to slow, longer
        
          // Counter-clockwise spin, gradual speed increase to slow, longer

          // Get time that has elapsed, print to LCD
          lcd.print("Knocking Stage :)");
          lcd.print("Time Left: ");
          lcd.display();
        
          // Indicate time left
          // This returns the amount of time that has passed since the beginning of the program
          time_end = millis();
          // Find time left, convert to minutes
          time_left = (float)mix - (float)(((time_end - time_begin))/60000);
          lcd.print('%.2f', time_left);
          // Get new time elapsed - is this too redundant..?
          time_end = millis();
        }
        
        // Indicate the process has finished
        lcd.print("Done mixing!\n");
        lcd.display();
        delay(500);
        
        // Move on to first rise
        myState = 8;
        
      case 8:
        // First Rising Stage!
        lcd.print("First Rising Stage :)\n");
        lcd.print("Time Left: ");
        lcd.display();
        
        // Indicate time left
        // This returns the amount of time that has passed since the beginning of the program
        time_begin = millis();
        // This returns the amount of time that has passed since the beginning of the program
        time_end = millis();
        // Find time left, convert to minutes
        time_left = (float)rise1 - (float)(((time_end - time_begin))/60000);
        lcd.print('%.2f', time_left); // can I do this?
        lcd.display();
               
        // Loop through getting the temperature and turning on the coil until the set time runs out
        while((time_end - time_begin) < rise1) {
          // Control temperature
          temp_control(rise1_temp);

          // Get time that has elapsed, print to LCD
          lcd.print("First Rising Stage :)");
          lcd.print("Time Left: ");
          lcd.display();
        
          // Indicate time left
          // This returns the amount of time that has passed since the beginning of the program
          time_end = millis();
          // Find time left, convert to minutes
          time_left = (float)rise1 - (float)(((time_end - time_begin))/60000);
          lcd.print('%.2f', time_left);
          // Get new time elapsed - is this too redundant..?
          time_end = millis();
        }
        
        // Indicate the process has finished
        lcd.print("Done rising!\n");
        lcd.display();
        delay(500);
                        
        // Move on to knock stage
        myState = 9;
        
      case 9:
        // Knock Stage
        // This stage has the purpose of evening out the fluffiness of the bread: the bubbles are knocked down, then more are allowed to be made.
        lcd.print("Knocking Stage :)");
        lcd.print("Time Left:");
        // Indicate time left
        lcd.display();
        
        // Indicate time left
        // This returns the amount of time that has passed since the beginning of the program
        time_begin = millis();
        // This returns the amount of time that has passed since the beginning of the program
        time_end = millis();
        // Find time left, convert to minutes
        time_left = (float)knock1 - (float)(((time_end - time_begin))/60000);
        lcd.print('%.2f', time_left); // can I do this?
        lcd.display();
               
        // Loop through motor control until the set time runs out
        while((time_end - time_begin) < rise2) {
          // Control motor
         
          // Clockwise spin, gradual speed increase to slow
        
          // Counter-clockwise spin, gradual speed increase to slow

          // Get time that has elapsed, print to LCD
          lcd.print("Knocking Stage :)");
          lcd.print("Time Left: ");
          lcd.display();
        
          // Indicate time left
          // This returns the amount of time that has passed since the beginning of the program
          time_end = millis();
          // Find time left, convert to minutes
          time_left = (float)knock1 - (float)(((time_end - time_begin))/60000);
          lcd.print('%.2f', time_left);
          // Get new time elapsed - is this too redundant..?
          time_end = millis();
        }
        
        // Indicate the process has finished
        lcd.print("Done knocking!\n");
        lcd.display();
        delay(500);
        
        // Move on to second rise time
        myState = 10;
        
      case 10:
        // Second Rise Time
        lcd.print("Second Rising Stage :)");
        lcd.print("Almost Done:");
        // Indicate time left
        lcd.display();
        
        // Indicate time left
        // This returns the amount of time that has passed since the beginning of the program
        time_begin = millis();
        // This returns the amount of time that has passed since the beginning of the program
        time_end = millis();
        // Find time left, convert to minutes
        time_left = (float)rise2 - (float)(((time_end - time_begin))/60000);
        lcd.print('%.2f', time_left); // can I do this?
        lcd.display();
               
        // Loop through getting the temperature and turning on the coil until the set time runs out
        while((time_end - time_begin) < rise2) {
          // Control temperature
          temp_control(rise2_temp);

          // Get time that has elapsed, print to LCD
          lcd.print("Second Rising Stage :)");
          lcd.print("Time Left: ");
          lcd.display();
        
          // Indicate time left
          // This returns the amount of time that has passed since the beginning of the program
          time_end = millis();
          // Find time left, convert to minutes
          time_left = (float)rise2 - (float)(((time_end - time_begin))/60000);
          lcd.print('%.2f', time_left);
          // Get new time elapsed - is this too redundant..?
          time_end = millis();
        }
        
        // Indicate the process has finished
        lcd.print("Done rising!\n");
        lcd.display();
        delay(500);
        
        // Move on to final stage
        myState = 11;
        
        case 11:
        // Final Stage!!
        
        default:
        // Some default
        myState = 0;
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
  else if (temp < set_temp) {
    // Turn on led
    digitalWrite(heat_coil, HIGH);   // turns the heating element on
  }
  else {
    digitalWrite(heat_coil, LOW);    // turns the heating element off
  }
}

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
        
        // Get Time: loop through keys pressed as long as the 'start over' key '#' is not pressed
        while ((key != '*') | (i < k)){
          // When '*' is pressed, (or when '#' is) need to fill in the rest of the string with NULLs
          if((key == '*') | (key == '#')){
            while (i < k){
            // Set last characters in the array to null
            key_string[i] = NULL;
            i++; // can I do this?
            }
          }
          else{
            lcd.print(key);
            key_string[i] = key;
            i++;
            key = numpad.waitForKey();
          }
        }
        
        // Store output as an int and return it
        output = atoi(key_string); // Check syntax
        return output;
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
  
  if ( !ds.search(addr)) {
    Serial.println("No more addresses.");
    Serial.println();
    ds.reset_search();
    delay(250);
    return 0;
  }
  
  Serial.print("ROM =");
  for( i = 0; i < 8; i++) {
    Serial.write(' ');
    Serial.print(addr[i], HEX);
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return 0;
  }
  Serial.println();
 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      Serial.println("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      Serial.println("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      Serial.println("Device is not a DS18x20 family device.");
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

  Serial.print("  Data = ");
  Serial.print(present, HEX);
  Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.print(" CRC=");
  Serial.print(OneWire::crc8(data, 8), HEX);
  Serial.println();

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
  Serial.print("  Temperature = ");
  Serial.print(celsius);
  Serial.print(" Celsius, ");
  Serial.print(fahrenheit);
  Serial.println(" Fahrenheit");
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
