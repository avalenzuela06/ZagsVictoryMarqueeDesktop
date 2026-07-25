/* Example code for scrolling text effect on 
   MAX7219 LED dot matrix display with Arduino. 
   More info: https://www.makerguides.com */

#include "MD_Parola.h"
#include "MD_MAX72xx.h"
#include "SPI.h"

// Define hardware type, size, and output pins:
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 8
#define CS_PIN 9 //13

// Create a new instance of the MD_Parola class with hardware SPI connection:
//MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// Setup for software SPI:
#define MATRIX_PIN 7 //14
#define CLK_PIN 41
MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, MATRIX_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
String phrases[] = {"Welcome to Gonzaga University!", "Zags Win!!!!", "Congrats Class of 2026!", "Welcome to Freshman Orientation!", "Thank you Domatas Sabonis!"};
int numPhrases = 4;

//ports for Heart LED
#define heartLight 10

//LED Strip Lights
#include <FastLED.h>
#define NUM_LEDS 86
#define STRIP_PIN 12
#define NUM_COLORS 6

CRGB leds[NUM_LEDS];
uint8_t hue = 0;
int currentPhrase = 0;
bool heartOn = false;

int chosenStripSetup = 1;
int chosenBlinkAnim = 0;
int chosenStripAnim = 0;

//Red, Orange, Yellow, Green, Blue, Purple
CRGB SetColors[] = {CRGB::Red, CRGB::Orange, CRGB(248, 255, 0), CRGB::Green, CRGB::Blue, CRGB(255, 0 , 220)};
CRGB Basketball[] {CRGB::Orange, CRGB::White, CRGB::Blue, CRGB::Gray};
CRGB FASU[] = {CRGB::Red, CRGB::Blue, CRGB::White, CRGB::Yellow};
CRGB Science[] = {CRGB::Green, CRGB::White};

// Array of pointers to each color array
CRGB* colorSets[] = {Basketball, FASU, Science, SetColors};

// Number of colors in each set (must match order above)98
int colorSetSizes[] = {4, 3, 2, 6};

//animation vars
const int NUM_COLOR_SETS = 4;
int chosenSet = random(0, NUM_COLOR_SETS);
CRGB* chosenColors = colorSets[chosenSet];
int chosenNumColors = colorSetSizes[chosenSet];

// Timing variables for LED strip
unsigned long lastStripUpdate = 0;
int stripPattern = 0;
const int STRIP_INTERVAL = 100; // how fast strip alternates
bool stripInitialized = false;
bool animationPlaying = false;

//Button
#define onSwitch 4
const int holdThreshold = 3000;   // 3 seconds
const int connectThreshold = 3000; // 3 second hold while on to connect to wifi
const int offThreshold = 7000; // 7 second hold to turn off device if on
const int debounceDelay = 50;     // 50 ms

unsigned long pressStartTime = 0;
unsigned long lastDebounceTime = 0;

bool lastButtonReading = HIGH;
bool buttonState = HIGH;
bool holdTriggered = false;
bool lightOn = false;
unsigned long holdTime;



//Basic Lighting Procedure
void fillMirror(int hue) {
  fill_rainbow(leds, 44, hue, 6); // fill first 45 normally
  for (int i = 0; i < 42; i++) {
    leds[44 + i] = leds[i]; // straight copy into second half
  }
}

void turnLightsOn() {
  Serial.println("turnLightsOn() called");
  Serial.println("Starting LED fade...");
  for (int i = 0; i < 150; i++) {
    fillMirror(i);
    FastLED.setBrightness(i);
    FastLED.show();
    delay(10);
}
  for(int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      digitalWrite(heartLight, HIGH);
      delay(100);
      digitalWrite(heartLight, LOW);
      delay(100);
      digitalWrite(heartLight, HIGH);
    }
    delay(1000);
  } 
  myDisplay.displayText("Welcome to Gonzaga University!", PA_CENTER, 100, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  while (!myDisplay.displayAnimate()) {
        // wait for this scroll to finish too
  }
  Serial.println("turnLightsOn() done");
}

