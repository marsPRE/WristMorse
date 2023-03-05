#include "M5StickCPlus.h"

#include <BleKeyboard.h>

BleKeyboard bleKeyboard;

int symbol_start = 0;
int pause_start = 0;
int word_pause_count = 0;
int letter_pause_count = 0;
int dit_length = 100;
int letter_pause_length = 4*dit_length;
int word_pause_length = 8*dit_length;
String word_cw = "";
bool beep = true;
bool ble = false;

const int ditPin = 26; // set the pin for dit button
const int dahPin = 25; // set the pin for dah button

int ditState = 0; // initialize the dit button state
int dahState = 0; // initialize the dah button state
bool prevDitState = false; // initialize the previous dit button state
bool prevDahState = false; // initialize the previous dah button state


String morsecode[48][2] = {
  {" ", " "}, {".----.", "'"}, {"-.--.-", ")"}, {"--..--", ","},
  {"-....-", "-"}, {".-.-.-", "."}, {"-..-.", "/"}, {"-----", "0"},
  {".----", "1"}, {"..---", "2"}, {"...--", "3"}, {"....-", "4"},
  {".....", "5"}, {"-....", "6"}, {"--...", "7"}, {"---..", "8"},
  {"----.", "9"}, {"---...", ":"}, {"-.-.-.", ";"}, {"..--..", "?"},
  {".-", "A"}, {"-...", "B"}, {"-.-.", "C"}, {"-..", "D"}, {".", "E"},
  {"..-.", "F"}, {"--.", "G"}, {"....", "H"}, {"..", "I"}, {".---", "J"},
  {"-.-", "K"}, {".-..", "L"}, {"--", "M"}, {"-.", "N"}, {"---", "O"},
  {".--.", "P"}, {"--.-", "Q"}, {".-.", "R"}, {"...", "S"}, {"-", "T"},
  {"..-", "U"}, {"...-", "V"}, {".--", "W"}, {"-..-", "X"}, {"-.--", "Y"},
  {"--..", "Z"}, {"..--.-", "_"},{"........", "backspace"}
};




