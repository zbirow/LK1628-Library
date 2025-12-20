# Linkage LK1628 DVD Display Library

Arduino library for the **Linkage LK1628** LED controller

## ✨ Features

*   **Plug & Play:** Pre-mapped for the LK1628 pinout. No manual segment mapping required.
*   **Clock Mode:** Simple `showTime(h, m, s)` function.
*   **Timer/Counter Mode:** Independent control of Left, Center, and Right digit sections.
*   **Full Icon Support:** Control specific icons like DVD, VCD, MP3, DTS, Dolby, Repeat, etc.
*   **Disc Animation:** Built-in functions for the spinning disc indicator (Progress bar, Single segment, or Full fill).
*   **Colon Control:** Manual toggle for the time separators.

## 🔌 Wiring (LK1628 to Arduino/ESP)

Look for the pin header on the LK1628 board. Connect it to your microcontroller as follows:

| LK1628 Pin | Function | Microcontroller Pin |
| :--- | :--- | :--- |
| **5V** | VCC | 5V (Stable power required) |
| **GND** | Ground | GND |
| **STB** | Strobe | Any Digital Pin (e.g., D5) |
| **CLK** | Clock | Any Digital Pin (e.g., D18) |
| **DIO** | Data | Any Digital Pin (e.g., D23) |

> **Warning:** The LK1628 is a 5V logic device. If using ESP32/ESP8266 (3.3V), it usually works directly, but a logic level shifter is recommended for stability.

## 📦 Installation

1.  Download this repository.
2.  Copy the `DVDDisplay` folder into your Arduino `libraries` directory.
3.  Restart Arduino IDE.
4.  Include the library in your sketch: `#include "DVDDisplay.h"`

## 🚀 Example Usage

```cpp
#include "DVDisplay.h"

// Initialize with pins: DIO, CLK, STB
DVDisplay dvd(23, 18, 5); 

void setup() {
  dvd.begin();
  dvd.setBrightness(7); // Set max brightness
  
  // Turn on some icons
  dvd.setIcon(ICON_DVD, true);
  dvd.setIcon(ICON_DOLBY, true);
}

void loop() {
  // Update time based on your logic
  dvd.showTime(19, 30, 00);
  
  // Animate the disc (loading effect)
  static int frame = 0;
  dvd.setDiscProgress(frame++);
  if(frame > 8) frame = 0;
  
  // Commit changes to display
  dvd.update(); 
  
  delay(200);
}
```

## 📚 API Reference

### Core Functions

*   `DVDisplay(dio, clk, stb)` - Constructor.
*   `void begin()` - Initializes the display.
*   `void setBrightness(0-7)` - Sets intensity.
*   `void clear()` - Clears the display buffer.
*   `void update()` - Sends the buffer to the chip (must be called to see changes).

### Numeric Display

*   `void showTime(h, m, s)` - Displays formatted time.
*   `void setValues(left, center, right)` - Displays raw numbers (0-99) in the three sections. Use `-1` to blank a section.

### Icons & Symbols

Use `setIcon(IconName, state)` to control segments.

| Icon Name | Description |
| :--- | :--- |
| `ICON_DVD` | DVD Logo |
| `ICON_VCD` | VCD Logo |
| `ICON_MP3` | MP3 Logo |
| `ICON_PLAY` | Play Symbol |
| `ICON_STOP` | Pause/Stop Symbol |
| `ICON_REPEAT` | Loop/Repeat Arrow |
| `ICON_DTS` | DTS Logo |
| `ICON_DOLBY` | Dolby Logo |
| `ICON_PBC` | PBC Logo |

*   `void setColon(bool state)` - Toggles the colon separators.

### Disc Animation (The "Pizza" Wheel)

*   `void setDiscSingle(0-7)` - Lights up one specific segment.
*   `void setDiscProgress(0-8)` - Fills the disc like a progress bar.
*   `void setDiscFull(bool)` - Lights up the entire disc.
*   `void setDiscRaw(byte mask)` - Manual bitmask control.

## 🛠 Technical Details (LK1628 Mapping)

**Digit Mapping (Data Bits):**
*   **Hours:** Bit 2 (Tens) & Bit 128 (Units)
*   **Minutes:** Bit 4 (Tens) & Bit 8 (Units)
*   **Seconds:** Bit 16 (Tens) & Bit 32 (Units)

**Icon Mapping (Addresses):**
*   Most icons are located on **Bit 2** (Odd addresses C1, C3...).
*   DTS, Dolby, and Repeat are on **Bit 64** (Even addresses C0, C6, C8).
*   Colons are multiplexed on **Bit 64** at addresses C2 and C4.
