#define USE_DMA

// Load TFT driver library
#include <SPI.h>
#include <TFT_eSPI.h>
#include <string.h>  // Add header file for strlen function

// Loading text display related constants
#define LOADING_TEXT "Gif Loading....."
#define SD_ERROR_TEXT "SD Card Error!"
#define GIF_ERROR_TEXT "GIF Read Failed!"
#define SYSTEM_START_TEXT "System Starting..."
#define MODE_SWITCH_TEXT "Mode Switching..."
#define LOADING_TEXT_SIZE 2  // Text size
#define ERROR_TEXT_SIZE 2    // Error text size
#define INFO_TEXT_SIZE 2     // Info text size
#define LOADING_TEXT_COLOR TFT_GREEN   // Loading text color (green)
#define ERROR_TEXT_COLOR TFT_RED      // Error text color (red)
#define INFO_TEXT_COLOR TFT_CYAN     // Info text color (cyan)
#define LOADING_BG_COLOR TFT_BLACK

// Function forward declarations
void showInfoText(const char* infoText, uint16_t textColor, uint8_t textSize, uint16_t delayTime = 0);
void showErrorText(const char* errorText);
void showLoading();
void handleGifSwitchLED();

// GIF playback related libraries (for mode c)
#include <SD.h>
#include <FS.h>
#include <SPIFFS.h>
#include <AnimatedGIF.h>

TFT_eSPI tft;           // A single instance is used for 1 or 2 displays

// GIF playback buffer definitions
#define DISPLAY_WIDTH  tft.width()
#define DISPLAY_HEIGHT tft.height()
#define BUFFER_SIZE 256            // Optimum is >= GIF width or integral division of width

#ifdef USE_DMA
  uint16_t usTemp[2][BUFFER_SIZE]; // Global to support DMA use
#else
  uint16_t usTemp[1][BUFFER_SIZE];    // Global to support DMA use
#endif
bool     dmaBuf = 0;

// A pixel buffer is used during eye rendering
#define BUFFER_SIZE_EYES 1024 // 128 to 1024 seems optimum

#ifdef USE_DMA
#define BUFFERS 2      // 2 toggle buffers with DMA
#else
#define BUFFERS 1      // 1 buffer for no DMA
#endif

uint16_t pbuffer[BUFFERS][BUFFER_SIZE_EYES]; // Pixel rendering buffer for eyes
bool     dmaBufEyes   = 0;                  // DMA buffer selection for eyes

// This struct is populated in config.h
typedef struct {        // Struct is defined before including config.h --
  int8_t  select;       // pin numbers for each eye's screen select line
  int8_t  wink;         // and wink button (or -1 if none) specified there,
  uint8_t rotation;     // also display rotation and the x offset
  int16_t xposition;    // position of eye on the screen
} eyeInfo_t;

#include "config.h"     // ****** CONFIGURATION IS DONE IN HERE ******

extern void user_setup(void); // Functions in the user*.cpp files
extern void user_loop(void);
extern int getX(void);
extern int getY(void);

#define SCREEN_X_START 0
#define SCREEN_X_END   SCREEN_WIDTH   // Badly named, actually the "eye" width!
#define SCREEN_Y_START 0
#define SCREEN_Y_END   SCREEN_HEIGHT  // Actually "eye" height

// A simple state machine is used to control eye blinks/winks:
#define NOBLINK 0       // Not currently engaged in a blink
#define ENBLINK 1       // Eyelid is currently closing
#define DEBLINK 2       // Eyelid is currently opening
typedef struct {
  uint8_t  state;       // NOBLINK/ENBLINK/DEBLINK
  uint32_t duration;    // Duration of blink state (micros)
  uint32_t startTime;   // Time (micros) of last state change
} eyeBlink;

struct {                // One-per-eye structure
  int16_t   tft_cs;     // Chip select pin for each display
  eyeBlink  blink;      // Current blink/wink state
  int16_t   xposition;  // x position of eye image
} eye[NUM_EYES];

uint32_t startTime;  // For FPS indicator

bool ischange = false;
TaskHandle_t Task_HandleOne = NULL;

// GIF playback related global variables (for mode c)
AnimatedGIF gif;
File gifFile;
int currentGifIndex = 1;
int totalGifCount = 0;
String currentFilename;
volatile bool gifButtonPressed = false;
unsigned long lastGifButtonPress = 0;

// SD card initialization status flags
bool sdCardInitialized = false;
bool spiffsInitialized = false;

