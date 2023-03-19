#include "M5StickCPlus.h"
#include <Arduino.h> // Include this for randomSeed and random functions
#include <BleKeyboard.h>
#include <EEPROM.h> // for storing settings
#include <WiFi.h>
#undef min // to resolve conflict between the min macro defined in the M5StickCPlus library's In_eSPI.h and the std::min function used in the painlessMesh library.
#include <esp_now.h>


BleKeyboard bleKeyboard;

// Use the broadcast MAC address for ESP-NOW
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Global variables
int symbol_start = 0;
int pause_start = 0;
int word_pause_count = 0;
int letter_pause_count = 0;
String word_cw = "";
bool beep = true;
bool ble = false;

// Pin definitions
const int ditPin = 26; // set the pin for dit button
const int dahPin = 25; // set the pin for dah button

// Button states
int ditState = 0; // initialize the dit button state
int dahState = 0; // initialize the dah button state
bool prevDitState = false; // initialize the previous dit button state
bool prevDahState = false; // initialize the previous dah button state
String decoded_text = "";
String mode_names[6] = {"Keyer", "BLE_keyboard", "TRX", "Koch Trainer", "Koch New Lesson", "Echo Trainer"};
int settings[3] = {15 , 0, 1}; //wpm ,lower_cases, Koch lesson
int dit_length = 1200 / settings[0];
int letter_pause_length = 4 * dit_length;
int word_pause_length = 8 * dit_length;

int mode = 0;

Button Btn_dit = Button(ditPin, true, 10);


//data structure for esp-now TRX
typedef struct struct_message {
    char data[32];
} struct_message;

// Create a variable to hold the data to be sent
struct_message myData;




// Morse code array
String morsecode[49][2] = {
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
  {"--..", "Z"}, {"..--.-", "_"}, {"........", "backspace"},{"-.--.","<kn>"}
};

String Koch_lessons[18][2] = {
  {"1", "KM"},
  {"2", "RS"},
  {"3", "AU"},
  {"4", "FP"},
  {"5", "WT"},
  {"6", "LV"},
  {"7", "NI"},
  {"8", "HX"},
  {"10", "JZ"},
  {"11", "QY"},
  {"12", "CG"},
  {"13", "O5"},
  {"14", "94"},
  {"15", "38"},
  {"16", "27"},
  {"17", "16"},
  {"18", "0/"},
};



//Callback function for esp-now
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Last Packet Send Status: ");
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("Delivery success");
  } else {
    Serial.println("Delivery failed");
  }
}



void onDataReceived(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
  // Cast the incoming data to your data structure
  struct_message *receivedData = (struct_message *)incomingData;

  // Print the received data to the Serial monitor
  Serial.print("Received data from: ");
  for (int i = 0; i < 6; i++) {
    Serial.print(mac_addr[i], HEX);
    if (i < 5) {
      Serial.print(":");
    }
  }
  Serial.println();

  Serial.print("Data:");
  Serial.print(receivedData->data);
  M5.Lcd.fillRect(0, 80, 240, 80, BLACK); // erase previous character
  //decoded_text = padString(decoded_text, 16);
  M5.Lcd.setCursor(0, 80);
  M5.Lcd.print(receivedData->data);


}





// Function to pad a string with spaces
String padString(String str, int maxChars) {
  int strLength = str.length();
  if (strLength >= maxChars) {
    return str.substring(strLength - maxChars);
  } else {
    String spaces = "";
    for (int i = 0; i < maxChars - strLength; i++) {
      spaces += " ";
    }
    return spaces + str;
  }
}

