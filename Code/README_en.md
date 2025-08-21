# ESP32 Animated Eyes Project

## Project Overview
This is an ESP32-based animated eyes project that supports three working modes:
- **Mode a (Auto Mode)**: Eyes move automatically and blink randomly
- **Mode m (Manual Mode)**: Manual eye movement control via joystick
- **Mode c (GIF Playback Mode)**: Play GIF animations from SD card

## Project Structure
```
ESP32_Animated_Eyes/
├── ESP32_Animated_Eyes.ino    # Main program file
├── config.h                   # Hardware configuration and pin definitions
├── eye_functions.ino          # Eye drawing and animation functions
├── user_gif.cpp              # GIF mode user functions
├── wiring.ino                # Hardware connection information
├── custom.h                  # Custom eye image data
├── README.md                 # Chinese project documentation
├── README_en.md              # English project documentation
└── README_cn.md              # Chinese detailed user guide
```

## Hardware Configuration
- **Main Controller**: ESP32 development board
- **Displays**: 2x 240x240 TFT screens (GC9A01)
- **Storage**: SD card (for GIF file storage)
- **Controls**: 
  - Three-position mode switch
  - Joystick control module
  - Button interrupt

### Pin Definitions
```cpp
// Display Control
#define TFT1_CS 22         // Screen 1 chip select
#define TFT2_CS 21         // Screen 2 chip select

// Mode Switching
#define AUTO_PIN 26        // Auto mode
#define MANUAL_PIN 27      // Manual mode
#define CHANGE_PIN 25      // Color change/GIF switch button

// Joystick Control
#define JOYSTICK_X_PIN 33  // Joystick X-axis
#define JOYSTICK_Y_PIN 32  // Joystick Y-axis
#define BLINK_PIN 13       // Joystick button

// Storage Module
#define SD_CS_PIN 12       // SD card chip select
```

## Latest Improvements: Mode Switching Response Speed Optimization

### Problem Description
- When switching from mode m to other modes, the switching process was very slow
- Mode m kept executing and couldn't be interrupted immediately
- Visual feedback was needed for mode switching

### Solution
1. **Multi-level Mode Detection**: Added mode detection at multiple key function locations
2. **Fast Response Mechanism**: Immediately interrupt current operations when mode switching is detected
3. **LED Indicator System**: Complete LED feedback mechanism during mode switching
4. **Serial Output Optimization**: Reduced serial output frequency in mode m

### Key Improvements
1. **New Functions**:
   - `quickModeCheck()`: Fast mode detection function
   - `ledIndicateModeSwitch()`: Mode switching LED indication (fast flash 3 times)
   - `ledIndicateProcessing()`: Processing LED indication (slow flash)
   - `ledIndicateComplete()`: Completion LED indication (turn off)

2. **Multi-point Mode Detection**:
   - Detection at the beginning of `frame()` function
   - Detection during pixel buffer refresh
   - Frequent detection in main loop
   - Detection during GIF playback
   - Detection during auto mode movement

3. **Response Speed Optimization**:
   - Reduced serial output from every frame to every 16 frames
   - Reduced FPS display from every 256 frames to every 1024 frames
   - Immediate interruption of current rendering when mode switching

4. **GIF Switching LED Indication Optimization**:
   - Minimal LED indication logic: button press → start flashing → new GIF plays → stop flashing
   - Failure indication: constant light for 2 seconds then turn off, avoiding customer misunderstanding of code freezing
   - Unified LED control logic, LED continues flashing until image plays correctly
   - Non-blocking LED control to ensure smooth process

## Previous Fix: SD Card Initialization Issue

### Problem Description
- When program starts in mode m, switching to mode c caused SD card initialization failure
- If program starts directly in mode c, SD card works normally
- Once SD card fails after switching from mode m to mode c, subsequent starts in any mode would fail