// LED status control global variables
bool systemReadyBlinking = false;  // Continuous blinking state after system ready
bool gifSwitchInProgress = false;
bool gifPlayingSuccessfully = false;
unsigned long lastGifLedToggle = 0;
bool gifLedState = false;
int gifLedInterval = 300; // Blinking interval (milliseconds)

// Function forward declarations moved to file beginning

void func1() {
  unsigned long currentTime = millis();
  
  if (CUR_MODE == 'c') {
    // Mode c: GIF switching function
    if (currentTime - lastGifButtonPress > 200) { // Debounce: 200ms
      gifButtonPressed = true;
      lastGifButtonPress = currentTime;
    }
  } else {
    // Mode a and m: Eye color switching function
    if (pressCount + 1 > 5)
      pressCount = 0;
    else
      pressCount++;
    
    Serial.println("Eye color changed to: " + String(pressCount));
  }
}

// GIF playback related functions (for mode c)
// Count the number of GIF files in SD card
int countGifFiles() {
  int count = 0;
  for (int i = 1; i <= 100; i++) { // Check up to 100 files
    String filename = "/" + String(i) + ".gif";
    if (SD.exists(filename)) {
      count = i; // Record the largest file number
    }
  }
  return count;
}

// Update current GIF filename
void updateCurrentFilename() {
  currentFilename = "/" + String(currentGifIndex) + ".gif";
}

// Copy GIF file to SPIFFS
bool copyGifToSpiffs(String filename) {
  // Ensure SD card CS pin state is correct
  digitalWrite(SD_CS_PIN, HIGH);
  delay(10);
  
  File sdFile = SD.open(filename);
  if (!sdFile) {
    Serial.println("Cannot open GIF file from SD card: " + filename);
    return false;
  }

  File spiffsFile = SPIFFS.open("/current.gif", FILE_WRITE, true);
  if (!spiffsFile) {
    Serial.println("Cannot create GIF file in SPIFFS!");
    sdFile.close();
    return false;
  }

  Serial.println("Copying " + filename + " to SPIFFS...");
  
      // At file copy stage, stop system ready blinking, start file copy blinking
    systemReadyBlinking = false;
  
  byte buffer[512];
  size_t totalBytes = 0;
  
  while (sdFile.available()) {
    int bytesRead = sdFile.read(buffer, sizeof(buffer));
    if (bytesRead > 0) {
      spiffsFile.write(buffer, bytesRead);
      totalBytes += bytesRead;
      
      // Call LED control function during file copy process
      handleGifSwitchLED();
    }
  }

  spiffsFile.close();
  sdFile.close();
  
  Serial.println("Copy completed, total bytes: " + String(totalBytes));
  return true;
}

