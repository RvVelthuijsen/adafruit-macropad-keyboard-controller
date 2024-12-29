#include <Adafruit_SH110X.h>
#include <Adafruit_NeoPixel.h>
#include <RotaryEncoder.h>
#include <Wire.h>
#include <Keyboard.h>
#include <map>

// Create the neopixel strip with the built in definitions NUM_NEOPIXEL and PIN_NEOPIXEL
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_NEOPIXEL, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

// Create the OLED display
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &SPI1, OLED_DC, OLED_RST, OLED_CS);

// Create the rotary encoder
RotaryEncoder encoder(PIN_ROTB, PIN_ROTA, RotaryEncoder::LatchMode::FOUR3);
void checkPosition() {  encoder.tick(); } // just call tick() to check the state.
// our encoder position state
int encoder_pos = 0;
int cursorRowPosition = 0;
int brightnessValue = 40;

bool lightsOff = false;
bool keysOn = true;

bool encoderDown = false;
unsigned long encoderDownTime = 0;

char keyMap[12][3] = {
{KEY_LEFT_SHIFT, KEY_HOME}, {KEY_LEFT_CTRL, 'x'}, {KEY_LEFT_CTRL, 's'},
{KEY_LEFT_SHIFT, KEY_END}, {KEY_LEFT_CTRL, 'c'}, {KEY_LEFT_CTRL, '/'},
{KEY_LEFT_CTRL, 'y'}, {KEY_LEFT_CTRL, 'v'}, {KEY_LEFT_CTRL, KEY_LEFT_SHIFT, KEY_LEFT_ARROW},
{KEY_LEFT_CTRL, 'z'}, {KEY_LEFT_SHIFT, KEY_LEFT_ALT,	KEY_DOWN_ARROW}, {KEY_LEFT_CTRL, KEY_LEFT_SHIFT, KEY_RIGHT_ARROW},
};  

bool keyDown[12];

String menuOptions[] = { "Assign Keys", "Settings" };

String settingsOptions[] = { "Key Brightness", "Key Colors" };

int brightnessOptions[] = { 10, 20, 30, 40, 50, 60, 70, 80 };

// String &currentMenu[] = menuOptions;
map<String, String[]> menus = {
  {"main", menuOptions},
  {"settings", settingsOptions}
}

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
  
  encoder.tick();          // check the encoder
  int newPos = encoder.getPosition();
  if (encoder_pos != newPos) {
    Serial.print("Encoder:");
    Serial.print(newPos);
    Serial.print(" Direction:");
    Serial.println((int)(encoder.getDirection()));
    encoder_pos = newPos;
  }

  drawMenu();

  // Not currently necessary
  // ScanI2C();
  
  // check encoder press
  
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
    encoderDown = false;
    encoderDownTime = 0;
  }
  

  for(int i=0; i< pixels.numPixels(); i++) {
    // pixels.setPixelColor(i, Wheel(((i * 256 / pixels.numPixels()) + j) & 255));
    pixels.setPixelColor(i, colorPicker(i));
  }
  
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

  // show neopixels, incredment swirl
  pixels.show();
  j++;

  // display oled
  display.display();
}

void changeMenu(int menuOptionChosen) {
}

void drawMenu(){
  cursorRowPosition += 16;
  display.setCursor(0, cursorRowPosition);
  // display.print("Rotary encoder: ");
  // display.print(encoder_pos);
  display.print("MENU OPTIONS: ");
  // for(String option : menuOptions) {
  // for(int i=0; i<2; i++) {
  int counter = 0;
  for(String option : menuOptions) {
    cursorRowPosition += 8;
    display.setCursor(0, cursorRowPosition);
    if(encoder_pos == counter){
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
      display.print(menuOptions[counter]);                 
      display.setTextColor(SH110X_WHITE, SH110X_BLACK);
    } else {
      display.print(menuOptions[counter]);
    }
    counter++;
  }
}

void executeKeyMap(int keyNumber) {
  // move the text into a 3x4 grid
  // display.setCursor(((i-1) % 3)*48, 32 + ((i-1)/3)*8);
  pixels.setPixelColor(keyNumber, 0xFFFFFF);  // make white
  
  display.print("KEY ");
  display.print(keyNumber + 1);
  display.print(": ");
  display.print(sizeof(keyMap[keyNumber]));

  for (int i=0; i<sizeof(keyMap[keyNumber]); i++) {
    Keyboard.press(keyMap[keyNumber][i]);
  }

  Keyboard.releaseAll();
}

void toggleLights() {
  if(lightsOff == true){
    Serial.println("leds and display turned on");
    // if lights off is currently true, turn on lights
    pixels.setBrightness(brightnessValue);
    display.oled_command(SH110X_DISPLAYON);
    lightsOff = false;
  } else {
    Serial.println("leds and display turned off");
    // if lights off is currently false, turn lights off
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

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
   WheelPos -= 170;
   return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}

uint32_t colorPicker(int keyNumber) {
  switch (keyNumber) {
    case 0:
      return pixels.Color(5, 250, 5);
      break;
    case 1:
      return pixels.Color(255,165,0);
      break;
    case 2:
      return pixels.Color(255, 255, 255);
      break;
    case 3:
      return pixels.Color(5, 5, 250);
      break;
    case 4:
      return pixels.Color(100, 250, 0);
      break;
    case 5:
      return pixels.Color(255, 255, 255);
      break;
    case 6:
      return pixels.Color(180, 0, 160);
      break;
    case 7:
      return pixels.Color(5, 250, 5);
      break;
    case 8:
      return pixels.Color(255,255,0);
      break;
    case 9:
      return pixels.Color(255, 0, 3);
      break;
    case 10:
      return pixels.Color(255,255,0);
      break;
    case 11:
      return pixels.Color(255,255,0);
      break;
    default:
      return pixels.Color(255, 255, 255);
      break;
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