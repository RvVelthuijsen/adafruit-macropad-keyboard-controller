#include <Adafruit_SH110X.h>
#include <Adafruit_NeoPixel.h>
#include <RotaryEncoder.h>
#include <Wire.h>
#include <Keyboard.h>

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
bool encoderDown = false;
unsigned long encoderDownTime = 0;
const int ENCODER_SLEEP_TIME_MILLIS = 3000;
const int NUMBER_OF_KEYS = 12;
const int MAX_NUMBER_OF_ROWS = 5;

int menu_pos = 0;
int cursorRowPosition = 0;

int brightnessValue = 40;
bool lightsOff = false;

bool hasBeenToggled = false;


typedef void (*funcPtr)();
typedef void (*funcPtrStr)(String);

typedef struct { 
  const char* colorName;
  const uint32_t color;
} colorOption;

colorOption colorOptions[] {
  {"white", pixels.Color(255, 255, 255)}, 
  {"green", pixels.Color(5, 250, 5)}, 
  {"blue", pixels.Color(5, 5, 250)}, 
  {"yellow", pixels.Color(255,255,0)}, 
  {"purple", pixels.Color(180, 0, 160)}, 
  {"red", pixels.Color(255, 0, 3)},
  {"off", pixels.Color(0, 0, 0, 0)}
};


typedef struct {
  const char* keyName;
  const uint8_t key;
} KeyAction;

KeyAction actions[] {
  {"Left shift", KEY_LEFT_SHIFT},
  {"Left ctrl", KEY_LEFT_CTRL}
};


typedef struct { 
  uint8_t keyIndex;
  const char* colorName;
  uint8_t actions[3];
  bool isDown;
} KeyItem;


KeyItem* keyMapTest = new KeyItem[NUMBER_OF_KEYS]{
  {0, "green", {KEY_LEFT_SHIFT, KEY_HOME}},
  {1, "yellow", {KEY_LEFT_CTRL, 'x'}},
  {2, "white", {KEY_LEFT_CTRL, 's'}},
  {3, "blue", {KEY_LEFT_SHIFT, KEY_END}},
  {4, "green", {KEY_LEFT_CTRL, 'c'}},
  {5, "white", {KEY_LEFT_CTRL, '/'}},
  {6, "purple", {KEY_LEFT_CTRL, 'y'}},
  {7, "green", {KEY_LEFT_CTRL, 'v'}},
  {8, "yellow", {KEY_LEFT_CTRL, KEY_LEFT_SHIFT, KEY_LEFT_ARROW}},
  {9, "red", {KEY_LEFT_CTRL, 'z'}},
  {10, "yellow", {KEY_LEFT_SHIFT, KEY_LEFT_ALT, KEY_DOWN_ARROW}},
  {11, "yellow", {KEY_LEFT_CTRL, KEY_LEFT_SHIFT, KEY_RIGHT_ARROW}}
};



class KeyManager{
  private:
    KeyItem* keyMap;
  public:
    KeyManager(KeyItem* keys) : keyMap(keys) {}
    bool keysOn = true;
    int cursorPosition = 0;
    int colorCursorPosition = 0;
    bool inKeyMenu = false;
    bool drawColorsActive = false;

    

    void setAllKeyColors(){
      for (uint8_t i = 0; i < NUMBER_OF_KEYS; i++) {
        for (const auto& colorOption : colorOptions) {
          if (keyMap[i].colorName == colorOption.colorName){
            pixels.setPixelColor(i, colorOption.color);
          }
        }
      }
    }

    void turnOffAllKeyColors(){
      for (uint8_t i = 0; i < NUMBER_OF_KEYS; i++) {
        pixels.setPixelColor(i, 0, 0, 0, 0);
      }
    }

    void setHoveredKeyColor(int hoveredKeyIndex){
      for (uint8_t i = 0; i < NUMBER_OF_KEYS; i++) {
        if (hoveredKeyIndex == i){
          for (const auto& colorOption : colorOptions) {
            if(keyMap[i].colorName == colorOption.colorName){
              pixels.setPixelColor(i, colorOption.color);
            }
          }
        } else {
          pixels.setPixelColor(i, 0, 0, 0, 0);
        }
      }
    }

    void checkForKeyInput(){
      for (int i = 0; i < NUMBER_OF_KEYS; i++) {
        if (!digitalRead(i + 1)) { // switch pressed!
          Serial.print("Switch "); Serial.println(i);
          if (isKeyDown(i) == false && keysOn == true) {
            executeKeyMap(i);
          }
          setKeyDown(i, true);
        } else {
          setKeyDown(i, false);
        }
      }
    }