// Function to play
void playMorseCode(String input) {
  Serial.print("playing:");
  Serial.println(input);
  for (int i = 0; i < input.length(); i++) {
    char currentChar = input.charAt(i);
    String morse = "";

    // Find the Morse code for the current character
    for (int j = 0; j < 48; j++) {
      if (morsecode[j][1].charAt(0) == currentChar) {
        morse = morsecode[j][0];
        Serial.println(morse); // Print dot on serial console
        break;
      }
    }

    // Play the Morse code using the buzzer
    for (int k = 0; k < morse.length(); k++) {
      M5.update();
      char symbol = morse.charAt(k);
      if (symbol == '.') {
        M5.Beep.tone(400, dit_length);
        delay(dit_length);
        M5.Beep.tone(0);
        M5.Beep.update();
        Serial.print("."); // Print dot on serial console
      } else if (symbol == '-') {
        M5.Beep.tone(400, 3 * dit_length);
        delay(3 * dit_length);
        M5.Beep.tone(0);
        M5.Beep.update();
        Serial.print("-"); // Print dash on serial console
      }

      // Add a pause between symbols
      if (k < morse.length() - 1) {
        delay(dit_length);
      }
    }

    // Add a pause between letters
    if (i < input.length() - 1) {
      delay(3 * dit_length);
    }
  }

  // Add a pause between words
  delay(7 * dit_length);
}


//Function to print settings to screen
void print_status(){
  M5.Lcd.setTextFont(2);
  M5.Lcd.fillRect(110, 0, 100, 60, BLACK); // erase previous character
  M5.Lcd.setCursor(110,0);
  M5.Lcd.print(settings[0]);
  M5.Lcd.print(" WPM");
  M5.Lcd.setCursor(110,20);
  M5.Lcd.print(mode_names[mode]);
  if ((mode_names[mode] == "Koch Trainer")||(mode_names[mode] == "Koch New Lesson") || (mode_names[mode] == "Echo Trainer")) {
    M5.Lcd.setCursor(110,40);
    M5.Lcd.print("lesson: ");
    M5.Lcd.print(settings[2]);
  }

}



// get a random character from the selected lesson and all lessons before
String GetLessonCharacters(int lessonNumber, bool previous_char) {
  String characters = "";

  // Concatenate all the characters from the current and previous lessons
  if (previous_char) {
    for (int i = 0; i < lessonNumber && i < sizeof(Koch_lessons) / sizeof(Koch_lessons[0]); i++) {
      characters += Koch_lessons[i][1];
    }
  } else {
    characters = Koch_lessons[lessonNumber][1];
  }
  Serial.println(characters);

  // Randomly select a character from the concatenated string
  int charIndex = random(characters.length());
  char selectedChar = characters.charAt(charIndex);
  return String(selectedChar);
}


void loadSettingsFromEEPROM() {
  int address = 0; // Start address to load the settings

  for (int i = 0; i < sizeof(settings) / sizeof(settings[0]); i++) {
    settings[i] = EEPROM.read(address);
    address++;
  }
}

void saveSettingsToEEPROM() {
  int address = 0; // Start address to save the settings

  for (int i = 0; i < sizeof(settings) / sizeof(settings[0]); i++) {
    EEPROM.write(address, settings[i]);
    address++;
  }

  EEPROM.commit(); // Commit the changes to the EEPROM
}




// Function to open the Settings menu
void openSettingsMenue(){
  int menuSelection = 0; // initialize the menu selection to the first option
  int numMenuItems = 4; // the number of items in the menu
  String menuItems[] = {
    "WPM",
    "Capital Letters",
    "Koch_lesson",
    "Exit Menu"
  };
  M5.Lcd.setTextFont(2);
  while (true) { // loop until a valid selection is made
    M5.Lcd.fillScreen(BLACK);; // clear the screen
    M5.Lcd.setCursor(0, 0); // set cursor to top left corner

    // Display menu items
    for (int i = 0; i < numMenuItems; i++) {
      if (i == menuSelection) { // if the current item is selected
        M5.Lcd.print(">"); // print an arrow to indicate selection
      } else {
        M5.Lcd.print(" "); // otherwise, print a space
      }
      M5.Lcd.print(menuItems[i]); // print the menu item text
      if (i < (numMenuItems - 1)){ //if not exit, print setting value
        M5.Lcd.print(": ");
        M5.Lcd.println(settings[i]);
      }
      
    }

    M5.update(); // update the M5StickCPlus
    if (M5.BtnB.wasPressed()) { // if the B button was pressed
      menuSelection++; // increment the menu selection
      if (menuSelection >= numMenuItems) { // if the selection is out of bounds
        M5.Lcd.fillScreen(BLACK);; // clear the screen
        print_status();
        menuSelection = 0; // wrap around to the first item
      }
    } else if (M5.BtnA.wasPressed()) { // if the A button was pressed
      if (menuItems[menuSelection] == "WPM"){
        if (settings[0]<25){ 
        settings[0] += 1;
        } else {
          settings[0] = 10;
        }
        dit_length = 1200 / settings[0];
        letter_pause_length = 4 * dit_length;
        word_pause_length = 8 * dit_length;

      }
      if (menuItems[menuSelection] == "Koch_lesson"){
        if (settings[2]<19){ 
        settings[2] += 1;
        } else {
          settings[2] = 1;
        }

      }
      if (menuSelection == numMenuItems - 1) { // if the "Exit Menu" item is selected
        saveSettingsToEEPROM();
        break; // exit the menu loop
      }
      

    }


  }
  delay(200); // short delay to prevent button debouncing issues
}