// GIF drawing callback function
void GIFDraw(GIFDRAW *pDraw) {
  uint8_t *s;
  uint16_t *d, *usPalette;
  int x, y, iWidth, iCount;

  // Display bounds check and cropping
  iWidth = pDraw->iWidth;
  if (iWidth + pDraw->iX > DISPLAY_WIDTH)
    iWidth = DISPLAY_WIDTH - pDraw->iX;
  usPalette = pDraw->pPalette;
  y = pDraw->iY + pDraw->y; // current line
  if (y >= DISPLAY_HEIGHT || pDraw->iX >= DISPLAY_WIDTH || iWidth < 1)
    return;

  // Old image disposal
  s = pDraw->pPixels;
  if (pDraw->ucDisposalMethod == 2) // restore to background color
  {
    for (x = 0; x < iWidth; x++)
    {
      if (s[x] == pDraw->ucTransparent)
        s[x] = pDraw->ucBackground;
    }
    pDraw->ucHasTransparency = 0;
  }

  // Draw the same line of pixel data for both screens
  for (int screen = 0; screen < 2; screen++)
  {
    // Select current screen and set rotation angle
    if (screen == 0) {
      digitalWrite(TFT1_CS, LOW);
      digitalWrite(TFT2_CS, HIGH);
      tft.setRotation(3); // Screen 1 rotated 270 degrees
    } else {
      digitalWrite(TFT1_CS, HIGH);
      digitalWrite(TFT2_CS, LOW);
      tft.setRotation(1); // Screen 2 rotated 90 degrees
    }
    
    // Apply the new pixels to the main image
    if (pDraw->ucHasTransparency) // if transparency used
    {
      uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
      s = pDraw->pPixels; // Reset pointer position
      pEnd = s + iWidth;
      x = 0;
      iCount = 0; // count non-transparent pixels
      while (x < iWidth)
      {
        c = ucTransparent - 1;
        d = &usTemp[0][0];
        while (c != ucTransparent && s < pEnd && iCount < BUFFER_SIZE )
        {
          c = *s++;
          if (c == ucTransparent) // done, stop
          {
            s--; // back up to treat it like transparent
          }
          else // opaque
          {
            *d++ = usPalette[c];
            iCount++;
          }
        } // while looking for opaque pixels
        if (iCount) // any opaque pixels?
        {
          // DMA would degrade performance here due to short line segments
          tft.setAddrWindow(pDraw->iX + x, y, iCount, 1);
          tft.pushPixels(&usTemp[0][0], iCount);
          x += iCount;
          iCount = 0;
        }
        // no, look for a run of transparent pixels
        c = ucTransparent;
        while (c == ucTransparent && s < pEnd)
        {
          c = *s++;
          if (c == ucTransparent)
            x++;
          else
            s--;
        }
      }
    }
    else
    {
      s = pDraw->pPixels; // Reset pointer position

      // Unroll the first pass to boost DMA performance
      // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
      if (iWidth <= BUFFER_SIZE)
        for (iCount = 0; iCount < iWidth; iCount++) usTemp[dmaBuf][iCount] = usPalette[*s++];
      else
        for (iCount = 0; iCount < BUFFER_SIZE; iCount++) usTemp[dmaBuf][iCount] = usPalette[*s++];

#ifdef USE_DMA // 71.6 fps (ST7796 84.5 fps)
      tft.dmaWait();
      tft.setAddrWindow(pDraw->iX, y, iWidth, 1);
      tft.pushPixelsDMA(&usTemp[dmaBuf][0], iCount);
      dmaBuf = !dmaBuf;
#else // 57.0 fps
      tft.setAddrWindow(pDraw->iX, y, iWidth, 1);
      tft.pushPixels(&usTemp[0][0], iCount);
#endif

      int remainingWidth = iWidth - iCount;
      // Loop if pixel buffer smaller than width
      while (remainingWidth > 0)
      {
        // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
        if (remainingWidth <= BUFFER_SIZE)
          for (iCount = 0; iCount < remainingWidth; iCount++) usTemp[dmaBuf][iCount] = usPalette[*s++];
        else
          for (iCount = 0; iCount < BUFFER_SIZE; iCount++) usTemp[dmaBuf][iCount] = usPalette[*s++];

#ifdef USE_DMA
        tft.dmaWait();
        tft.pushPixelsDMA(&usTemp[dmaBuf][0], iCount);
        dmaBuf = !dmaBuf;
#else
        tft.pushPixels(&usTemp[0][0], iCount);
#endif
        remainingWidth -= iCount;
      }
    }
  }
  
  // Disable all screen chip selects after drawing completion
  digitalWrite(TFT1_CS, HIGH);
  digitalWrite(TFT2_CS, HIGH);
}

// GIF file operation callback functions
void *gifFileOpen(const char *filename, int32_t *pFileSize) {
  gifFile = SPIFFS.open("/current.gif", FILE_READ);
  *pFileSize = gifFile.size();
  if (!gifFile) {
    Serial.println("Failed to open GIF file from SPIFFS!");
  }
  return &gifFile;
}

void gifFileClose(void *pHandle) {
  gifFile.close();
}

int32_t gifFileRead(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
  int32_t iBytesRead = iLen;
  if ((pFile->iSize - pFile->iPos) < iLen)
    iBytesRead = pFile->iSize - pFile->iPos;
  if (iBytesRead <= 0) return 0;

  gifFile.seek(pFile->iPos);
  int32_t bytesRead = gifFile.read(pBuf, iLen);
  pFile->iPos += iBytesRead;
  return bytesRead;
}

int32_t gifFileSeek(GIFFILE *pFile, int32_t iPosition) {
  if (iPosition < 0) iPosition = 0;
  else if (iPosition >= pFile->iSize) iPosition = pFile->iSize - 1;
  pFile->iPos = iPosition;
  gifFile.seek(pFile->iPos);
  return iPosition;
}

