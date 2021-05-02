// KY404 Knob
#include <ButtonEvents.h>
ButtonEvents knobButton;
const byte knobButtonPin = 4; // Uno Pin 3 - ESP Pin 4
int CLK_PIN = 0; // Uno Pin 2 - ESP Pin 0
int DT_PIN = 5; // Uno Pin 4 - ESP Pin 5
boolean knobUpdated;
int knobRotation;
int knobRotationLastCycle = 0;

// Variables to debounce Rotary Encoder
long timeOfLastDebounce = 0;
int delayofDebounce = 0.01;
// Store previous Pins state
int previousCLK;   
int previousDATA;
int knob=0; // Store current counter value
int knobRaw;
int knobLastCycle;
boolean knobIncreased;
boolean knobDecreased;

// LED STRIPE
// The whole stripe of the NUMPIXEL will be splitted in multiple "stripes" defined by LEDS_PER_STRIPE
#define STRIPES 5 
const int STRIPES_START_ADDRESS[] = {0, 24, 46, 70, 92};
#define LEDS_PER_STRIPE 10
boolean stripesStates[] = {true, true, true, true, true};
int brightness = 125;
const byte minBrightness = 25;
const byte maxBrightness = 255;

#include <Adafruit_NeoPixel.h>
#define NEO_PIN    12 // Uno Pin 6 - ESP Pin 12
#define NEO_PTYPE  NEO_GRBW
#define NUMPIXELS  115
// Waittimes for the LED stripe to change effects
#define SSWAIT   5
#define SWAIT   25
#define LWAIT   50
#define HWAIT   500
#define IWAIT   2000
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, NEO_PIN, NEO_PTYPE + NEO_KHZ800);
// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

// NOTE: RGBW LEDs draw up to 80mA with all colors + white at full brightness!
// That means that a 60-pixel strip can draw up to 60x 80 = 4800mA, so you
// should use a 5A power supply; for a 144-pixel strip its max 11520mA = 12A!

// COLOR VARS for NEO Pixels
uint32_t warmWhite = strip.Color(0, 0, 0, 255);
uint32_t warmYellow = strip.Color(200, 200, 0, 255);
uint32_t warmOrange = strip.Color(255, 145, 0, 255);
uint32_t warmRed = strip.Color(200, 0, 0, 255);
uint32_t warmPink = strip.Color(200, 0, 200, 255);
uint32_t warmBlue = strip.Color(0, 0, 200, 255);
uint32_t red = strip.Color(255, 0, 0, 0);
uint32_t orange = strip.Color(255, 145, 0, 0);
uint32_t yellow = strip.Color(255, 255, 0, 0);
uint32_t green = strip.Color(0, 255, 0, 0);
uint32_t blue = strip.Color(0, 0, 255, 0);
uint32_t aqua = strip.Color(0, 220, 255, 0);
uint32_t pink = strip.Color(255, 0, 255, 0);
uint32_t purple = strip.Color(155, 0, 255, 0);
uint32_t black = strip.Color(0, 0, 0, 0);
uint32_t colors[] = {warmWhite, warmYellow, warmOrange, warmRed, warmPink, warmBlue, red, orange, yellow, green, blue, aqua, pink, purple};
int maxColor = 13;
int minColor = 0;
int selectedColor = 0;
int selectedColorLastCycle;
boolean changeColor = false;

boolean ledStripeState;

// modes
boolean brightnessMode = true;
boolean colorMode = false;
boolean patternMode = false;

// patterns
int minPattern = 0;
int maxPattern = 20;
int selectedPattern = 0;
int selectedPatternLastCycle;
boolean changePattern = false;

