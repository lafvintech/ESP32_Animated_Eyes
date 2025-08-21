//#define SYMMETRICAL_EYELID

// Enable ONE of these #includes -- HUGE graphics tables for various eyes:
#include "custom.h"

char CUR_MODE = 'c'; //c is GIF playback mode (three-position switch 3, esp32-cam positive connects to expansion board power input positive, switch 3 position grounded)  m is manual mode  a is auto mode
#define AUTO_PIN 26 //Auto mode pin, three-position switch 1
#define MANUAL_PIN 27 //Manual mode pin, three-position switch 2  
#define CHANGE_PIN 25 //Change eye color button pin (interrupt triggered)
volatile int pressCount = 0 ; // Count of eye color change button presses
 

// DISPLAY HARDWARE SETTINGS (screen type & connections) -------------------
#define TFT_COUNT 2        // Number of screens (1 or 2)
#define TFT1_CS 22         // TFT 1 chip select pin (set to -1 to use TFT_eSPI setup)
#define TFT2_CS 21         // TFT 2 chip select pin (set to -1 to use TFT_eSPI setup)
#define TFT_1_ROT 3        // TFT 1 rotation
#define TFT_2_ROT 1        // TFT 2 rotation
#define EYE_1_XPOSITION  0 // x shift for eye 1 image on display
#define EYE_2_XPOSITION  0 // x shift for eye 2 image on display

#define DISPLAY_BACKLIGHT  -1 // Pin for backlight control (-1 for none)
#define BACKLIGHT_MAX    255

// EYE LIST ----------------------------------------------------------------
#define NUM_EYES 2 // Number of eyes to display (1 or 2)

//#define BLINK_PIN   -1 // Pin for manual blink button (BOTH eyes)
#define BLINK_PIN  13  //Joystick button SW
#define LH_WINK_PIN -1 // Left wink pin (set to -1 for no pin)
#define RH_WINK_PIN -1 // Right wink pin (set to -1 for no pin)

// This table contains ONE LINE PER EYE.  The table MUST be present with
// this name and contain ONE OR MORE lines.  Each line contains THREE items:
// a pin number for the corresponding TFT/OLED display's SELECT line, a pin
// pin number for that eye's "wink" button (or -1 if not used), a screen
// rotation value (0-3) and x position offset for that eye.

#if (NUM_EYES == 2)
eyeInfo_t eyeInfo[] = {
  { TFT1_CS, LH_WINK_PIN, TFT_1_ROT, EYE_1_XPOSITION }, // LEFT EYE chip select and wink pins, rotation and offset
  { TFT2_CS, RH_WINK_PIN, TFT_2_ROT, EYE_2_XPOSITION }, // RIGHT EYE chip select and wink pins, rotation and offset
};
#else
eyeInfo_t eyeInfo[] = {
  { TFT1_CS, LH_WINK_PIN, TFT_1_ROT, EYE_1_XPOSITION }, // EYE chip select and wink pins, rotation and offset
};
#endif

// INPUT SETTINGS (for controlling eye motion) -----------------------------

// JOYSTICK_X_PIN and JOYSTICK_Y_PIN specify analog input pins for manually
// controlling the eye with an analog joystick.  If set to -1 or if not
// defined, the eye will move on its own.
// IRIS_PIN speficies an analog input pin for a photocell to make pupils
// react to light (or potentiometer for manual control).  If set to -1 or
// if not defined, the pupils will change on their own.
// BLINK_PIN specifies an input pin for a button (to ground) that will
// make any/all eyes blink.  If set to -1 or if not defined, the eyes will
// only blink if AUTOBLINK is defined, or if the eyeInfo[] table above
// includes wink button settings for each eye.


#define JOYSTICK_X_PIN 33 // Joystick X-axis
#define JOYSTICK_Y_PIN 32 // Joystick Y-axis

#define JOYSTICK_X_FLIP   // If defined, reverse stick X axis
#define JOYSTICK_Y_FLIP   // If defined, reverse stick Y axis
#define TRACKING            // If defined, eyelid tracks pupil
#define AUTOBLINK           // If defined, eyes also blink autonomously

// GIF playback mode related definitions (for mode c)
#define SD_CS_PIN 12       // SD card chip select pin
#define GIF_BUTTON_PIN 25  // GIF switch button pin (reuse CHANGE_PIN)

// Onboard LED pin definition (for status indication)
// LED indication rules:
// - Mode switching: Fast flash 3 times indicates accepting switch request
// - Processing: Slow flash (500ms interval) indicates processing
// - Complete: LED turns off
#ifndef LED_BUILTIN
#define LED_BUILTIN 2      // ESP32 onboard LED pin
#endif

// GIF file management variables (for mode c)
extern int currentGifIndex;     // Current displayed GIF index (starts from 1)
extern int totalGifCount;       // Total number of GIF files in SD card
extern String currentFilename;  // Current GIF filename
extern volatile bool gifButtonPressed; // GIF button status flag
extern unsigned long lastGifButtonPress; // GIF button debounce time

//  #define LIGHT_PIN      -1 // Light sensor pin
#define LIGHT_CURVE  0.33 // Light sensor adjustment curve
#define LIGHT_MIN       0 // Minimum useful reading from light sensor
#define LIGHT_MAX    1023 // Maximum useful reading from sensor

#define IRIS_SMOOTH         // If enabled, filter input from IRIS_PIN
#if !defined(IRIS_MIN)      // Each eye might have its own MIN/MAX
#define IRIS_MIN       70 // Iris size (0-1023) in brightest light 90
#endif
#if !defined(IRIS_MAX)
#define IRIS_MAX      120 // Iris size (0-1023) in darkest light  130
#endif