// Universal SD card initialization function (called when all modes start)
bool initSDCard() {
  Serial.println("Starting SD card initialization...");
  
  // LED indication: Start initialization (fast flash)
  for (int i = 0; i < 6; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
  
  // Ensure SD card CS pin state is correct
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  delay(10);
  
  // Initialize SD card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card initialization failed!");
    // LED indication: Initialization failed (long light for 3 seconds)
    digitalWrite(LED_BUILTIN, HIGH);
    delay(3000);
    digitalWrite(LED_BUILTIN, LOW);
    return false;
  }
  
  Serial.println("SD card initialization successful");
  sdCardInitialized = true;
  
  // LED indication: SD card initialization successful (slow flash 3 times)
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(300);
    digitalWrite(LED_BUILTIN, LOW);
    delay(300);
  }
  
  return true;
}

// Universal SPIFFS initialization function
bool initSPIFFS() {
  Serial.println("Starting SPIFFS initialization...");
  
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS initialization failed!");
    return false;
  }
  
  Serial.println("SPIFFS initialization successful");
  spiffsInitialized = true;
  return true;
}

// Initialize GIF playback system (only called in mode c)
void initGifSystem() {
  Serial.println("Initializing GIF playback system...");
  systemReadyBlinking = true; // Start system ready blinking
  handleGifSwitchLED(); // Keep LED blinking during GIF system initialization
  showLoading(); // Show loading prompt
  
  // Check if SD card is inserted
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card not detected!");
    showErrorText(SD_ERROR_TEXT);
    return;
  }
  
  // Check if SD card is initialized
  if (!sdCardInitialized) {
    Serial.println("SD card not initialized, attempting to reinitialize...");
    if (!initSDCard()) {
      showErrorText(SD_ERROR_TEXT);  // Show SD card error message
      return;
    }
  }
  
  // Check if SPIFFS is initialized
  if (!spiffsInitialized) {
    if (!initSPIFFS()) {
      return;
    }
  }

  // Configure each screen for GIF mode
  Serial.println("Configuring displays for GIF mode...");
  handleGifSwitchLED(); // Keep LED blinking during initialization process
  
  // Configure rotation angles for screen 1 and screen 2
  digitalWrite(TFT2_CS, HIGH); // Ensure screen 2 is disabled
  digitalWrite(TFT1_CS, LOW);  // Enable screen 1
  tft.setRotation(3); // Screen 1: 270 degree rotation
  digitalWrite(TFT1_CS, HIGH); // Disable screen 1

  digitalWrite(TFT1_CS, HIGH); // Ensure screen 1 is disabled
  digitalWrite(TFT2_CS, LOW);  // Enable screen 2
  tft.setRotation(1); // Screen 2: 90 degree rotation
  digitalWrite(TFT2_CS, HIGH); // Disable screen 2
  
  handleGifSwitchLED(); // Keep LED blinking during initialization process
  showLoading(); // Re-show loading prompt

  handleGifSwitchLED(); // Keep LED blinking before counting GIF files
  totalGifCount = countGifFiles();
  Serial.println("Found " + String(totalGifCount) + " GIF files in SD card");
  handleGifSwitchLED(); // Keep LED blinking after counting GIF files
  
  if (totalGifCount == 0) {
    Serial.println("No GIF files found!");
    showErrorText(GIF_ERROR_TEXT);
    return;
  }

  SPIFFS.format();
  updateCurrentFilename();
  if (copyGifToSpiffs(currentFilename)) {
    Serial.println("Successfully loaded initial GIF: " + currentFilename);
  }

  gif.begin(BIG_ENDIAN_PIXELS);
  Serial.println("GIF playback system initialization completed");
  
  // LED indication: GIF system initialization completed (double flash 2 times)
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(150);
    digitalWrite(LED_BUILTIN, LOW);
    delay(150);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(150);
    digitalWrite(LED_BUILTIN, LOW);
    delay(300);
  }
}

// LED blinking control function
void handleGifSwitchLED() {
  if (gifSwitchInProgress || systemReadyBlinking) {
    unsigned long currentTime = millis();
    if (currentTime - lastGifLedToggle >= gifLedInterval) {
      gifLedState = !gifLedState;
      digitalWrite(LED_BUILTIN, gifLedState ? HIGH : LOW);
      lastGifLedToggle = currentTime;
    }
  }
}

// Manually control LED blinking during blocking operations
void manualLedBlink() {
  static unsigned long lastToggle = 0;
  static bool ledState = false;
  
  unsigned long currentTime = millis();
  if (currentTime - lastToggle >= gifLedInterval) {
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
    lastToggle = currentTime;
  }
}