    void executeKeyMap(int keyIndex) {
      for (int i = 0; i < sizeof(keyMap[keyIndex].actions) / sizeof(keyMap[keyIndex].actions[0]); i++) {
        Keyboard.press(keyMap[keyIndex].actions[i]);
      }
      Keyboard.releaseAll();
    }

    bool isKeyDown(int keyIndex){
      return keyMap[keyIndex].isDown;
    }

    void setKeyDown(int keyIndex, bool value) {
      keyMap[keyIndex].isDown = value;
    }

    void toggleKeyFunc(){
      if (keysOn == true) {
        keysOn = false;
      } else {
        keysOn = true;
      }
    }
  
    void drawKeys(){
      display.println("CHOOSE KEY:");
      for(int i = (cursorPosition < 4) ? 0 : (cursorPosition - 4); i <= ((cursorPosition < 4) ? 4 : cursorPosition); i++){
          display.setCursor(0, cursorRowPosition += 8);
          if (i == NUMBER_OF_KEYS){
            turnOffAllKeyColors();
            display.setTextColor(SH110X_BLACK, SH110X_WHITE);
            display.println("BACK");
            display.setTextColor(SH110X_WHITE, SH110X_BLACK);
          } else {
            if (cursorPosition == i) {
              display.setTextColor(SH110X_BLACK, SH110X_WHITE);
              setHoveredKeyColor(i);
            }
            display.print("Key ");
            display.print(keyMap[i].keyIndex);
            if(i <= 9){
              display.print(" ");
            }
            display.print(" -  ");
            display.println(keyMap[i].colorName);
            display.setTextColor(SH110X_WHITE, SH110X_BLACK);
          }
      }
    }

    void drawColors(){
      Serial.println();
      display.println("CHOOSE COLOR:");
      for(int i = (colorCursorPosition < 4) ? 0 : (colorCursorPosition - 4); i <= ((colorCursorPosition < 4) ? 4 : colorCursorPosition); i++){
        if (colorCursorPosition == i) {
          display.setTextColor(SH110X_BLACK, SH110X_WHITE);
          keyMap[cursorPosition].colorName = colorOptions[i].colorName;
          pixels.setPixelColor(cursorPosition, colorOptions[i].color);
        }
        display.println(colorOptions[i].colorName);
        display.setTextColor(SH110X_WHITE, SH110X_BLACK);
      }
    }

    int findIndexOfColor(const char* colorToFind){
      for (int i = 0; i < sizeof(colorOptions) / sizeof(colorOptions[0]); i++){
        if (colorOptions[i].colorName == colorToFind){
          return i;
        }
      }
      return -1;
    }

    void setCursor(){
      cursorPosition += encoder_direction;
      if (cursorPosition == NUMBER_OF_KEYS + 1) {
        cursorPosition = 0;
      } else if (cursorPosition == -1){
        cursorPosition = NUMBER_OF_KEYS;
      }
    }

    void setColorCursor(){
      colorCursorPosition += encoder_direction;
      if (colorCursorPosition == sizeof(colorOptions) / sizeof(colorOptions[0])) {
        colorCursorPosition = 0;
      } else if (colorCursorPosition == -1){
        colorCursorPosition = sizeof(colorOptions) / sizeof(colorOptions[0]) - 1;
      }
    }

    void itemPressed(){
      if (cursorPosition == NUMBER_OF_KEYS){
        cursorPosition = 0;
        inKeyMenu = false;
      } else if (drawColorsActive == true) {
        drawColorsActive = false;
      } else {
        int indexOfColor = findIndexOfColor(keyMap[cursorPosition].colorName);
        colorCursorPosition = (indexOfColor != -1) ? indexOfColor : 0;
        drawColorsActive = true;
      }
    }
};


KeyManager* keyManager = new KeyManager(keyMapTest);


void inputCallbackNew() {
    // Do stuff with value
    Serial.println("test sub");
}

class BasicMenuItem {
  protected:
    const char* title = NULL;
    funcPtr callback;

  public:
    BasicMenuItem(const char* title) : title(title) {}
    bool isActionable = false;
    bool isParent = false;

    virtual void testFunc(){}

    const char* getTitle(){
      return title;
    }

    void draw(){
      Serial.println(title);
    }

    void drawItem(bool hovered){
      if (hovered == true) {
        display.setTextColor(SH110X_BLACK, SH110X_WHITE);
      }
      display.println(title);
      display.setTextColor(SH110X_WHITE, SH110X_BLACK);
    }