void turnLightsOff() {
  FastLED.setBrightness(150);
  for(int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      digitalWrite(heartLight, LOW);
      delay(100);
      digitalWrite(heartLight, HIGH);
      delay(100);
      digitalWrite(heartLight, LOW);
    }
    delay(1000);
  } 
myDisplay.displayText("GO ZAGS!", PA_CENTER, 100, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
while (!myDisplay.displayAnimate()) {
        // wait for this scroll to finish too
        }
  for (int i = 150; i > 0; i--) {
    FastLED.setBrightness(i);
    FastLED.show();
    delay(10);
  }
  FastLED.clear();
  FastLED.show();
}

//Helper functions for make light animation
void resetAnimation() {
  stripPattern = 0;
  lastStripUpdate = 0;
  stripInitialized = false;
}

void stripSetup(CRGB colorArr[], int numColors, int option) { // divide led strip evenly with each light in the aray (option 1), alternate colors (option 2)
  
  if (option == 1) {
    // ---- Divide strip evenly into color segments ----
    int ledsPerColor = NUM_LEDS / numColors;
    int remainder = NUM_LEDS % numColors;
    int ledIndex = 0;

    for (int i = 0; i < numColors; i++) {
      int segmentSize = ledsPerColor + (i < remainder ? 1 : 0);
      for (int j = 0; j < segmentSize; j++) {
        leds[ledIndex] = colorArr[i];
        ledIndex++;
      }
    }

  } else if (option == 2) {
    // ---- Alternate colors one LED at a time ----
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = colorArr[i % numColors];
    }
  }

  FastLED.show();
}

unsigned long lastLightUpdate = 0;
unsigned long LIGHT_INTERVAL = 1000;


void blinkAnimation(int option) {
  if (option == 0) {
    // ---- Static, all letters on ----
    digitalWrite(heartLight, HIGH);
    return; // no timing needed
  }

  if (millis() - lastLightUpdate < LIGHT_INTERVAL) return; // not time yet
  lastLightUpdate = millis();

  if (option == 1) {
    // ---- On and off ----
    heartOn = !heartOn;
    digitalWrite(heartLight, heartOn ? HIGH : LOW);
  } else if (option == 2) {
    // ---- Heartbeat: quick double-flash, then pause ----
    static unsigned long lastHeartUpdate = 0;
    static int heartStep = 0;
    const int FLASH_DURATION = 100;  // ms per on/off blip
    const int PAUSE_DURATION = 800;  // ms rest between heartbeats

    unsigned long stepInterval = (heartStep < 4) ? FLASH_DURATION : PAUSE_DURATION;
    if (millis() - lastHeartUpdate < stepInterval) return;
    lastHeartUpdate = millis();

    switch (heartStep) {
      case 0: digitalWrite(heartLight, HIGH); break; // lub - on
      case 1: digitalWrite(heartLight, LOW);  break; // lub - off
      case 2: digitalWrite(heartLight, HIGH); break; // dub - on
      case 3: digitalWrite(heartLight, LOW);  break; // dub - off
      case 4: /* pause step, stays off */     break;
    }

    heartStep++;
    if (heartStep > 4) heartStep = 0;
    return;
  }
}