// Handle GIF switching
void handleGifSwitch() {
  if (gifButtonPressed) {
    gifButtonPressed = false;
    
    // Stop system ready blinking, start GIF switching process
    Serial.println("GIF switch button detected, starting switch...");
    systemReadyBlinking = false;
    gifSwitchInProgress = true;
    gifPlayingSuccessfully = false;
    lastGifLedToggle = millis();
    gifLedState = true;
    digitalWrite(LED_BUILTIN, HIGH); // Immediately turn on LED to start blinking
    
    // Show Loading text
    showLoading();
    
    // Let LED blink once to show response
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
    digitalWrite(LED_BUILTIN, HIGH);
    
    currentGifIndex++;
    if (currentGifIndex > totalGifCount) {
      currentGifIndex = 1;
    }
    
    updateCurrentFilename();
    Serial.println("Switching to GIF: " + currentFilename);
    
    // Blink again to show processing
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
    digitalWrite(LED_BUILTIN, HIGH);
    
    // Format SPIFFS (during blinking process)
    Serial.println("Formatting SPIFFS...");
    
    // Let LED blink a few times before formatting to show user response
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(150);
      digitalWrite(LED_BUILTIN, LOW);
      delay(150);
    }
    
    // Ensure LED is on before formatting
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100); // Brief delay to ensure user sees LED is on
    
    // Start formatting task (this is a blocking operation)
    SPIFFS.format();
    
    // After formatting completion, restart normal blinking
    lastGifLedToggle = millis();
    gifLedState = true;
    digitalWrite(LED_BUILTIN, HIGH);
    
    Serial.println("SPIFFS formatting completed, starting GIF file copy...");
    
    // Use faster blinking frequency during file copy
    int originalInterval = gifLedInterval;
    gifLedInterval = 100; // Increase blinking frequency to 100ms
    
    // Copy GIF file
    if (copyGifToSpiffs(currentFilename)) {
      // Restore original blinking frequency
      gifLedInterval = originalInterval;
      Serial.println("GIF file copy successful, preparing to play...");
      // LED will stop blinking when new GIF starts playing
    } else {
      // Restore original blinking frequency
      gifLedInterval = originalInterval;
      Serial.println("GIF file read failed");
      gifSwitchInProgress = false;
      // Show GIF read failure message
      showErrorText(GIF_ERROR_TEXT);
      digitalWrite(LED_BUILTIN, LOW);
      
      // Rollback to previous GIF
      currentGifIndex--;
      if (currentGifIndex < 1) {
        currentGifIndex = totalGifCount;
      }
      updateCurrentFilename();
    }
  }
}

// Play GIF animation
void playGif() {
  // Check SD card status
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card not detected or damaged!");
    showErrorText(SD_ERROR_TEXT);
    return;
  }

  if (gif.open("/current.gif", gifFileOpen, gifFileClose, gifFileRead, gifFileSeek, GIFDraw)) {
    Serial.printf("Playing GIF: %s; Canvas size = %d x %d\n", 
                  currentFilename.c_str(), gif.getCanvasWidth(), gif.getCanvasHeight());
    
    // If GIF switching is in progress, now GIF starts playing, stop LED blinking
    if (gifSwitchInProgress) {
      Serial.println("New GIF starts playing, stop LED blinking");
      gifSwitchInProgress = false;
      gifPlayingSuccessfully = true;
      digitalWrite(LED_BUILTIN, LOW); // Turn off LED
    }
    
    // Clear screen before starting playback
    for (int screen = 0; screen < 2; screen++) {
      if (screen == 0) {
        digitalWrite(TFT1_CS, LOW);
        digitalWrite(TFT2_CS, HIGH);
        tft.setRotation(3);
      } else {
        digitalWrite(TFT1_CS, HIGH);
        digitalWrite(TFT2_CS, LOW);
        tft.setRotation(1);
      }
      tft.fillScreen(TFT_BLACK);
    }
    digitalWrite(TFT1_CS, HIGH);
    digitalWrite(TFT2_CS, HIGH);
    
    // Ensure all screen chip selects are disabled
    digitalWrite(TFT1_CS, HIGH);
    digitalWrite(TFT2_CS, HIGH);
    
    // Start SPI write session
    tft.startWrite();
    
    // Play GIF frames
    while (gif.playFrame(true, NULL) && !gifButtonPressed && CUR_MODE == 'c') {
      // Frequently check mode switching during GIF playback
      char currentMode;
      if (digitalRead(AUTO_PIN) == LOW) {
        currentMode = 'a';
      } else if (digitalRead(MANUAL_PIN) == LOW) {
        currentMode = 'm';
      } else {
        currentMode = 'c';
      }
      
      // If mode changes, immediately exit GIF playback
      if (currentMode != CUR_MODE) {
        Serial.println("Mode switching detected during GIF playback, exiting immediately");
        showInfoText(MODE_SWITCH_TEXT, INFO_TEXT_COLOR, INFO_TEXT_SIZE, 1000);
        break;
      }
      
      yield();
    }
    
    // Close GIF and SPI session
    gif.close();
    tft.endWrite();
    
    // Ensure all screen chip selects are disabled
    digitalWrite(TFT1_CS, HIGH);
    digitalWrite(TFT2_CS, HIGH);
    
    Serial.println("GIF playback finished");
  } else {
    Serial.println("Failed to open GIF file for playback");
    showErrorText(GIF_ERROR_TEXT);  // Show GIF read failure prompt
  }
}

