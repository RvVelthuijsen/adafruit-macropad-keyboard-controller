#include <Adafruit_SH110X.h>
#include <Adafruit_NeoPixel.h>
#include <RotaryEncoder.h>
#include <Wire.h>
#include <Keyboard.h>
// #include <ArxContainer.h>

// Create the neopixel strip with the built in definitions NUM_NEOPIXEL and PIN_NEOPIXEL
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_NEOPIXEL, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

// Create the OLED display
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &SPI1, OLED_DC, OLED_RST, OLED_CS);

// Create the rotary encoder
RotaryEncoder encoder(PIN_ROTB, PIN_ROTA, RotaryEncoder::LatchMode::FOUR3);
void checkPosition() {  encoder.tick(); } // just call tick() to check the state.
// our encoder position state
int encoder_pos = 0;
int encoder_direction = 0;
int menu_pos = 0;
int cursorRowPosition = 0;
int brightnessValue = 40;

bool lightsOff = false;
bool keysOn = true;
bool inMenu = false;

bool encoderDown = false;
unsigned long encoderDownTime = 0;

char keyMap[12][3] = {
{KEY_LEFT_SHIFT, KEY_HOME}, {KEY_LEFT_CTRL, 'x'}, {KEY_LEFT_CTRL, 's'},
{KEY_LEFT_SHIFT, KEY_END}, {KEY_LEFT_CTRL, 'c'}, {KEY_LEFT_CTRL, '/'},
{KEY_LEFT_CTRL, 'y'}, {KEY_LEFT_CTRL, 'v'}, {KEY_LEFT_CTRL, KEY_LEFT_SHIFT, KEY_LEFT_ARROW},
{KEY_LEFT_CTRL, 'z'}, {KEY_LEFT_SHIFT, KEY_LEFT_ALT,	KEY_DOWN_ARROW}, {KEY_LEFT_CTRL, KEY_LEFT_SHIFT, KEY_RIGHT_ARROW},
};  

bool keyDown[12];

int brightnessOptions[] = { 10, 20, 30, 40, 50, 60, 70, 80 };

//std::map<String, uint32_t> ledColorMap {{"white", pixels.Color(255, 255, 255)}, {"green", pixels.Color(5, 250, 5)}, {"blue", pixels.Color(5, 5, 250)}, {"yellow", pixels.Color(255,255,0)}, {"purple", pixels.Color(180, 0, 160)}, {"red", pixels.Color(255, 0, 3)}};

//std::map<int, String> keyColorMap {{0, "green"}, {1, "yellow"}, {2, "white"}, {3, "blue"}, {4, "green"}, {5, "white"}, {6, "purple"}, {7, "green"}, {8, "yellow"}, {9, "red"}, {10, "yellow"}, {11, "yellow"} };

typedef struct { 
  uint8_t index;
  const char* values[3];
} menu;

typedef struct { 
  String colorName;
  uint32_t color;
} colorOption;

typedef struct { 
  uint8_t keyIndex;
  String colorName;
} keyColorMapItem;

keyColorMapItem keyColorMap[] {
  {0, "green"}, 
  {1, "yellow"}, 
  {2, "white"}, 
  {3, "blue"}, 
  {4, "green"}, 
  {5, "white"}, 
  {6, "purple"}, 
  {7, "green"}, 
  {8, "yellow"}, 
  {9, "red"}, 
  {10, "yellow"}, 
  {11, "yellow"}   
};

colorOption colorOptions[] {
  {"white", pixels.Color(255, 255, 255)}, 
  {"green", pixels.Color(5, 250, 5)}, 
  {"blue", pixels.Color(5, 5, 250)}, 
  {"yellow", pixels.Color(255,255,0)}, 
  {"purple", pixels.Color(180, 0, 160)}, 
  {"red", pixels.Color(255, 0, 3)}
};

menu listOfMenus[] {
    {0, { "Assign Keys", "Settings" }},
    {1, { "Key Brightness", "Key Colors", "BACK" }},
};