### Solution
1. **Unified Initialization Process**: Initialize SD card at the beginning of `setup()` function regardless of mode
2. **LED Status Indication**: Use onboard LED flash patterns to indicate SD card initialization status
3. **SPI Status Management**: Ensure SD card CS pin maintains correct state during mode switching and screen operations

### LED Status Indication
**System Startup Related**:
- **System Startup**: Long light for 1 second
- **SD Card Initialization Start**: Fast flash 6 times
- **SD Card Initialization Success**: Slow flash 3 times
- **SD Card Initialization Failure**: Long light for 3 seconds
- **System Initialization Complete**: Short flash 3 times

**Mode Switching Related**:
- **Mode Switch Request**: Fast flash 3 times (receiving switch request)
- **Mode Switch Processing**: Slow flash (500ms interval, until mode completely switches)
- **Mode Switch Complete**: LED turns off

**GIF Playback Related**:
- **GIF System Initialization Complete**: Double flash 2 times
- **GIF Switch Button Pressed**: Start flashing (300ms interval)
- **New GIF Starts Playing**: Stop flashing, LED turns off
- **GIF Switch Failure**: Constant light for 2 seconds then turn off

### Key Modifications
1. **New Functions**:
   - `InitSDCard()`: Universal SD card initialization function
   - `InitSPIFFS()`: Universal SPIFFS initialization function

2. **Modified Process**:
   - Call SD card initialization at the very beginning of `setup()`
   - Ensure SD card CS pin state is correct during mode switching
   - Regularly check SD card CS pin state during mode a/m operation

3. **Status Management**:
   - Added `sdCardInitialized` and `spiffsInitialized` flags
   - Avoid re-initializing already successful components

## Working Modes Detailed

### Mode a (Auto Mode)
- Eyes move automatically and blink randomly
- Pupil size changes automatically
- Supports button switching of eye colors (6 colors)

### Mode m (Manual Mode)
- Control eye gaze direction through joystick
- Joystick button controls blinking
- Supports button switching of eye colors

### Mode c (GIF Playback Mode)
- Play GIF animation files from SD card
- Supports button switching of GIF files (1.gif, 2.gif, ...)
- Dual screen synchronous display with automatic screen rotation adaptation

## Usage Instructions

### Hardware Setup
1. **Connect the displays**:
   - Connect TFT1_CS (pin 22) to the first screen's CS pin
   - Connect TFT2_CS (pin 21) to the second screen's CS pin
   - Connect MOSI (pin 23), SCLK (pin 18), DC (pin 2), RST (pin 4) to both screens
   - Connect VCC and GND to both screens

2. **Connect the joystick**:
   - Connect JOYSTICK_X_PIN (pin 33) to joystick X-axis
   - Connect JOYSTICK_Y_PIN (pin 32) to joystick Y-axis
   - Connect BLINK_PIN (pin 13) to joystick button
   - Connect VCC and GND to joystick

3. **Connect the mode switch**:
   - Connect AUTO_PIN (pin 26) to switch position 1
   - Connect MANUAL_PIN (pin 27) to switch position 2
   - Connect CHANGE_PIN (pin 25) to switch position 3

4. **Connect the SD card module**:
   - Connect SD_CS_PIN (pin 12) to SD card module CS pin
   - Connect MOSI, MISO, SCLK to SD card module
   - Connect VCC and GND to SD card module

### Software Setup
1. **Install required libraries**:
   - TFT_eSPI library
   - AnimatedGIF library
   - SD library (included with ESP32)

2. **Configure TFT_eSPI library**:
   - Edit TFT_eSPI library's User_Setup.h file
   - Enable GC9A01 display driver
   - Set correct pin definitions

3. **Prepare GIF files**:
   - Format SD card as FAT32
   - Copy GIF files to SD card root directory
   - Name files as 1.gif, 2.gif, 3.gif, etc.
   - Ensure GIF files are compatible with AnimatedGIF library

### Operation Instructions
1. **Power on the system**:
   - Connect ESP32 to power supply
   - Observe LED indicator for system startup status
   - Wait for initialization to complete