    void executeCallbackAction(){
      callback();
    }
};


class TestingMenuScreen {
  private:
    BasicMenuItem** items = NULL;
    uint8_t itemCount = 0;

  public:
    TestingMenuScreen(BasicMenuItem** items, bool isSubMenu) : items(items) {
      while (items[itemCount] != nullptr) {
        itemCount++;
      }
      if (isSubMenu == true){
        items[itemCount] = new BasicMenuItem("BACK");
        itemCount++;
      }
    }
    
    int cursorPosition = 0;
    bool runningCallback = false;
    bool isCurrentlyActive = false;

    int getCurrentMenuLength(){
      return itemCount;
    }

    bool isCurrentItemActionable(){
      return items[cursorPosition]->isActionable;
    }

    bool isCurrentItemParent(){
      return items[cursorPosition]->isParent;
    }

    void runCallBack(){
      items[cursorPosition]->executeCallbackAction();
    }

    void runSubMenu(){
      items[cursorPosition]->testFunc();
    }

    const char* getItemTitle(){
      return items[cursorPosition]->getTitle();
    }

    void printValues(){
      for (int i = 0; i < itemCount; i++){
        items[i]->draw();
      }
    }

    void printLength(){
      Serial.println(itemCount);
    }

    void drawMenu(){
      display.println("OPTIONS:");
      for (int i = 0; i < itemCount; i++){
        display.setCursor(0, cursorRowPosition += 8);
        items[i]->drawItem(i == cursorPosition ? true : false);
      }
    }
};

class ParentMenuItem : public BasicMenuItem {
  public:
    ParentMenuItem(const char* title, TestingMenuScreen* child) : BasicMenuItem{title}, childMenu(child){
      isParent = true;
    }
    TestingMenuScreen* childMenu;

    void testFunc() override {
      childMenu->isCurrentlyActive = true;
    }

};

class ActionMenuItem : public BasicMenuItem {
  public:
    ActionMenuItem(const char* title, funcPtr callBackFunc) : BasicMenuItem{title} {
      callback = callBackFunc;
      isActionable = true;
    }

};

BasicMenuItem* keyMenuList[] = { new ActionMenuItem("Key Functionality", &setBrightness), new ActionMenuItem("Key Color", &drawKeys), nullptr };
TestingMenuScreen* keyMenu = new TestingMenuScreen(keyMenuList, true);

BasicMenuItem* settingMenuList[] = { new ActionMenuItem("LED Brightness", &setBrightness), nullptr };
TestingMenuScreen* settingsMenu = new TestingMenuScreen(settingMenuList, true);

BasicMenuItem* mainMenuList[] = { new ParentMenuItem("Key Management", keyMenu), new ParentMenuItem("Settings", settingsMenu), nullptr };
TestingMenuScreen* mainMenu = new TestingMenuScreen(mainMenuList, false);

class MenuManager {
  private:
    TestingMenuScreen** menuList = NULL;
    uint8_t menuCount = 0;
    uint8_t currentActiveMenuIndex = 0;

  public:
    MenuManager(TestingMenuScreen** list) : menuList(list) {
      while (menuList[menuCount] != nullptr) {
        menuCount++;
      }
      menuList[0]->isCurrentlyActive = true;
    }
    bool runningCallback = false;

    void setCursor(){
      for (int i = 0; i < menuCount; i++) {
        if(menuList[i]->isCurrentlyActive == true){
          int lengthOfCurrentMenu = menuList[i]->getCurrentMenuLength();
          menuList[i]->cursorPosition += encoder_direction;
          if (menuList[i]->cursorPosition == lengthOfCurrentMenu) {
            menuList[i]->cursorPosition = 0;
          } else if (menuList[i]->cursorPosition == -1){
            menuList[i]->cursorPosition = lengthOfCurrentMenu - 1;
          }
          break;
        }
      }
    }

    void itemPressed(){
      for (int i = 0; i < menuCount; i++) {
        if(menuList[i]->isCurrentlyActive == true){
          if (menuList[i]->isCurrentItemActionable() == true) {
            menuList[i]->runningCallback = !menuList[i]->runningCallback;
            runningCallback = !runningCallback;
            break;
          } else if (menuList[i]->isCurrentItemParent() == true) {
            // submenu needs to be activated
            menuList[i]->isCurrentlyActive = false;
            menuList[i]->runSubMenu();
            break;
          } else if (menuList[i]->getItemTitle() == "BACK") {
            Serial.println("BACK");
            menuList[i]->isCurrentlyActive = false;
            menuList[i]->cursorPosition = 0;
            menuList[0]->isCurrentlyActive = true;
            break;
          }
        }
      }
    }