// Function to open the menu
void openMenu() {
  M5.Lcd.fillScreen(BLACK); // clear the screen
  int menuSelection = 0; // initialize the menu selection to the first option
  int numMenuItems = 8; // the number of items in the menu

    // the number of items in the menu
  String menuItems[] = {
    "Keyer Mode",
    "BLE Keyboard Mode",
    "TRX Mode",
    "Koch Trainer Mode",
    "Koch New Lesson Mode",
    "Echo Trainer Mode",
    "Settings",
    "Exit Menu"
  };
  M5.Lcd.setTextFont(2);
  while (true) { // loop until a valid selection is made
    M5.Lcd.fillScreen(BLACK);; // clear the screen
    M5.Lcd.setCursor(0, 0); // set cursor to top left corner

    // Display menu items
    for (int i = 0; i < numMenuItems; i++) {
      if (i == menuSelection) { // if the current item is selected
        M5.Lcd.print(">"); // print an arrow to indicate selection
      } else {
        M5.Lcd.print(" "); // otherwise, print a space
      }
      M5.Lcd.println(menuItems[i]); // print the menu item text
    }

    M5.update(); // update the M5StickCPlus

    // Check for button presses
    if (M5.BtnB.wasPressed()) { // if the B button was pressed
      menuSelection++; // increment the menu selection
      if (menuSelection >= numMenuItems) { // if the selection is out of bounds
        M5.Lcd.fillScreen(BLACK); // clear the screen
        print_status();
        menuSelection = 0; // wrap around to the first item
      }
    } else if (M5.BtnA.wasPressed()) { // if the A button was pressed
      if (menuSelection == numMenuItems - 1) { // if the "Exit Menu" item is selected
        M5.Lcd.fillScreen(BLACK);
        print_status(); 
        break; // exit the menu loop
      } else if (menuSelection == numMenuItems - 2) { // if the "Settings Menu" item is selected
        openSettingsMenue();
      } else if (menuItems[menuSelection] == "BLE Keyboard Mode") { // if the "BLE KEyboard" item is selected
        bleKeyboard.begin();
        mode = menuSelection; // set the mode based on the selected menu item
        M5.Lcd.fillScreen(BLACK);; // clear the screen
        print_status();
        break; // exit the menu loop
      }
      
      
      else {
        mode = menuSelection; // set the mode based on the selected menu item
        M5.Lcd.fillScreen(BLACK);; // clear the screen
        print_status();
        break; // exit the menu loop
      }
    }
    delay(100); // short delay to prevent button debouncing issues
  }
}


void print_decoded_text(String decoded_text){
    M5.Lcd.fillRect(0, 60, 240, 80, BLACK); // erase previous character
    decoded_text = padString(decoded_text, 16);
    M5.Lcd.setCursor(0, 60);
    M5.Lcd.print(decoded_text);
}