void setup() { 
  // put your setup code here, to run once:
  Serial.begin (9600);

  // KY404 Knob / Button
  pinMode(knobButtonPin, INPUT_PULLUP);
  knobButton.attach(knobButtonPin);
  pinMode(CLK_PIN, INPUT);
  pinMode(DT_PIN, INPUT);
  knobRotation = digitalRead(CLK_PIN);

  // Adafriut LED STRIPE
  #ifdef __AVR_ATtiny85__
    // This is for Trinket 5V 16MHz
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
    // End of trinket special code
  #endif
    
  strip.begin(); // This initializes the NeoPixel library.
  strip.setBrightness(brightness); // set brightness
  strip.show(); // Initialize all pixels to 'off'
  
  #ifdef IWAIT
    delay(IWAIT);
  #endif
}

void loop() {
  // put your main code here, to run repeatedly:
  knobButton.update();

  // If enough time has passed check the rotary encoder
  if ((millis() - timeOfLastDebounce) > delayofDebounce) {
    updateKnob();  // Rotary Encoder check routine below
    
    previousCLK=digitalRead(CLK_PIN);
    previousDATA=digitalRead(DT_PIN);
    
    timeOfLastDebounce=millis();  // Set variable to current millis() timer
  }

  if(knobButton.tapped() == true){    
    Serial.println("single tap");
  
    colorMode = !colorMode;
    
    if(colorMode){       
      patternMode = false;
      brightnessMode = false;
    } else {
      brightnessMode = true;
    }

    Serial.print("colorMode: ");
    Serial.println(colorMode);
    Serial.print("brightnessMode: ");
    Serial.println(brightnessMode);
  } 

  if(knobButton.doubleTapped() == true){
    Serial.println("Touble Tap");  
    patternMode = !patternMode;

    if(patternMode){
      colorMode = false;
      brightnessMode = false;
    } else {
      brightnessMode = true;
    }

    Serial.print("paternMode: ");
    Serial.println(patternMode);
    Serial.print("brightnessMode: ");
    Serial.println(brightnessMode);
  }

  if(knobButton.held() == true){
    Serial.println("Hold Tap"); 
    ledStripeState = !ledStripeState;

    if(ledStripeState){
      stripeColorWipe(warmWhite, SSWAIT);
    } else {
      stripeColorWipe(black, SSWAIT); 
    }   
  }

  if(knob != knobLastCycle){
    knobIncreased = knob > knobLastCycle;
    knobDecreased = knob < knobLastCycle;

    Serial.print("inc ");
    Serial.println(knobIncreased);
    Serial.print("dec ");
    Serial.println(knobDecreased);

    
    if(brightnessMode){
      if(knobIncreased) {brightness += 10;}
      if(knobDecreased) {brightness -= 10;}
      if(brightness > maxBrightness){brightness = maxBrightness;}
      Serial.print("between: ");
      Serial.println(brightness);
      if(brightness < minBrightness){brightness = minBrightness;}
      
      setLedsBrightness(brightness); 

      Serial.print("brightness: ");
      Serial.println(brightness);
    }

    if(colorMode){
      if(knobIncreased) {selectedColor++;}
      if(knobDecreased) {selectedColor--;}
      if(selectedColor > maxColor){selectedColor = maxColor;}
      if(selectedColor < minColor){selectedColor = minColor;}

      changeColor = selectedColor != selectedColorLastCycle;
      if(changeColor){
        setStripsColor(colors[selectedColor]);
        changeColor = false;
      }
     
      Serial.print("selectedColor: ");
      Serial.println(selectedColor);

      selectedColorLastCycle = selectedColor;
    }

    if(patternMode){
       if(knobIncreased) {selectedPattern++;}
      if(knobDecreased) {selectedPattern--;}
      if(selectedPattern > maxPattern){selectedPattern = maxPattern;}
      if(selectedPattern < minPattern){selectedPattern = minPattern;}

      changePattern = selectedPattern != selectedPatternLastCycle;
      if(changePattern){
        setStripePattern(colors[selectedColor], selectedPattern);
        changePattern = false;
      }

      selectedPatternLastCycle = selectedPattern;
    }
  }

  knobLastCycle = knob;
}

