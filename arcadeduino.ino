#include "HID-Project.h"

const int selectorPins = 2;
const int sigPin = 6;

// ShiftPWM uses timer1 by default. To use a different timer, before '#include <ShiftPWM.h>', add
// #define SHIFTPWM_USE_TIMER2  // for Arduino Uno and earlier (Atmega328)
// #define SHIFTPWM_USE_TIMER3  // for Arduino Micro/Leonardo (Atmega32u4)

// Clock and data pins are pins from the hardware SPI, you cannot choose them yourself.
// Data pin is MOSI (Uno and earlier: 11, Leonardo: ICSP 4, Mega: 51, Teensy 2.0: 2, Teensy 2.0++: 22)
// Clock pin is SCK (Uno and earlier: 13, Leonardo: ICSP 3, Mega: 52, Teensy 2.0: 1, Teensy 2.0++: 21)

// You can choose the latch pin yourself.
const int ShiftPWM_latchPin=8;

// ** uncomment this part to NOT use the SPI port and change the pin numbers. This is 2.5x slower **
// #define SHIFTPWM_NOSPI
// const int ShiftPWM_dataPin = 11;
// const int ShiftPWM_clockPin = 13;

// If your LED's turn on if the pin is low, set this to true, otherwise set it to false.
const bool ShiftPWM_invertOutputs = true;

// You can enable the option below to shift the PWM phase of each shift register by 8 compared to the previous.
// This will slightly increase the interrupt load, but will prevent all PWM signals from becoming high at the same time.
// This will be a bit easier on your power supply, because the current peaks are distributed.
const bool ShiftPWM_balanceLoad = false;

#include <ShiftPWM.h>   // include ShiftPWM.h after setting the pins!

// Here you set the number of brightness levels, the update frequency and the number of shift registers.
// These values affect the load of ShiftPWM.
// Choose them wisely and use the PrintInterruptLoad() function to verify your load.
unsigned char maxBrightness = 255;
unsigned char pwmFrequency = 75;
const unsigned int numRegisters = 3;
unsigned int numOutputs = numRegisters*8;
const unsigned int numRGBLeds = numRegisters*8/3;

float press_dim = 0.02;
float fade_in_amt = 0.004;
float leds_fade[numRGBLeds] = {0.0};
float led_colors[numRGBLeds][3] = {
  {255, 0,   0},
  {0,   255, 0},
  {255, 255, 0},
  {0,   0,   255},
  {0,   255, 255},
  {255, 0,   255},
  {255, 64,  64},
  {255, 255, 255}
};

void setup() {
  Serial.begin(9600);

  // Sets the number of 8-bit registers that are used.
  ShiftPWM.SetAmountOfRegisters(numRegisters);

  // SetPinGrouping allows flexibility in LED setup.
  // If your LED's are connected like this: RRRRGGGGBBBBRRRRGGGGBBBB, use SetPinGrouping(4).
  ShiftPWM.SetPinGrouping(1); //This is the default, but I added here to demonstrate how to use the funtion

  ShiftPWM.Start(pwmFrequency, maxBrightness);
  //printInstructions();

  for (int i = 0; i < 4; i++) {
    pinMode(selectorPins + i, OUTPUT);
  }

  pinMode(sigPin, INPUT_PULLUP);

  // Sends a clean report to the host. This is important on any Arduino type.
  Gamepad.begin();
}

bool buttons[16] = {false};

void loop() {
  // fade in each of the leds one step
  for (unsigned int i = 0; i < numRGBLeds; i++) {
    leds_fade[i] = min(1.0, leds_fade[i] + (1 - leds_fade[i]) * fade_in_amt);
  }

  Gamepad.releaseAll();

  bool right = false;
  bool up = false;
  bool down = false;
  bool left = false;

  for (unsigned int i = 0; i < 16; i++) {
    // choose correct pin
    digitalWrite(selectorPins + 0, i & 0b0001 ? HIGH : LOW);
    digitalWrite(selectorPins + 1, i & 0b0010 ? HIGH : LOW);
    digitalWrite(selectorPins + 2, i & 0b0100 ? HIGH : LOW);
    digitalWrite(selectorPins + 3, i & 0b1000 ? HIGH : LOW);

    int pressed = !digitalRead(sigPin);

    buttons[i] = pressed;
    
    if (pressed) {
      if (i >= 12) {
        // directional input
        right |= i == 12;
        up    |= i == 13;
        down  |= i == 14;
        left  |= i == 15;
      } else {
        // buttons
        Gamepad.press(i + 1);
      }

      if (i < numRGBLeds) {
        // dim out corresponding led
        leds_fade[i] = press_dim;
      }
    }

    if (i < numRGBLeds) {
      // set button led color
      float fade = leds_fade[i];
      ShiftPWM.SetRGB(i, led_colors[i][0] * fade, led_colors[i][1] * fade, led_colors[i][2] * fade);
    }
  }

  if (up && right) {
    Gamepad.dPad2(GAMEPAD_DPAD_UP_RIGHT);
  } else if (up && left) {
    Gamepad.dPad2(GAMEPAD_DPAD_UP_LEFT);
  } else if (down && right) {
    Gamepad.dPad2(GAMEPAD_DPAD_DOWN_RIGHT);
  } else if (down && left) {
    Gamepad.dPad2(GAMEPAD_DPAD_DOWN_LEFT);
  } else if (up) {
    Gamepad.dPad2(GAMEPAD_DPAD_UP);
  } else if (down) {
    Gamepad.dPad2(GAMEPAD_DPAD_DOWN);
  } else if (right) {
    Gamepad.dPad2(GAMEPAD_DPAD_RIGHT);
  } else if (left) {
    Gamepad.dPad2(GAMEPAD_DPAD_LEFT);
  } else {
    Gamepad.dPad2(GAMEPAD_DPAD_CENTERED);
  }

  String buttons_s = "";

  for (int i = 0; i < 16; i++) {
    buttons_s += (buttons[i] ? "1 " : "0 ");
  }
  Serial.println(String(millis()) + "\t" + buttons_s);
  if (USBDevice.configured()) {
    Gamepad.write();
  }
}