String touch_paddle(){
  M5.Lcd.setTextFont(4);
  //pause_start = millis();
  //letter_pause_count = 1;


  while(true){
    
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.print(word_cw);
    M5.update();

    // read the current state of the dit button
    ditState = digitalRead(ditPin);

    //return to main loop if button B is pressed
    if (M5.BtnB.isPressed()) { 
      return("");
    }

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
            M5.Lcd.fillRect(0, 40, 240, 80, BLACK); // erase previous character
            
            for (int i = 0; i < 49; i++) {
            if (word_cw == morsecode[i][0]) {
                morse_char = morsecode[i][1];
                break;
            }
            }
            if (morse_char == "") {
            M5.Lcd.setCursor(0, 40);
            M5.Lcd.print("???");
            //return morse_char;
            }  
            else {
            M5.Lcd.setCursor(0, 40);
            M5.Lcd.print(morse_char);
            //return morse_char;  
            }
        M5.Lcd.fillRect(0, 0, 100, 30, BLACK); // erase previous character
        word_cw = "";
        letter_pause_count += 1;
        return morse_char;     
        }
        
        if ((millis()) - pause_start >= word_pause_length && word_pause_count == 0) {
        String morse_char = " ";

        word_pause_count += 1;
        letter_pause_count += 1;
        return morse_char; 
        }
  }
}


void buton_key(){

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
        M5.Lcd.fillRect(0, 40, 100, 30, BLACK); // erase previous character
        
        for (int i = 0; i < 49; i++) {
          if (word_cw == morsecode[i][0]) {
            morse_char = morsecode[i][1];
            break;
          }
        }
        if (morse_char == "") {
          M5.Lcd.setCursor(0, 40);
          M5.Lcd.print("???");
        } 
        else if (morse_char == "backspace") {
          M5.Lcd.setCursor(0, 40);
          M5.Lcd.print(morse_char);
          if (ble == true){
          bleKeyboard.write(KEY_BACKSPACE);
          } 
        }
        else{  
          
          M5.Lcd.setCursor(0, 40);
          M5.Lcd.print(morse_char);
          decoded_text = padString(decoded_text+morse_char, 16);
          M5.Lcd.setCursor(0, 80);
          M5.Lcd.print(decoded_text);
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
      decoded_text = padString(decoded_text+morse_char, 16);
      M5.Lcd.setCursor(0, 80);
      M5.Lcd.print(decoded_text);
      if (ble == true){
      bleKeyboard.write(morse_char.charAt(0));
      }
      word_pause_count += 1;
    }
  
}












void setup() {
  pinMode(ditPin, INPUT); // set dit pin as input
  pinMode(dahPin, INPUT); // set dah pin as input
  

  
  M5.begin();
  Serial.begin(9600); // Start serial communication at 9600 baud rate
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setRotation(3);
  M5.Lcd.setTextFont(2);
  M5.Lcd.setCursor(110,0);
  print_status();
  randomSeed(analogRead(0)); // Seed the random number generator
  EEPROM.begin(512); // Initialize the EEPROM with a size of 512 bytes
  loadSettingsFromEEPROM(); // Load the settings array from the EEPROM
  if (settings[0] == 255){
      settings[0] = 15;
      settings[2] = 1 ;
      saveSettingsToEEPROM();
  }
  
  dit_length = 1200 / settings[0];
  letter_pause_length = 4 * dit_length;
  word_pause_length = 8 * dit_length;
  
// Set the device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Set up the ESP-NOW callback function
  esp_now_register_send_cb(OnDataSent);

  // Add the receiver's MAC address
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  esp_now_register_recv_cb(onDataReceived);




}