int currentMenuIndex = 0;
String currentMenuOption;

int setLengthOfCurrentMenuArray(){
  int counter = 0;
    for(String test : listOfMenus[currentMenuIndex].values){
      if(test.length() > 1){
        counter++;
      }
    }
    return counter;
}

int lengthOfCurrentMenuArray = setLengthOfCurrentMenuArray();

void setup() {
  Serial.begin(115200);
  //while (!Serial) { delay(10); }     // wait till serial port is opened
  delay(100);  // RP2040 delay is not a bad idea

  Serial.println("Adafruit Macropad with RP2040");

  // start pixels!
  pixels.begin();
  pixels.setBrightness(brightnessValue);
  pixels.show(); // Initialize all pixels to 'off'

  // Start OLED
  display.begin(0, true); // we dont use the i2c address but we will reset!
  display.display();
  
  // set all mechanical keys to inputs
  for (uint8_t i=0; i<=12; i++) {
    pinMode(i, INPUT_PULLUP);
  }

  // set rotary encoder inputs and interrupts
  pinMode(PIN_ROTA, INPUT_PULLUP);
  pinMode(PIN_ROTB, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ROTA), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ROTB), checkPosition, CHANGE);  

  // We will use I2C for scanning the Stemma QT port
  Wire.begin();

  // text display tests
  display.setTextSize(1);
  display.setTextWrap(false);
  display.setTextColor(SH110X_WHITE, SH110X_BLACK); // white text, black background

  // Enable speaker
  pinMode(PIN_SPEAKER_ENABLE, OUTPUT);
  digitalWrite(PIN_SPEAKER_ENABLE, HIGH);
  // Play some tones
  pinMode(PIN_SPEAKER, OUTPUT);
  digitalWrite(PIN_SPEAKER, LOW);
  tone(PIN_SPEAKER, 988, 100);  // tone1 - B5
  delay(100);
  tone(PIN_SPEAKER, 1319, 200); // tone2 - E6
  delay(200);
}

uint8_t j = 0;
bool i2c_found[128] = {false};

void loop() {
  display.clearDisplay();
  cursorRowPosition = 0;
  display.setCursor(0,cursorRowPosition);
  display.println("* Adafruit Macropad *");

  readAndSetEncoderPositions();
  if (inMenu){
    setBrightness();
  } else {
    drawMenu();
  }

  // Not currently necessary:
  // ScanI2C();
  
  if (!digitalRead(PIN_SWITCH)) {
    //display.setCursor(0, 24);
    //display.print("Encoder pressed ");
    //pixels.setBrightness(255);     // bright!
    
    Serial.println("Encoder button pressed");

    if (encoderDown == true){
      // if encoder was already down last loop
      unsigned long timePassed = millis() - encoderDownTime;

      if (timePassed >= 3000){
        toggleLights();
        toggleKeyFunc();
        encoderDownTime = 0;
        encoderDown = false;
      }
      
    } else {
      // if this is the first loop the encoder is down, set to true and save millis
      encoderDown = true;
      encoderDownTime = millis();
    }

  } else {
    if(encoderDown == true){
    // if encoder was pressed but since let go it means a menu option was selected
    // execute selection
      changeMenu();
    }
    // then reset the values
    encoderDown = false;
    encoderDownTime = 0;
  }
  

  // set color for leds under keys
  for (const auto& key : keyColorMap) {
    //pixels.setPixelColor(m.first, ledColorMap[m.second]);
    for (const auto& colorOption : colorOptions) {
      if (key.colorName == colorOption.colorName){
        pixels.setPixelColor(key.keyIndex, colorOption.color);
      }
    }
  }
  
  // loop through all 12 keys to check if one was pressed
  for (int i=0; i<12; i++) {
    if (!digitalRead(i + 1)) { // switch pressed!
      Serial.print("Switch "); Serial.println(i);
      if (keyDown[i] == false && keysOn == true) {
        executeKeyMap(i);
      }
      keyDown[i] = true;
    } else {
      keyDown[i] = false;
    }
  }

  // show neopixels, increment swirl
  pixels.show();
  j++;

  // display oled
  display.display();
}