// Show Loading text
void showLoading() {
  // Show Loading on both screens
  for (int screen = 0; screen < 2; screen++) {
    // Select current screen
    if (screen == 0) {
      digitalWrite(TFT1_CS, LOW);
      digitalWrite(TFT2_CS, HIGH);
      tft.setRotation(3); // Screen 1 rotated 270 degrees
    } else {
      digitalWrite(TFT1_CS, HIGH);
      digitalWrite(TFT2_CS, LOW);
      tft.setRotation(1); // Screen 2 rotated 90 degrees
    }
    
    // Clear screen and show Loading text
    tft.fillScreen(LOADING_BG_COLOR);
    tft.setTextColor(LOADING_TEXT_COLOR);
    tft.setTextSize(LOADING_TEXT_SIZE);
    
    // Calculate text position to center it
    // Each character is about 8 pixels wide (at text size 2)
    int charWidth = 8 * LOADING_TEXT_SIZE;
    int textWidth = strlen(LOADING_TEXT) * charWidth;
    int x = (tft.width() - textWidth) / 2;
    int y = (tft.height() - (16 * LOADING_TEXT_SIZE)) / 2;
    
    tft.setCursor(x, y);
    tft.print(LOADING_TEXT);
  }
  
  // Ensure all screen chip selects are disabled
  digitalWrite(TFT1_CS, HIGH);
  digitalWrite(TFT2_CS, HIGH);
}

// Show error text
void showErrorText(const char* errorText) {
  // Show error message on both screens
  for (int screen = 0; screen < 2; screen++) {
    // Select current screen
    if (screen == 0) {
      digitalWrite(TFT1_CS, LOW);
      digitalWrite(TFT2_CS, HIGH);
      tft.setRotation(3); // Screen 1 rotated 270 degrees
    } else {
      digitalWrite(TFT1_CS, HIGH);
      digitalWrite(TFT2_CS, LOW);
      tft.setRotation(1); // Screen 2 rotated 90 degrees
    }
    
    // Clear screen and show error text
    tft.fillScreen(LOADING_BG_COLOR);
    tft.setTextColor(ERROR_TEXT_COLOR);
    tft.setTextSize(ERROR_TEXT_SIZE);
    
    // Calculate text position to center it
    int charWidth = 8 * ERROR_TEXT_SIZE;
    int textWidth = strlen(errorText) * charWidth;
    int x = (tft.width() - textWidth) / 2;
    int y = (tft.height() - (16 * ERROR_TEXT_SIZE)) / 2;
    
    tft.setCursor(x, y);
    tft.print(errorText);
  }
  
  // Ensure all screen chip selects are disabled
  digitalWrite(TFT1_CS, HIGH);
  digitalWrite(TFT2_CS, HIGH);
  
  // Delay 2 seconds after error display
  delay(2000);
}

// Show info text
void showInfoText(const char* infoText, uint16_t textColor, uint8_t textSize, uint16_t delayTime) {
  // Show info on both screens
  for (int screen = 0; screen < 2; screen++) {
    // Select current screen
    if (screen == 0) {
      digitalWrite(TFT1_CS, LOW);
      digitalWrite(TFT2_CS, HIGH);
      tft.setRotation(3); // Screen 1 rotated 270 degrees
    } else {
      digitalWrite(TFT1_CS, HIGH);
      digitalWrite(TFT2_CS, LOW);
      tft.setRotation(1); // Screen 2 rotated 90 degrees
    }
    
    // Clear screen and show text
    tft.fillScreen(LOADING_BG_COLOR);
    tft.setTextColor(textColor);
    tft.setTextSize(textSize);
    
    // Calculate text position to center it
    int charWidth = 8 * textSize;
    int textWidth = strlen(infoText) * charWidth;
    int x = (tft.width() - textWidth) / 2;
    int y = (tft.height() - (16 * textSize)) / 2;
    
    tft.setCursor(x, y);
    tft.print(infoText);
  }
  
  // Ensure all screen chip selects are disabled
  digitalWrite(TFT1_CS, HIGH);
  digitalWrite(TFT2_CS, HIGH);
  
  // If delay time is specified, wait
  if (delayTime > 0) {
    delay(delayTime);
  }
}

