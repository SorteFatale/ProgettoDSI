#include <LiquidCrystal.h>

LiquidCrystal lcd(22, 21, 5, 18, 23, 19);

int rs = 22;
int enable = 21;
int d4=5;
int d5=18;
int d6=23;
int d7=19;

//PIN VVD required
//PIN V0 a GND 
// Anodo a VDD
// Katodo a GND

void setup() {
  lcd.begin(16, 2);
  // you can now interact with the LCD, e.g.
  lcd.print("Hello World!");
}

void loop() {
  lcd.setCursor(0, 0);
  lcd.print("Hello World!");

  lcd.setCursor(0, 1);
  lcd.print("DIO PORCO!");
  
}