void readAndSetEncoderPositions(){
  encoder.tick();
  int newPos = encoder.getPosition();
  encoder_direction = (int)(encoder.getDirection());
  if (encoder_pos != newPos) {
    Serial.print("Encoder:");
    Serial.print(newPos);
    Serial.print(" Direction:");
    Serial.println(encoder_direction);
    encoder_pos = newPos;
    
    menu_pos += encoder_direction;
    if (menu_pos == lengthOfCurrentMenuArray) {
      menu_pos = 0;
    } else if (menu_pos == -1){
      menu_pos = lengthOfCurrentMenuArray - 1;
    }
  }
}

void changeMenu() {
  if (currentMenuOption == "BACK"){
    currentMenuIndex = 0;
  } else if (currentMenuOption == "Key Brightness") {
    inMenu = true;
    currentMenuIndex = 0;
  } else {
    currentMenuIndex = menu_pos;
  }
  menu_pos = 0;
  lengthOfCurrentMenuArray = setLengthOfCurrentMenuArray();
}

void drawMenu(){
  cursorRowPosition += 16;
  display.setCursor(0, cursorRowPosition);
  display.print("MENU OPTIONS: ");

  int counter = 0;
  for(String test : listOfMenus[currentMenuIndex].values){
    cursorRowPosition += 8;
    display.setCursor(0, cursorRowPosition);
    if(menu_pos == counter){
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
      display.print(test);
      display.setTextColor(SH110X_WHITE, SH110X_BLACK);
      currentMenuOption = test;
    } else {
      display.print(test);
    }
    counter++;
  }
}

void executeKeyMap(int keyNumber) {
  for (int i=0; i<sizeof(keyMap[keyNumber]); i++) {inMenu
    Keyboard.press(keyMap[keyNumber][i]);
  }
  Keyboard.releaseAll();
}

void toggleLights() {
  if(lightsOff == true){
    // if lights off is currently true, turn on lights
    Serial.println("leds and display turned on");
    pixels.setBrightness(brightnessValue);
    display.oled_command(SH110X_DISPLAYON);
    lightsOff = false;
  } else {
    // if lights off is currently false, turn lights off
    Serial.println("leds and display turned off");
    pixels.setBrightness(0);
    display.oled_command(SH110X_DISPLAYOFF);
    lightsOff = true;
  }
}

void toggleKeyFunc(){
  if (keysOn == true) {
    keysOn = false;
  } else {
    keysOn = true;
  }
}

void setBrightness(){
  display.println("Brightness:");
  display.println(brightnessValue);

  brightnessValue += encoder_direction;
  brightnessValue = constrain(brightnessValue,0, 255);

  pixels.setBrightness(brightnessValue);
  if (!digitalRead(PIN_SWITCH)) {
    inMenu = false;
  }
}

void ScanI2C(){
    // Scanning takes a while so we don't do it all the time
  if ((j & 0x3F) == 0) {
    Serial.println("Scanning I2C: ");
    Serial.print("Found I2C address 0x");
    for (uint8_t address = 0; address <= 0x7F; address++) {
      Wire.beginTransmission(address);
      i2c_found[address] = (Wire.endTransmission () == 0);
      if (i2c_found[address]) {
        Serial.print("0x");
        Serial.print(address, HEX);
        Serial.print(", ");
      }
    }
    Serial.println();
  }
  
  display.setCursor(0, 16);
  display.print("I2C Scan: ");
  for (uint8_t address=0; address <= 0x7F; address++) {
    if (!i2c_found[address]) continue;
    display.print("0x");
    display.print(address, HEX);
    display.print(" ");
  }
}