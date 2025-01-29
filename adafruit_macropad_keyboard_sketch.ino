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
int menu_pos = 0;
int cursorRowPosition = 0;
int brightnessValue = 40;

bool lightsOff = false;
bool keysOn = true;
bool hasBeenToggled = false;

bool encoderDown = false;
unsigned long encoderDownTime = 0;

char keyMap[12][3] = {
{KEY_LEFT_SHIFT, KEY_HOME}, {KEY_LEFT_CTRL, 'x'}, {KEY_LEFT_CTRL, 's'},
{KEY_LEFT_SHIFT, KEY_END}, {KEY_LEFT_CTRL, 'c'}, {KEY_LEFT_CTRL, '/'},
{KEY_LEFT_CTRL, 'y'}, {KEY_LEFT_CTRL, 'v'}, {KEY_LEFT_CTRL, KEY_LEFT_SHIFT, KEY_LEFT_ARROW},
{KEY_LEFT_CTRL, 'z'}, {KEY_LEFT_SHIFT, KEY_LEFT_ALT,	KEY_DOWN_ARROW}, {KEY_LEFT_CTRL, KEY_LEFT_SHIFT, KEY_RIGHT_ARROW},
};  

bool keyDown[12];

class Key {
  public:
    Key(int index, char* actions, uint32_t color) : keyIndex(index), keyActions(actions){}
    int keyIndex;
    char* keyActions;
    uint32_t keyColor;
}


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


typedef void (*funcPtr)();
typedef void (*funcPtrStr)(String);


void inputCallback(String test) {
    // Do stuff with value
    Serial.println(test);
}

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

BasicMenuItem* keyMenuList[] = { new ActionMenuItem("Key Functionality", &setBrightness), new ActionMenuItem("Key Color", &inputCallbackNew), nullptr };
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
  setAllKeyColors();
  pixels.show();

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

  readAndSetEncoderPositions();

  if (menuManager->runningCallback == true){
    // screen->runCallBack();
    menuManager->runCallback();
  } else {
    // screen->drawMenu();
    menuManager->drawActiveMenu();
  }

  
  // detect if encoder is pressed
  if (!digitalRead(PIN_SWITCH)) {
    if (encoderDown == true){
      // if encoder was already down last loop
      unsigned long timePassed = millis() - encoderDownTime;

      if (timePassed >= 3000 && hasBeenToggled == false){
        toggleLights();
        toggleKeyFunc();
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
    } else if (encoderDown == true && keysOn == true && lightsOff == false) {
      // if encoder was pressed but since let go and the lights are one it means a menu option was selected
      // execute selection
      Serial.println("calling itemPressed");
      menuManager->itemPressed();
    }
    // then reset the values
    hasBeenToggled = false;
    encoderDown = false;
    encoderDownTime = 0;
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

  setAllKeyColors();
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
      menuManager->setCursor();
    }
  }
}

void executeKeyMap(int keyNumber) {
  for (int i = 0; i < sizeof(keyMap[keyNumber]) / sizeof(keyMap[keyNumber][0]); i++) {
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

void setAllKeyColors(){
  for (const auto& key : keyColorMap) {
    for (const auto& colorOption : colorOptions) {
      if (key.colorName == colorOption.colorName){
        pixels.setPixelColor(key.keyIndex, colorOption.color);
      }
    }
  }
}

void setBrightness(){
  display.println("Brightness:");

  brightnessValue += encoder_direction;
  brightnessValue = constrain(brightnessValue, 0, 255);

  display.println(brightnessValue);
  pixels.setBrightness(brightnessValue);

}