void stripAnimation(CRGB colorArr[], int numColors, int option) {
  if (millis() - lastStripUpdate < STRIP_INTERVAL) {
    FastLED.show();
    return;
    }
  lastStripUpdate = millis();

  if (option == 0) {
    // ---- Static, do nothing ----
    FastLED.show();
    return;

  } else if (option == 1) {
    // ---- Pause, then double flash ----
    // Uses its own fixed 500ms interval regardless of STRIP_INTERVAL
    static unsigned long lastBlinkUpdate = 0;
    if (millis() - lastBlinkUpdate < 500) return;
    lastBlinkUpdate = millis();

    const int pauseSteps = 4;

    if (stripPattern < pauseSteps) {
        FastLED.setBrightness(0);
        FastLED.show();
        stripPattern++;
    } else if (stripPattern == pauseSteps) {
        FastLED.setBrightness(150);
        FastLED.show();
        stripPattern++;
    } else if (stripPattern == pauseSteps + 1) {
        FastLED.setBrightness(0);
        FastLED.show();
        stripPattern++;
    } else if (stripPattern == pauseSteps + 2) {
        FastLED.setBrightness(150);
        FastLED.show();
        stripPattern++;
    } else {
        FastLED.setBrightness(0);
        FastLED.show();
        stripPattern = 0;
    }
  } else if (option == 2) {
    // ---- Shift colors down the strip, wrapping end to start ----
    CRGB last = leds[NUM_LEDS - 1]; // save the last LED
    for (int i = NUM_LEDS - 1; i > 0; i--) {
        leds[i] = leds[i - 1]; // shift each LED forward
    }
    leds[0] = last; // wrap the last LED back to the front
    FastLED.show();
    stripPattern = (stripPattern + 1) % NUM_LEDS; // track position if needed

  } else if (option == 3) {
    int trailLength = 8;
    int headPos = stripPattern % NUM_LEDS;

    // Fill entire strip with base color (first color, dimmed)
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = colorArr[0];
        leds[i].nscale8(80); // dim background
    }

    // Draw chasers in remaining colors (or brighter version of color 0 if only 1 color)
    int chaserColors = numColors > 1 ? numColors - 1 : 1;
    for (int c = 0; c < chaserColors; c++) {
        // Space chasers evenly around the strip
        int chaserHead = (headPos + (NUM_LEDS / chaserColors) * c) % NUM_LEDS;
        CRGB chaserColor = numColors > 1 ? colorArr[c + 1] : colorArr[0];

        for (int t = 0; t < trailLength; t++) {
            int trailPos = (chaserHead - t + NUM_LEDS) % NUM_LEDS;
            uint8_t brightness = map(t, 0, trailLength, 255, 100);
            leds[trailPos] = chaserColor;
            leds[trailPos].nscale8(brightness);
        }
    }

    FastLED.show();
    stripPattern++;
    if (stripPattern >= NUM_LEDS) stripPattern = 0;
  }
}

//Category Specific lights
void sportsLights(int sport) { // a general light animation for sports,
  if (sport == 1) { // basketball
    stripSetup(Basketball, 4, 2);
    stripAnimation(Basketball, 4, 1);
    blinkAnimation(1);
  }
  else if (sport == 2) { // soccer


  }
  else if (sport == 3) { // rowing

  }
}

void academicLights(int department) { // lights for different departments at school
  if (department == 1) { //science
    stripAnimation(Science, 2, 3);
    blinkAnimation(1);
  }
}

void clubLights(int club) { //club win animation
  if (club == 1) { // FASU
    stripSetup(FASU, 3, 2);
    stripAnimation(FASU, 3, 2);
    blinkAnimation(2);
  }
}

//create a function that takes in paramaters for light animation
//chasing lights animation
/** 
 params: 
  CRGB colors[] - array of colors to use
  int numColors - number of colors in the array
  int stripSetup - what strip light setup; 1 - colors divided evenly across marquee, 2 - colors alternating 
  int blinkAnimation - What type of blink animation for heart to use; 1 - no blink, 1 - blinkanimation1 (on and off), 2 - blinkanimation2 (heartbeat)
  int stripAnimation - What strip animation to do; 0 - no animation (static), 1 - blink, 2- moving light animation, 3- chasing lights (overrides strip setup)
*/
void makeLightAnimation(CRGB colorArr[], int numColors, int stripSetupOption, int blinkAnimationOption, int stripAnimationOption) {
  
  // ---- Strip Setup (only runs once on first call or when reset) ----
 if (!stripInitialized && stripAnimationOption != 3) {
    stripSetup(colorArr, numColors, stripSetupOption);
    stripInitialized = true;
  }
  stripAnimation(colorArr, numColors, stripAnimationOption);
  blinkAnimation(blinkAnimationOption);
}