void openMenu() {
  int menuSelection = 0; // initialize the menu selection to the first option
  int numMenuItems = 6; // the number of items in the menu
  String menuItems[6] = {"Dit Length","Beep","BLE Keyb","BtnA key","Touch paddle", "Exit Menu"}; // the items in the menu
  int menuX = 0; // the x position of the menu
  int menuY = 0; // the y position of the menu
  int menuItemHeight = 20; // the height of each menu item
  int menuItemPadding = 5; // the padding between each menu item
  int selectedColor = WHITE; // the color of the selected menu item
  int unselectedColor = BLUE; // the color of the unselected menu items
  
  M5.Lcd.fillScreen(BLACK);
  //M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextFont(2);

  while(true) {
    M5.Lcd.setCursor(menuX, menuY);
    for (int i = 0; i < numMenuItems; i++) {
      if (i == menuSelection) {
        M5.Lcd.setTextColor(selectedColor);
      } else {
        M5.Lcd.setTextColor(unselectedColor);
      }
      M5.Lcd.print(menuItems[i]);
      M5.Lcd.setCursor(menuX, menuY + ((i + 1) * menuItemHeight) + (i * menuItemPadding));
    }
    M5.update();
    
    if (M5.BtnB.wasPressed()) {
      menuSelection = (menuSelection + 1) % numMenuItems; // move to the next menu item
    }
    
    
    if (M5.BtnA.wasPressed()) {
      if (menuSelection == 0) {
        // Change Dit Length
        M5.Lcd.fillRect(0, 0, 200, 120, BLACK);
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.setTextColor(WHITE);
        M5.Lcd.print("Dit Length:");
        M5.Lcd.setCursor(80,0);
        M5.Lcd.print(dit_length);
        while (true) {
          M5.update();
          if (M5.BtnA.wasPressed()) {
            M5.Lcd.fillRect(80, 0, 200, 120, BLACK);
            dit_length += 20;
            if (dit_length > 200) {
                dit_length = 20;
                }
            M5.Lcd.setCursor(80,0);
            M5.Lcd.print(dit_length);
          }
          if (M5.BtnB.wasPressed()) {
            M5.Lcd.fillScreen(BLACK);
            letter_pause_length = 4*dit_length;
            word_pause_length = 8*dit_length;
            break;
          }

        }
      } else if (menuSelection == 1) {
        // Beep ON/OFF
        M5.Lcd.fillRect(0, 0, 200, 120, BLACK);
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.setTextColor(WHITE);
        M5.Lcd.print("Beep:");
        while (true) {
          M5.update();
          if (M5.BtnA.wasPressed()) {
            M5.Lcd.fillRect(80, 0, 200, 120, BLACK);
            beep = !beep;
            M5.Lcd.setCursor(80,0);
            M5.Lcd.print(beep);
          }
          if (M5.BtnB.wasPressed()) {
            M5.Lcd.fillScreen(BLACK);
            break;
          }
        }
      }else if (menuSelection == 2) {
        // BLE Keyb ON/OFF
        M5.Lcd.fillRect(0, 0, 200, 120, BLACK);
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.setTextColor(WHITE);
        M5.Lcd.print("BLE Keyb:");
        while (true) {
          M5.update();
          if (M5.BtnA.wasPressed()) {
            M5.Lcd.fillRect(80, 0, 200, 120, BLACK);
            ble = !ble;
            M5.Lcd.setCursor(80,0);
            M5.Lcd.print(ble);
            if (ble == true){
              bleKeyboard.begin();
            }
          }
          if (M5.BtnB.wasPressed()) {
            M5.Lcd.fillScreen(BLACK);
            break;
          }
        }
      
      
      }else if (menuSelection == 3) {
        // BtnA Key
        M5.Lcd.fillScreen(BLACK);
        butonA_key();
    }else if (menuSelection == 4) {
        // Touch paddle
        M5.Lcd.fillScreen(BLACK);
        touch_paddle();
    }else if (menuSelection == 5) {
        // Exit Menu
        M5.Lcd.fillScreen(BLACK);
        break;
    }
    

  }
}}



void touch_paddle(){
  while (!M5.BtnB.wasPressed()) {
      M5.Lcd.setTextFont(4);
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.print(word_cw);
      M5.update();
      
      // read the current state of the dit button
      ditState = digitalRead(ditPin);

      // if the dit button state has changed
      if ((millis()-pause_start >= 2*dit_length && prevDitState) or (millis()-pause_start >= 4*dit_length)) {
        // if the button is pressed
        if (ditState == HIGH) {
          pause_start = millis();
          word_cw += ".";
          letter_pause_count = 0;
          word_pause_count = 0;
          if (beep == true){
            M5.Beep.tone(400,dit_length);
          }          
        prevDitState = true; // update the previous dit button state
        prevDahState = false; // update the previous dah button state
        }

      }
    // read the current state of the dah button
    dahState = digitalRead(dahPin);

    // if the dah button state has changed
    if ((millis()-pause_start >= 4*dit_length) or (millis()-pause_start >= 2*dit_length && prevDitState ) )   {
      // if the button is pressed
      if (dahState == HIGH) {
        pause_start = millis();
        word_cw += "-";
        letter_pause_count = 0;
        word_pause_count = 0;
        if (beep == true){
          M5.Beep.tone(400,3*dit_length);
        }
        prevDitState = false; // update the previous dit button state
        prevDahState = true; // update the previous dah button state
    }
 
  
  }
      if ((millis()) - pause_start >= letter_pause_length && letter_pause_count == 0) {
        //pause_start = 0;
        String morse_char = "";
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.print(word_cw);
        M5.Lcd.fillRect(0, 60, 100, 30, BLACK); // erase previous character
        
        for (int i = 0; i < 48; i++) {
          if (word_cw == morsecode[i][0]) {
            morse_char = morsecode[i][1];
            break;
          }
        }
        if (morse_char == "") {
          M5.Lcd.setCursor(0, 60);
          M5.Lcd.print("???");
          
        }  
        else if (morse_char == "backspace") {
          M5.Lcd.setCursor(0, 60);
          M5.Lcd.print(morse_char);
          if (ble == true){
          bleKeyboard.write(KEY_BACKSPACE);
          } 
        }
        else {
          M5.Lcd.setCursor(0, 60);
          M5.Lcd.print(morse_char);
          if (ble == true){
          bleKeyboard.write(morse_char.charAt(0));
                }
       }
       M5.Lcd.fillRect(0, 0, 100, 30, BLACK); // erase previous character
       word_cw = "";
       letter_pause_count += 1;     
    }
      
    if ((millis()) - pause_start >= word_pause_length && word_pause_count == 0) {
      String morse_char = " ";     
      if (ble == true){
      bleKeyboard.write(morse_char.charAt(0));
      }
      word_pause_count += 1;
      letter_pause_count += 1;
    }

  }
}