void updateKnob() {
  // knobRaw is necesarry because the knob is not updated when the knob 
  // makes the click wheel turn, it also happens in between
  // with the modulo function it only happens at the clicky feeling
  
  if ((previousCLK == 0) && (previousDATA == 1)) {
    if ((digitalRead(CLK_PIN) == 1) && (digitalRead(DT_PIN) == 0)) {
      knobRaw++;

      if(knobRaw%2 == 0){
        knob++;  
      }
    }
    if ((digitalRead(CLK_PIN) == 1) && (digitalRead(DT_PIN) == 1)) {
      knobRaw--;
      if(knobRaw%2 == 0){
        knob--;  
      }
    }
  }
  
  if ((previousCLK == 1) && (previousDATA == 0)) {
      if ((digitalRead(CLK_PIN) == 0) && (digitalRead(DT_PIN) == 1)) {
        knobRaw++;
       if(knobRaw%2 == 0){
          knob++;  
        }
      }
      if ((digitalRead(CLK_PIN) == 0) && (digitalRead(DT_PIN) == 0)) {
        knobRaw--;
        if(knobRaw%2 == 0){
          knob--;  
        }
      }
    }
  
  if ((previousCLK == 1) && (previousDATA == 1)) {
      if ((digitalRead(CLK_PIN) == 0) && (digitalRead(DT_PIN) == 1)) {
        knobRaw++;
        if(knobRaw%2 == 0){
          knob++;  
        }
      }
      if ((digitalRead(CLK_PIN) == 0) && (digitalRead(DT_PIN) == 0)) {
        knobRaw--;
        if(knobRaw%2 == 0){
          knob--;  
        }
      }
    }  
  
  if ((previousCLK == 0) && (previousDATA == 0)) {
      if ((digitalRead(CLK_PIN) == 1) && (digitalRead(DT_PIN) == 0)) {
        knobRaw++;
        if(knobRaw%2 == 0){
          knob++;  
        }
      }
      if ((digitalRead(CLK_PIN) == 1) && (digitalRead(DT_PIN) == 1)) {
        knobRaw--;
        if(knobRaw%2 == 0){
          knob--;  
        }
      }
    }            
 }

void stripeColorWipe(uint32_t color, int wait) {
  for(int ledNumber=0; ledNumber<LEDS_PER_STRIPE; ledNumber++){
    for(int stripeNumber=0; stripeNumber<STRIPES; stripeNumber++){
      if(stripesStates[stripeNumber]){
        wipeLedColorByAdress((STRIPES_START_ADDRESS[stripeNumber]+ledNumber), color);                       
      } else {
        wipeLedColorByAdress((STRIPES_START_ADDRESS[stripeNumber]+ledNumber), black); 
      }
    }
  }
}

void setStripsColor(uint32_t color){
  for(int ledNumber=0; ledNumber<LEDS_PER_STRIPE; ledNumber++){
    for(int stripeNumber=0; stripeNumber<STRIPES; stripeNumber++){
      if(stripesStates[stripeNumber]){
        strip.setPixelColor((STRIPES_START_ADDRESS[stripeNumber]+ledNumber), color);                       
      } else {
        strip.setPixelColor((STRIPES_START_ADDRESS[stripeNumber]+ledNumber), black); 
      }
    }
  }
  strip.show();    
}

void wipeLedColorByAdress(int address, uint32_t color){
  strip.setPixelColor(address, color);
  strip.show();                          
  delay(SWAIT);   
}

void setLedsBrightness(byte value) {  
  strip.setBrightness(value);
  strip.show(); 
  delay(SWAIT); 
}

void setStripePattern(uint32_t color, int pattern) {
  selectStripPattern(pattern);
  
  for(int ledNumber=0; ledNumber<LEDS_PER_STRIPE; ledNumber++){
    for(int stripeNumber=0; stripeNumber<STRIPES; stripeNumber++){
      if(stripesStates[stripeNumber]){
        strip.setPixelColor((STRIPES_START_ADDRESS[stripeNumber]+ledNumber), color);                       
      } else {
        strip.setPixelColor((STRIPES_START_ADDRESS[stripeNumber]+ledNumber), black); 
      }
    }
  }
  strip.show();    
}