void setup() {
  Serial.begin(115200);
  pinMode(onSwitch, INPUT_PULLUP);
  pinMode(heartLight, OUTPUT);
  myDisplay.begin();
  myDisplay.setIntensity(0);
  myDisplay.displayClear();
    // WS2815 is natively supported in FastLED
  FastLED.addLeds<WS2815, STRIP_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(0); // 0-255
  randomSeed(analogRead(A0)); 

}

void loop() {

  // //Testing Light Code

  // turnLightsOn();

  // while (!myDisplay.displayAnimate()) {
  //   // just keep pumping the animation forward
  // }

  // delay(500);
  
  // FastLED.clear();
  // fill_solid(leds, NUM_LEDS, CRGB::White);
  // FastLED.show();

  // delay(500);

  // for(int i = 0; i < 3; i++) {
  //   for (int j = 0; j < 2; j++) {
  //     digitalWrite(heartLight, HIGH);
  //     delay(100);
  //     digitalWrite(heartLight, LOW);
  //     delay(100);
  //     digitalWrite(heartLight, HIGH);
  //   }
  //   delay(1000);
  // } 

  // delay(3000);

  // turnLightsOff();

  // while (!myDisplay.displayAnimate()) {
  //   // wait for this scroll to finish too
  // }
  // delay(1000);


  static unsigned long lastDebugPrint = 0;
  if (millis() - lastDebugPrint > 500) {
    lastDebugPrint = millis();
    Serial.print("buttonState: "); Serial.print(buttonState);
    Serial.print(" | lightOn: "); Serial.print(lightOn);
    Serial.print(" | holdTime: "); 
    Serial.print(pressStartTime != 0 ? millis() - pressStartTime : 0);
    Serial.print(" | holdTriggered: "); Serial.println(holdTriggered);
  }
  if (lightOn && animationPlaying) {
      makeLightAnimation(chosenColors, chosenNumColors, chosenStripSetup, chosenBlinkAnim, chosenStripAnim); //Colors, numColors, strip setup, blink animation, strip animation
      if (myDisplay.displayAnimate())
      {
        currentPhrase = (currentPhrase + 1) % numPhrases;
        myDisplay.displayReset();
        myDisplay.displayText( phrases[currentPhrase].c_str(), PA_CENTER, 100, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
      }
  }
  // ---------- Debounce ----------
  bool reading = digitalRead(onSwitch);

  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }
  lastButtonReading = reading;
  
  if (millis() - lastDebounceTime > debounceDelay && reading != buttonState) {
    buttonState = reading;

    if (buttonState == LOW) {
      // Just pressed
      pressStartTime = millis();
      holdTriggered = false;
    } else {
      //makes animation
      holdTime = millis() - pressStartTime;
      pressStartTime = 0;

      if (lightOn && !holdTriggered && holdTime >= 3000 && holdTime < 7000) {
        resetAnimation();
        int chosenSet = random(0, NUM_COLOR_SETS);
        chosenColors = colorSets[chosenSet];
        chosenNumColors = colorSetSizes[chosenSet];
        chosenStripSetup = random(1, 3);
        chosenBlinkAnim = random(0, 3);
        chosenStripAnim = random(1, 4);
        animationPlaying = true;
      }
      holdTime = 0; // ← clear it after using it
    }
  }

  // ---------- While Held — fire turn on / turn off at threshold ----------
  if (buttonState == LOW && pressStartTime != 0) {
    holdTime = millis() - pressStartTime;

   if (!holdTriggered) {
    if (!lightOn && holdTime >= 3000) {
        holdTriggered = true;
        pressStartTime = 0; // ← add this
        turnLightsOn();
        lightOn = true;
    }
    else if (lightOn && holdTime >= 7000) {
        holdTriggered = true;
        pressStartTime = 0; // ← add this
        turnLightsOff();
        lightOn = false;
        animationPlaying = false;
        resetAnimation();
      }
    }
  }
}