2. **Select working mode**:
   - **Position 1**: Auto mode (eyes move automatically)
   - **Position 2**: Manual mode (joystick control)
   - **Position 3**: GIF playback mode

3. **Mode-specific operations**:
   - **Auto Mode**: Eyes will move and blink automatically, press button to change eye color
   - **Manual Mode**: Use joystick to control eye direction, press joystick button to blink, press button to change eye color
   - **GIF Mode**: Press button to switch to next GIF file, observe LED for switching status

4. **Troubleshooting**:
   - If LED stays on for 3 seconds during startup, SD card initialization failed
   - If GIF doesn't play, check file format and compatibility
   - If mode switching is slow, wait for current operation to complete

## Technical Features
- **DMA Acceleration**: Supports DMA transfer for improved display performance
- **Dual Screen Synchronization**: Two screens display same content but with different rotation angles
- **Interrupt Handling**: Button response using interrupt method
- **Status Indication**: LED light indicates system running status
- **Error Recovery**: Automatic detection and recovery of SD card status
- **Memory Management**: Efficient buffer management for smooth animation
- **Power Management**: Optimized power consumption for battery operation

## Development Environment
- Arduino IDE / PlatformIO
- ESP32 development framework
- Required libraries: TFT_eSPI, AnimatedGIF

## File Descriptions

### Main Files
- **ESP32_Animated_Eyes.ino**: Main program file containing setup(), loop(), and core logic
- **config.h**: Hardware configuration, pin definitions, and mode settings
- **eye_functions.ino**: Eye rendering, animation, and drawing functions
- **user_gif.cpp**: User functions for GIF mode operation
- **wiring.ino**: Hardware connection diagrams and pin assignments
- **custom.h**: Custom eye image data and color definitions

### Key Functions
- **setup()**: System initialization, hardware setup, and mode detection
- **loop()**: Main program loop with mode-specific operations
- **updateEye()**: Eye movement and animation update
- **drawEye()**: Eye rendering and display function
- **playGif()**: GIF playback and management
- **quickModeCheck()**: Fast mode detection for responsive switching

## Performance Specifications
- **Display Resolution**: 240x240 pixels per eye
- **Refresh Rate**: Up to 60 FPS (depending on animation complexity)
- **Memory Usage**: Optimized buffer management for smooth operation
- **Power Consumption**: Low power design for extended battery life
- **Response Time**: <100ms mode switching response

## Troubleshooting Guide

### Common Issues
1. **SD Card Not Detected**:
   - Check SD card format (must be FAT32)
   - Verify wiring connections
   - Check SD card compatibility

2. **Display Not Working**:
   - Verify TFT_eSPI library configuration
   - Check display wiring connections
   - Ensure correct pin definitions in config.h

3. **Mode Switching Issues**:
   - Check mode switch wiring
   - Verify pin definitions
   - Wait for current operation to complete

4. **GIF Playback Problems**:
   - Ensure GIF files are compatible with AnimatedGIF library
   - Check file naming convention (1.gif, 2.gif, etc.)
   - Verify SD card is properly initialized

### Error Codes and Meanings
- **LED Fast Flash 6 times**: SD card initialization starting
- **LED Slow Flash 3 times**: SD card initialization successful
- **LED Long Light 3 seconds**: SD card initialization failed
- **LED Fast Flash 3 times**: Mode switching request received
- **LED Slow Flash**: Mode switching in progress
- **LED Constant Light 2 seconds**: GIF switch failed

## Future Enhancements
- **WiFi Connectivity**: Remote control via web interface
- **Bluetooth Control**: Mobile app control
- **Voice Commands**: Voice-activated eye movements
- **Facial Recognition**: Eye tracking based on detected faces
- **Custom Animations**: User-defined eye movement patterns
- **Cloud Storage**: GIF file management via cloud services

---
Last Updated: 2024 (Mode Switching Response Speed Optimization Version + LED Indicator System Enhancement + GIF Switching LED Minimal Indication Optimization) 