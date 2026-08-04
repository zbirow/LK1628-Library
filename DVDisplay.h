/*
  DVDisplay.h - Arduino library for DVD front panel displays driven by LK1628/TM1628.
  Version 2.0 - segment control, icons, disc animation, and panel button reading.
*/

#ifndef DVDisplay_h
#define DVDisplay_h

#include "Arduino.h"

// Icon names used by setIcon().
enum DvdIcon {
  ICON_DVD,
  ICON_VCD,
  ICON_MP3,
  ICON_PBC,
  ICON_PLAY,
  ICON_STOP,
  ICON_REPEAT,
  ICON_DTS,
  ICON_DOLBY
};

#define DVD_KEY(ks, kLine) ((byte)((((ks) - 1) * 2) + ((kLine) - 1)))
#define DVD_KEY_NONE 0xFF

// Five-button front panel layout.
enum DvdButton {
  DVD_BUTTON_CENTER = 0,
  DVD_BUTTON_UP,
  DVD_BUTTON_DOWN,
  DVD_BUTTON_LEFT,
  DVD_BUTTON_RIGHT,
  DVD_BUTTON_1 = DVD_BUTTON_CENTER,
  DVD_BUTTON_2 = DVD_BUTTON_UP,
  DVD_BUTTON_3 = DVD_BUTTON_DOWN,
  DVD_BUTTON_4 = DVD_BUTTON_LEFT,
  DVD_BUTTON_5 = DVD_BUTTON_RIGHT
};

class DVDisplay {
  public:
    DVDisplay(int dioPin, int clkPin, int stbPin);

    // --- BASIC CONTROL ---
    void begin();
    void setBrightness(int level); // 0 = minimum, 7 = maximum
    void clear();
    void update();

    // --- NUMERIC SECTIONS (HH MM SS) ---
    void showTime(int h, int m, int s);

    // Writes two-digit values into the left, center, and right numeric sections.
    // Use -1 to blank a section.
    void setValues(int left, int center, int right);

    // --- COLON ---
    void setColon(bool state);

    // --- ICONS ---
    void setIcon(DvdIcon icon, bool state);

    // --- DISC RING ---
    void setDiscSingle(int position);
    void setDiscProgress(int count);
    void setDiscFull(bool state);
    void setDiscRaw(byte mask);

    // --- BUTTONS ---
    // LK1628 scans a 10 x 2 matrix: KS1..KS10 and K1..K2.
    // Use DVD_KEY(ks, kLine), for example DVD_KEY(1, 1), to address one matrix key.
    void readKeyBytes(byte keyData[5]);
    uint32_t readKeysRaw();
    bool isKeyPressed(byte keyIndex);

    // Friendly API for the measured five-button DVD panel.
    // Default mapping: CENTER=KS4/K1, UP=KS3/K2, DOWN=KS2/K2,
    // LEFT=KS4/K2, RIGHT=KS2/K1.
    void setButtonMap(byte button1, byte button2, byte button3, byte button4, byte button5);
    byte buttonsFromRaw(uint32_t rawKeys);
    byte readButtons();          // bit0..bit4 map to buttons 1..5
    int readButton();            // returns 1..5 for the first pressed button, or 0 if none
    bool isButtonPressed(byte buttonIndex); // buttonIndex: 1..5
    bool isButtonPressed(DvdButton button);

  private:
    int _dio, _clk, _stb;
    byte _displayRam[14];
    byte _buttonMap[5];

    struct DiscMapping {
      byte addrIndex; // Display RAM index, 0..13
      byte bitVal;
    };

    static const DiscMapping _discMap[8];

    void sendCmd(byte cmd);
    void writeByte(byte data);
    byte readByte();
    void mapDigitToRam(int number, byte bitMask);
    const byte _digits[10] = {
      0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110,
      0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01101111
    };
};

#endif