// INITIALIZATION -- runs once at startup ----------------------------------
void setup(void) {
  Serial.begin(115200);
  //while (!Serial);
  Serial.println("ESP32 animated eyes system starting...");

  // Configure onboard LED pin (for status indication)
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  // Show system startup information
  showInfoText(SYSTEM_START_TEXT, INFO_TEXT_COLOR, INFO_TEXT_SIZE, 1000);
  
  // LED indication: System startup (single long light for 1 second)
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);

  // **Important: Initialize SD card first, regardless of mode**
  Serial.println("Initialize SD card first regardless of mode...");
  initSDCard();  // This will have LED status indication
  
  // Also initialize SPIFFS for mode c preparation
  initSPIFFS();

#if defined(DISPLAY_BACKLIGHT) && (DISPLAY_BACKLIGHT >= 0)
  // Enable backlight pin, initially off
  Serial.println("Backlight turned off");
  pinMode(DISPLAY_BACKLIGHT, OUTPUT);
  digitalWrite(DISPLAY_BACKLIGHT, LOW);
#endif

  // Configure input pins
  pinMode(JOYSTICK_X_PIN, INPUT);
  pinMode(JOYSTICK_Y_PIN, INPUT);
  pinMode(BLINK_PIN, INPUT_PULLUP);
  pinMode(AUTO_PIN, INPUT_PULLUP);
  pinMode(MANUAL_PIN, INPUT_PULLUP);
  pinMode(CHANGE_PIN, INPUT_PULLUP); 

  attachInterrupt(CHANGE_PIN, func1, FALLING);  // Set button falling edge trigger interrupt

  analogReadResolution(10);
  
  // User call for additional features
  user_setup();

  // Ensure SD card CS pin is in correct state before TFT initialization
  digitalWrite(SD_CS_PIN, HIGH);

  // Initialise the eye(s), this will set all chip selects low for the tft.init()
  initEyes();

  // Initialise TFT
  Serial.println("Initializing displays");
  tft.init();

#ifdef USE_DMA
  tft.initDMA();
#endif

  // Raise chip select(s) so that displays can be individually configured
  digitalWrite(eye[0].tft_cs, HIGH);
  if (NUM_EYES > 1) digitalWrite(eye[1].tft_cs, HIGH);

  // Ensure SD card CS pin remains HIGH during screen configuration
  digitalWrite(SD_CS_PIN, HIGH);

  for (uint8_t e = 0; e < NUM_EYES; e++) {
    digitalWrite(eye[e].tft_cs, LOW);
    tft.setRotation(eyeInfo[e].rotation);
    tft.fillScreen(TFT_BLACK);
    digitalWrite(eye[e].tft_cs, HIGH);
  }

  // Ensure SD card CS pin state is correct again
  digitalWrite(SD_CS_PIN, HIGH);

#if defined(DISPLAY_BACKLIGHT) && (DISPLAY_BACKLIGHT >= 0)
  Serial.println("Backlight turned on!");
  analogWrite(DISPLAY_BACKLIGHT, BACKLIGHT_MAX);
#endif

  startTime = millis(); // For frame-rate calculation
  
  Serial.println("System initialization completed, starting mode switch detection...");
  
  // LED indication: System initialization completed (three short flashes)
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
  
  // Start continuous blinking after system ready
  systemReadyBlinking = true;
  lastGifLedToggle = millis();
  gifLedState = false;
}

// LED indication function: Fast flash three times indicates receiving mode switch request
void ledIndicateModeSwitch() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
}

// LED indication function: Slow flash indicates mode switching in progress
void ledIndicateProcessing() {
  static unsigned long lastLedToggle = 0;
  static bool ledState = false;
  
  unsigned long currentTime = millis();
  if (currentTime - lastLedToggle >= 500) {  // Toggle every 500ms
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
    lastLedToggle = currentTime;
  }
}

// LED indication function: Stop slow flash, restore system ready blinking
void ledIndicateComplete() {
  digitalWrite(LED_BUILTIN, LOW);
  // If not in GIF switching process, restore system ready blinking state
  if (!gifSwitchInProgress) {
    systemReadyBlinking = true;
    lastGifLedToggle = millis();
    gifLedState = false;
  }
}