void butonA_key(){
  while (!M5.BtnB.wasPressed()) {
      M5.Lcd.setTextFont(4);
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.print(word_cw);
      M5.update();
      
      if (M5.BtnA.wasPressed()) {
        symbol_start = millis();
        if (beep == true){
          M5.Beep.tone(400);
          M5.Beep.setVolume(10);
          M5.Beep.update();
        }
      }
  
  
      if (M5.BtnA.wasReleased()) {
        if ((millis() - symbol_start) <= dit_length) {
          word_cw += ".";
          } 
        else {
          word_cw += "-";
          }
        pause_start = millis();
        letter_pause_count = 0;
        word_pause_count = 0;
        M5.Beep.setVolume(0);
        M5.Beep.tone(0);
        M5.Beep.update();
        }
  
  
      
      
      if ((millis()) - pause_start >= letter_pause_length && letter_pause_count == 0) {
        //pause_start = 0;
        String morse_char = "";
  
        
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.print(word_cw);
        M5.Lcd.fillRect(0, 60, 100, 30, BLACK); // erase previous character
        
        for (int i = 0; i < 48; i++) {
          if (word_cw == morsecode[i][0]) {
            morse_char = morsecode[i][1];
            break;
          }
        }
        if (morse_char == "") {
          M5.Lcd.setCursor(0, 60);
          M5.Lcd.print("???");
        } 
        else if (morse_char == "backspace") {
          M5.Lcd.setCursor(0, 60);
          M5.Lcd.print(morse_char);
          if (ble == true){
          bleKeyboard.write(KEY_BACKSPACE);
          } 
        }
        else{  
          
          M5.Lcd.setCursor(0, 60);
          M5.Lcd.print(morse_char);
          if (ble == true){
          bleKeyboard.write(morse_char.charAt(0));
                }
      }
       M5.Lcd.fillRect(0, 0, 100, 30, BLACK); // erase previous character
       word_cw = "";
       letter_pause_count += 1;
      
    }
  
    if ((millis()) - pause_start >= word_pause_length && word_pause_count == 0) {
      String morse_char = " ";     
      if (ble == true){
      bleKeyboard.write(morse_char.charAt(0));
      }
      word_pause_count += 1;
    }
  
}



}



void setup() {
  pinMode(ditPin, INPUT); // set dit pin as input
  pinMode(dahPin, INPUT); // set dah pin as input
  
  M5.begin();
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextFont(6);
  M5.Lcd.setRotation(3);  
  

}

void loop(){
  //butonA_key();
  touch_paddle();
        
    
    if (M5.BtnB.wasPressed()) {
    openMenu();
  }
  
  
   
}