    void runCallback() {
      for (int i = 0; i < menuCount; i++){
        if(menuList[i]->isCurrentlyActive == true){
          // Serial.println("menu %d : should only be 0", i);
          menuList[i]->runCallBack();
          break;
        }
      }
    }

    void drawActiveMenu(){
      for (int i = 0; i < menuCount; i++){
        if(menuList[i]->isCurrentlyActive == true){
          // Serial.println("menu %d : should only be 0", i);
          menuList[i]->drawMenu();
          break;
        }
      }
    }
};

TestingMenuScreen* menuList[] = {mainMenu, keyMenu, settingsMenu };
MenuManager* menuManager = new MenuManager(menuList);

void setup() {
  Serial.begin(115200);
  //while (!Serial) { delay(10); }     // wait till serial port is opened
  delay(100);  // RP2040 delay is not a bad idea

  // start pixels!
  pixels.begin();
  pixels.setBrightness(brightnessValue);
  pixels.show(); // Initialize all pixels to 'off'

  // set color for leds under keys
  keyManager->setAllKeyColors();
  pixels.show();

  // Start OLED
  display.begin(0, true); // we dont use the i2c address but we will reset!
  display.display();
  
  // set all mechanical keys to inputs
  for (uint8_t i = 0; i <= NUMBER_OF_KEYS; i++) {
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
  display.clearDisplay();

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


void loop() {
  display.clearDisplay();
  cursorRowPosition = 0;
  display.setCursor(0, cursorRowPosition);
  display.println("* Adafruit Macropad *");
  display.setCursor(0, cursorRowPosition += 16);

  if(encoderDown == false) {
    readAndSetEncoderPositions();
  }

  if (menuManager->runningCallback == true){
    menuManager->runCallback();
  } else {
    menuManager->drawActiveMenu();
  }

  
  // detect if encoder is pressed
  if (!digitalRead(PIN_SWITCH)) {
    if (encoderDown == true){
      // if encoder was already down last loop
      unsigned long timePassed = millis() - encoderDownTime;

      if (timePassed >= ENCODER_SLEEP_TIME_MILLIS && hasBeenToggled == false){
        toggleLights();
        keyManager->toggleKeyFunc();
        hasBeenToggled = true;
        //encoderDownTime = 0;
        // encoderDown = false;
      }
      
    } else {
      // if this is the first loop the encoder is down, set to true and save millis
      Serial.println("encoder read as high this loop but encoderDown set to false");
      encoderDown = true;
      encoderDownTime = millis();
    }

  } else {
    if(encoderDown == true && hasBeenToggled == true){
      // encoderDown was still true cause the lights had just been toggled
      Serial.println("encoder not read as high this loop but encoderDown still set to true and hasbeentoggled true");
    } else if (encoderDown == true && keyManager->keysOn == true && lightsOff == false) {
      // if encoder was pressed but since let go and the lights are one it means a menu option was selected
      // execute selection
      if (keyManager->inKeyMenu) {
        Serial.println("calling key itemPressed");
        keyManager->itemPressed();
        if (keyManager->inKeyMenu == false){
          menuManager->runningCallback = false;
        }
      } else {
        Serial.println("calling menu itemPressed");
        menuManager->itemPressed();
      }
    }
    // then reset the values
    hasBeenToggled = false;
    encoderDown = false;
    encoderDownTime = 0;
  }
  
  // loop through all 12 keys to check if one was pressed
  keyManager->checkForKeyInput();

  if (keyManager->inKeyMenu == false){
    keyManager->setAllKeyColors();
  }

  pixels.show();

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
    
    if (menuManager->runningCallback == false) {
      Serial.println("runningCallback == false - setting menu manager cursor");
      menuManager->setCursor();
    } else if (keyManager->inKeyMenu == true) {
      Serial.println("inkeymenu == true, setting key manager cursor");
      if (keyManager->drawColorsActive == true){
        keyManager->setColorCursor();
      } else {
        keyManager->setCursor();
      }
    }
  }
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

void drawKeys(){
  keyManager->inKeyMenu = true;
  if (keyManager->drawColorsActive == true){
    keyManager->drawColors();
  } else {
    keyManager->drawKeys();
  }
}


void setBrightness(){
  display.println("Brightness:");

  brightnessValue += encoder_direction;
  brightnessValue = constrain(brightnessValue, 0, 255);

  display.println(brightnessValue);
  pixels.setBrightness(brightnessValue);

}