// Global variables: Mode switching status
bool modeChanging = false;
unsigned long modeChangeStartTime = 0;

// MAIN LOOP -- runs continuously after setup() ----------------------------
void loop() {
  // Detect current mode
  char newMode;
  if (digitalRead(AUTO_PIN) == LOW) {
    newMode = 'a';
  } else if (digitalRead(MANUAL_PIN) == LOW) {
    newMode = 'm';
  } else {
    newMode = 'c';
  }
  
  // If mode changes, output log and ensure SD card state is correct
  if (newMode != CUR_MODE) {
    Serial.print("Mode switch: ");
    Serial.print(CUR_MODE);
    Serial.print(" -> ");
    Serial.println(newMode);
    
    // If GIF switching or system ready blinking is in progress, pause LED blinking
    if (gifSwitchInProgress) {
      Serial.println("Mode switch interrupts GIF switching, stop LED blinking");
      gifSwitchInProgress = false;
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (systemReadyBlinking) {
      Serial.println("Mode switching, pause system ready blinking");
      systemReadyBlinking = false;
      digitalWrite(LED_BUILTIN, LOW);
    }
    
    // LED indication: Fast flash three times indicates receiving mode switch request
    ledIndicateModeSwitch();
    
    // Set mode switching status
    modeChanging = true;
    modeChangeStartTime = millis();
    
    // Ensure SD card CS pin maintains correct state during mode switching
    digitalWrite(SD_CS_PIN, HIGH);
    delay(10);
    
    CUR_MODE = newMode;
  }
  
  // If switching modes, show slow flash LED
  if (modeChanging) {
    ledIndicateProcessing();
    
    // Check if mode switching indication should stop
    // Stop LED indication after 2 seconds of display content
    if (millis() - modeChangeStartTime > 2000) {
      modeChanging = false;
      ledIndicateComplete();
    }
  }

  if (CUR_MODE == 'c') {
    // Mode c: GIF playback mode
    static bool gifSystemInitialized = false;
    if (!gifSystemInitialized) {
      // Ensure SD card state is correct before initializing GIF system
      digitalWrite(SD_CS_PIN, HIGH);
      delay(50);
      
      Serial.println("Switching to GIF playback mode, initializing GIF system...");
      initGifSystem();
      gifSystemInitialized = true;
      
      // After initialization completion, stop mode switching LED indication
      if (modeChanging) {
        modeChanging = false;
        ledIndicateComplete();
      }
    }
    
    // Check mode before processing GIF switching
    char currentMode;
    if (digitalRead(AUTO_PIN) == LOW) {
      currentMode = 'a';
    } else if (digitalRead(MANUAL_PIN) == LOW) {
      currentMode = 'm';
    } else {
      currentMode = 'c';
    }
    
    // Only continue processing GIF if mode is still c
    if (currentMode == CUR_MODE) {
      handleGifSwitch(); // Handle GIF switching
      handleGifSwitchLED(); // Handle GIF switching LED status
      playGif();         // Play current GIF
    }
  } else {
    // Mode a and m: Original eye tracking mode
    // Ensure SD card CS pin maintains HIGH state to avoid interference
    static unsigned long lastSDCheck = 0;
    if (millis() - lastSDCheck > 1000) { // Check once per second
      digitalWrite(SD_CS_PIN, HIGH);
      lastSDCheck = millis();
    }
    
    // Check mode switching before calling updateEye
    char currentMode;
    if (digitalRead(AUTO_PIN) == LOW) {
      currentMode = 'a';
    } else if (digitalRead(MANUAL_PIN) == LOW) {
      currentMode = 'm';
    } else {
      currentMode = 'c';
    }
    
    // Only continue executing updateEye if mode hasn't changed
    if (currentMode == CUR_MODE) {
      // If entering this mode for the first time, stop mode switching LED indication
      static bool eyeModeInitialized = false;
      if (!eyeModeInitialized && modeChanging) {
        modeChanging = false;
        ledIndicateComplete();
        eyeModeInitialized = true;
      }
      
      updateEye();
    } else {
      // If mode changed, reset initialization flag
      static bool eyeModeInitialized = false;
      eyeModeInitialized = false;
    }
    
    // Also handle LED status in non-GIF modes (including system ready blinking and GIF switching LED status)
    handleGifSwitchLED();
  }
}