void loop(){
  M5.update(); // update the M5StickCPlus
  String morse_char;
  String send_char;
  int trials = 0;
  bool correct = false;






  if ((M5.BtnB.wasPressed()) || (M5.BtnB.isPressed())) {
    M5.Lcd.setCursor(110,40);
    M5.Lcd.print("menue");
    openMenu();

  }



  //butonA_key();

  switch (mode) {
  case 0: // touch keyer
    
    morse_char = touch_paddle();
    if (morse_char == "backspace") {
        decoded_text.remove(decoded_text.length()-1,1);
    }
    else {
      decoded_text = decoded_text+=morse_char;
    }
    print_decoded_text(decoded_text);
    break;
  case 1: //ble keyboard
    morse_char = touch_paddle();
    if (morse_char == "backspace") {
      decoded_text.remove(decoded_text.length()-1,1);
      bleKeyboard.write(KEY_BACKSPACE);
    }
    else {
      decoded_text = decoded_text+=morse_char;
      bleKeyboard.write(morse_char.charAt(0));
    }
    print_decoded_text(decoded_text);
    break;
  case 2: // TRX
    morse_char = touch_paddle();
    if (morse_char == "backspace") {
        decoded_text.remove(decoded_text.length()-1,1);
    }
    else if(morse_char == "<kn>"){
        // Set the data you want to send
      strncpy(myData.data, decoded_text.c_str(), sizeof(myData.data) - 1); // Use strncpy to avoid overflow
      myData.data[sizeof(myData.data) - 1] = '\0'; // Ensure null termination

      // Send the data
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));

      if (result == ESP_OK) {
        Serial.println("Sent with success");
      } else {
        Serial.println("Error sending the data");
      }
      
      
      
      
      decoded_text = "";

    }  
    else {
      decoded_text = decoded_text+=morse_char;
    }
    print_decoded_text(decoded_text);
    break;
   
  
  case 3: //Koch Trainer
    //Serial.print("getting char:"); 
    morse_char = GetLessonCharacters(settings[2],true);
    playMorseCode(morse_char);
    delay(500);
    M5.Lcd.setTextFont(4);
    M5.Lcd.fillRect(0, 40, 40, 40, BLACK); // erase previous character
    M5.Lcd.setCursor(0, 40);
    M5.Lcd.print(morse_char);
    delay(1000);
    playMorseCode(morse_char);
    M5.Lcd.fillRect(0, 40, 40, 40, BLACK); // erase previous character
    delay(500);
    M5.Beep.tone(600);
    delay(50);
    M5.Beep.tone(0);
    delay(500);
    M5.Lcd.setTextFont(2);

    break;

case 4: //Koch New Lesson Trainer
    //Serial.print("getting char:"); 
    morse_char = GetLessonCharacters(settings[2],false);
    playMorseCode(morse_char);
    delay(500);
    M5.Lcd.setTextFont(4);
    M5.Lcd.fillRect(0, 40, 40, 40, BLACK); // erase previous character
    M5.Lcd.setCursor(0, 40);
    M5.Lcd.print(morse_char);
    delay(1000);
    playMorseCode(morse_char);
    M5.Lcd.fillRect(0, 40, 40, 40, BLACK); // erase previous character
    delay(500);
    M5.Beep.tone(600);
    delay(50);
    M5.Beep.tone(0);
    delay(500);
    M5.Lcd.setTextFont(2);
    break;

case 5: //Echo Trainer
  morse_char = GetLessonCharacters(settings[2],true);
  playMorseCode(morse_char);
  delay(1000);
  M5.Lcd.setTextFont(4);
  M5.Lcd.fillRect(0, 40, 40, 40, BLACK); // erase previous character
  M5.Lcd.setCursor(0, 40);
  M5.Lcd.print(morse_char);

  trials = 0;
  correct = false;
  
  while (trials < 3 && !correct) {
    while (true){
      send_char = touch_paddle();
      if (send_char != " "){
        break;
      }
    }

    Serial.print("recieved:");
    Serial.print(send_char);
    Serial.println("|");


    if (morse_char == send_char) {
      M5.Beep.tone(600);
      delay(50);
      M5.Beep.tone(0);
      correct = true;
    } else {
      M5.Beep.tone(300);
      delay(50);
      M5.Beep.tone(0);
      trials++;
      if (trials < 3) {
        delay(1000);
        playMorseCode(morse_char);
        M5.Lcd.fillRect(0, 40, 40, 40, BLACK); // erase previous character
        M5.Lcd.setCursor(0, 40);
        M5.Lcd.print(morse_char);
      }
    }
  }

  if (!correct) {
    delay(1000);
    playMorseCode(morse_char);
  }

  M5.Lcd.fillRect(0, 40, 40, 40, BLACK); // erase previous character
  delay(1500);
  M5.Lcd.setTextFont(2);

  break;

  default:
    // Tue etwas, im Defaultfall
    // Dieser Fall ist optional
    break; // Wird nicht benötigt, wenn Statement(s) vorhanden sind
}

delay(50);
     
}
