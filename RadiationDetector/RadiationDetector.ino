/* From DanielDMlee.
   Original code is http://www.eleparts.co.kr/data/goods_old/design/product_file/Hoon/AN_GDK101_V1.0_I2C.pdf
*/

/*
  We will be using the I2C hardware interface on the Arduino in
  combination with the built-in Wire library to interface.

  Command List
  0xA0 :: Reset
  0xB0 :: Read Status
  0xB1 :: Read Measuring Time
  0xB2 :: Read Measuring Value (10min avg / 1min update)
  0xB3 :: Read Measuring Value (1min avg / 1min update)
  0xB4 :: Read Firmware Version

  Address Assignment
  Default Address :: 0x18
  A0 Open, A1 Short :: 0x19
  A0 Short, A1 Open :: 0x1A
  A0 Open, A1 Open :: 0x1B
*/

// Gamma Sensor's Example Interface
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2
#define PwmPin 11
#define TargetMin   10 // 10min

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

int addr = 0x18;
int day, hour, min, sec = 0;
byte buffer[2] = {0, 0};
int status = 0;
int FLAG = 0;

void setup() {
//Arduino Initialize
/***************************************************/
// Serial I/F setting
  Serial.begin(9600);
  Wire.begin();
  pinMode(LED_BUILTIN, OUTPUT);
/***************************************************/
// PWM setting
   pinMode(PwmPin, OUTPUT);
   
/***************************************************/
// OLED setting
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3c);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done

  // Clear the buffer.
  delay(1000);
  display.clearDisplay();
  delay(1000);
  
  // text display tests
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.setTextColor(WHITE, BLACK);
  delay(1000);

  //Read Firmware version
  Gamma_Mod_Read(0xB4);
  //Reset before operating the sensor
  Gamma_Mod_Read(0xA0);
  display.println();
  display.println();
  display.println("  2018.05.17");
  display.println("  Design by Daniel");
  display.display();
  delay(2000);
  // Clear the buffer.
  display.clearDisplay();
   //display.println("================================================");
}
void loop() {
  display.setCursor(0,0);
  display.setTextColor(BLACK, WHITE);
  display.println("    Gamma Sensing    \n");
  display.setTextColor(WHITE, BLACK);
  
  delay(450);
  //Read Statue, Measuring Time, Measuring Value
  Gamma_Mod_Read_Value();
  FLAG = !FLAG;
  digitalWrite(LED_BUILTIN, FLAG);
   //display.println("================================================");
}

void Gamma_Mod_Read_Value() {
  Gamma_Mod_Read(0xB0); // Read Status
  Gamma_Mod_Read(0xB1); // Read Measuring Time
  Gamma_Mod_Read(0xB2); // Read Measuring Value (10min avg / 1min update)
  Gamma_Mod_Read(0xB3); // Read Measuring Value (1min avg / 1min update)
  
  // make alarm every 10 min
  if (min == TargetMin) {
    while(1) {
      for(int i =0; i<5; i++)
      {
        analogWrite(PwmPin, 50);
        delay(100);
        analogWrite(PwmPin, 100);
        delay(100);
        analogWrite(PwmPin, 0);
        min=0;
      }
      delay(1000);
    }
  }
}

void Gamma_Mod_Read(int cmd) {
  /* Begin Write Sequence */
  Wire.beginTransmission(addr);
  Wire.write(cmd);
  Wire.endTransmission();
  /* End Write Sequence */
  delay(10);
  /* Begin Read Sequence */
  Wire.requestFrom(addr, 2);
  byte i = 0;
  while (Wire.available())
  {
    buffer[i] = Wire.read();
    i++;
  }
  /* End Read Sequence */

  /* View Results */
  Print_Result(cmd);
}
/*
  Calculation Measuring Time
  Format :: 0d 00:00:00 ( (day)d (hour):(min):(sec) )
*/
void Cal_Measuring_Time() {
  if (sec == 60) {
    sec = 0;
    min++;
  }
  if (min == 60) {
    min = 0;
    hour++;
  }
  if (hour == 24) {
    hour = 0;
    day++;
  }
  
  
   //display.println("M.Time");
  
   display.print(day);  display.print("d ");
  if (hour < 10)  display.print("0");
   display.print(hour);  display.print(":");
  if (min < 10)  display.print("0");
   display.print(min);  display.print(":");
  if (sec < 10)  display.print("0");
   display.println(sec);
  display.println("  ");
  display.display();
}

void Print_Result(int cmd) {
  float value = 0.0f;
  switch (cmd) {
    case 0xA0:
      if (buffer[0] == 1)  
      display.println("Reset Success.       ");
      else  
      display.println("Reset Fail(Ready Sat.) ");
      break;
    case 0xB0:
      switch (buffer[0]) {
        case 0:  display.println("Stat:Ready           "); break;
        case 1:  display.println("Stat:10min Waiting   "); break;
        case 2:  display.println("Stat:Normal          "); break;
      }
      status = buffer[0];
      switch (buffer[1]) {
        case 0:  display.println("VIB Stat:OFF         "); break;
        case 1:  display.println("VIB Stat:ON          "); break;
      }
      break;
    case 0xB1:
      if (status > 0) {
        sec++;
        Cal_Measuring_Time();
      }
      break;
    case 0xB2:
      value = buffer[0] + (float)buffer[1] / 100;
      display.print("10min avg:");
      display.print((float)value);
      display.println(" uSv/hr");
      break;
    case 0xB3:
      value = buffer[0] + (float)buffer[1] / 100;
      display.print("1min avg:");
      display.print((float)value);
      display.println("  uSv/hr");
      break;
    case 0xB4:
      display.print("FW Ver.:V");
      display.print(buffer[0]);
      display.print(".");
      display.print(buffer[1]);
      display.println("  ");
      break;
  }
  display.display();
}