void selectStripPattern(int patternNumber){
  switch(patternNumber){
    case 0: stripesStates[0] = true;  stripesStates[1] = true;  stripesStates[2] = true;  stripesStates[3] = true;  stripesStates[4] = true; break;
    
    case 1: stripesStates[0] = true;  stripesStates[1] = true;  stripesStates[2] = true;  stripesStates[3] = false; stripesStates[4] = true; break;
    case 2: stripesStates[0] = true;  stripesStates[1] = true;  stripesStates[2] = true;  stripesStates[3] = true;  stripesStates[4] = false;break;
    case 3: stripesStates[0] = false; stripesStates[1] = true;  stripesStates[2] = true;  stripesStates[3] = true;  stripesStates[4] = true; break;
    case 4: stripesStates[0] = true;  stripesStates[1] = false; stripesStates[2] = true;  stripesStates[3] = true;  stripesStates[4] = true; break;
    case 5: stripesStates[0] = true;  stripesStates[1] = true;  stripesStates[2] = false; stripesStates[3] = true;  stripesStates[4] = true; break;
    
    case 6: stripesStates[0] = true;  stripesStates[1] = true;  stripesStates[2] = true;  stripesStates[3] = false; stripesStates[4] = false;break;
    case 7: stripesStates[0] = false;  stripesStates[1] = true;  stripesStates[2] = true;  stripesStates[3] = true;  stripesStates[4] = false; break;
    case 8: stripesStates[0] = false;  stripesStates[1] = false;  stripesStates[2] = true;  stripesStates[3] = true;  stripesStates[4] = true; break;
    case 9: stripesStates[0] = true;  stripesStates[1] = false;  stripesStates[2] = false;  stripesStates[3] = true;  stripesStates[4] = true; break;
    case 10: stripesStates[0] = true;  stripesStates[1] = true;  stripesStates[2] = false;  stripesStates[3] = false;  stripesStates[4] = true; break;

    case 11: stripesStates[0] = false;  stripesStates[1] = false;  stripesStates[2] = true;  stripesStates[3] = true;  stripesStates[4] = false; break;
    case 12: stripesStates[0] = false;  stripesStates[1] = false;  stripesStates[2] = false;  stripesStates[3] = true;  stripesStates[4] = true; break;
    case 13: stripesStates[0] = true;  stripesStates[1] = false;  stripesStates[2] = false;  stripesStates[3] = false;  stripesStates[4] = true; break;
    case 14: stripesStates[0] = true;  stripesStates[1] = true;  stripesStates[2] = false;  stripesStates[3] = false;  stripesStates[4] = false; break;
    case 15: stripesStates[0] = false;  stripesStates[1] = true;  stripesStates[2] = true;  stripesStates[3] = false;  stripesStates[4] = false; break;

    case 16: stripesStates[0] = false;  stripesStates[1] = false;  stripesStates[2] = true;  stripesStates[3] = false;  stripesStates[4] = false; break;
    case 17: stripesStates[0] = false;  stripesStates[1] = false;  stripesStates[2] = false;  stripesStates[3] = true;  stripesStates[4] = false; break;
    case 18: stripesStates[0] = false;  stripesStates[1] = false;  stripesStates[2] = false;  stripesStates[3] = false;  stripesStates[4] = true; break;
    case 19: stripesStates[0] = true;  stripesStates[1] = false;  stripesStates[2] = false;  stripesStates[3] = false;  stripesStates[4] = false; break;
    case 20: stripesStates[0] = false;  stripesStates[1] = true;  stripesStates[2] = false;  stripesStates[3] = false;  stripesStates[4] = false; break;
  }
}
