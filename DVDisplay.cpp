#include "DVDisplay.h"

// Disc ring segment map based on the measured display RAM layout.
// Format: {display RAM index, bit value}. RAM indexes: C0=0, C1=1 ... CD=13.
const DVDisplay::DiscMapping DVDisplay::_discMap[8] = {
  {3, 1},   // 0: 1/8 (C3)
  {11, 1},  // 1: 2/8 (CB)
  {13, 1},  // 2: 3/8 (CD bit 1)
  {7, 1},   // 3: 4/8 (C7)
  {5, 1},   // 4: 5/8 (C5)
  {9, 1},   // 5: 6/8 (C9)
  {13, 2},  // 6: 7/8 (CD bit 2)
  {1, 1}    // 7: 8/8 (C1)
};

DVDisplay::DVDisplay(int dioPin, int clkPin, int stbPin) {
  _dio = dioPin;
  _clk = clkPin;
  _stb = stbPin;
  setButtonMap(DVD_KEY(4, 1), DVD_KEY(3, 2), DVD_KEY(2, 2), DVD_KEY(4, 2), DVD_KEY(2, 1));
}

void DVDisplay::begin() {
  pinMode(_dio, OUTPUT);
  pinMode(_clk, OUTPUT);
  pinMode(_stb, OUTPUT);
  digitalWrite(_stb, HIGH);
  digitalWrite(_clk, HIGH);
  sendCmd(0x03);
  setBrightness(7);
  clear();
  update();
}

void DVDisplay::setBrightness(int level) {
  if (level > 7) level = 7;
  sendCmd(0x88 | level);
}

void DVDisplay::clear() {
  for (int i = 0; i < 14; i++) _displayRam[i] = 0x00;
}

void DVDisplay::update() {
  digitalWrite(_stb, LOW);
  shiftOut(_dio, _clk, LSBFIRST, 0x44);
  digitalWrite(_stb, HIGH);
  for (int i = 0; i < 14; i++) {
    digitalWrite(_stb, LOW);
    shiftOut(_dio, _clk, LSBFIRST, 0xC0 + i);
    shiftOut(_dio, _clk, LSBFIRST, _displayRam[i]);
    digitalWrite(_stb, HIGH);
  }
}

// --- NUMERIC SECTIONS ---

void DVDisplay::showTime(int h, int m, int s) {
  setValues(h, m, s);
}

void DVDisplay::setValues(int left, int center, int right) {
  byte digitMask = 2 | 128 | 4 | 8 | 16 | 32;

  // Clear only the digit bits and leave icons untouched.
  for (int i = 0; i < 14; i += 2) {
    _displayRam[i] &= ~digitMask;
  }

  if (left >= 0) {
    mapDigitToRam((left / 10) % 10, 2);
    mapDigitToRam(left % 10, 128);
  }

  if (center >= 0) {
    mapDigitToRam((center / 10) % 10, 4);
    mapDigitToRam(center % 10, 8);
  }

  if (right >= 0) {
    mapDigitToRam((right / 10) % 10, 16);
    mapDigitToRam(right % 10, 32);
  }
}

// --- COLON ---

void DVDisplay::setColon(bool state) {
  if (state) {
    _displayRam[2] |= 64; // C2
    _displayRam[4] |= 64; // C4
  } else {
    _displayRam[2] &= ~64;
    _displayRam[4] &= ~64;
  }
}

// --- ICONS ---

void DVDisplay::setIcon(DvdIcon icon, bool state) {
  int addr = -1;
  int bitVal = 0;

  switch (icon) {
    case ICON_DVD:    addr = 1;  bitVal = 2;  break;
    case ICON_VCD:    addr = 3;  bitVal = 2;  break;
    case ICON_MP3:    addr = 5;  bitVal = 2;  break;
    case ICON_PBC:    addr = 7;  bitVal = 2;  break;
    case ICON_PLAY:   addr = 9;  bitVal = 2;  break;
    case ICON_STOP:   addr = 11; bitVal = 2;  break;
    case ICON_REPEAT: addr = 0;  bitVal = 64; break;
    case ICON_DTS:    addr = 6;  bitVal = 64; break;
    case ICON_DOLBY:  addr = 8;  bitVal = 64; break;
  }

  if (addr != -1) {
    if (state) _displayRam[addr] |= bitVal;
    else       _displayRam[addr] &= ~bitVal;
  }
}

// --- DISC RING ---

void DVDisplay::setDiscSingle(int position) {
  setDiscRaw(0);
  if (position >= 0 && position <= 7) {
    _displayRam[_discMap[position].addrIndex] |= _discMap[position].bitVal;
  }
}

void DVDisplay::setDiscProgress(int count) {
  setDiscRaw(0);
  if (count > 8) count = 8;
  for (int i = 0; i < count; i++) {
    _displayRam[_discMap[i].addrIndex] |= _discMap[i].bitVal;
  }
}

void DVDisplay::setDiscFull(bool state) {
  if (state) setDiscProgress(8);
  else setDiscRaw(0);
}

void DVDisplay::setDiscRaw(byte mask) {
  for (int i = 0; i < 8; i++) {
    _displayRam[_discMap[i].addrIndex] &= ~_discMap[i].bitVal;
  }

  for (int i = 0; i < 8; i++) {
    if ((mask >> i) & 0x01) {
      _displayRam[_discMap[i].addrIndex] |= _discMap[i].bitVal;
    }
  }
}

// --- BUTTONS ---

void DVDisplay::readKeyBytes(byte keyData[5]) {
  if (!keyData) return;

  for (byte i = 0; i < 5; i++) keyData[i] = 0;

  pinMode(_dio, OUTPUT);
  digitalWrite(_stb, LOW);
  writeByte(0x42);

  pinMode(_dio, INPUT_PULLUP);
  delayMicroseconds(2);

  for (byte i = 0; i < 5; i++) {
    keyData[i] = readByte();
  }

  digitalWrite(_stb, HIGH);
  digitalWrite(_clk, LOW);
  pinMode(_dio, OUTPUT);
  digitalWrite(_dio, LOW);
}

uint32_t DVDisplay::readKeysRaw() {
  byte keyData[5];
  readKeyBytes(keyData);

  uint32_t keys = 0;
  for (byte pairIndex = 0; pairIndex < 5; pairIndex++) {
    byte base = pairIndex * 4;
    byte value = keyData[pairIndex];

    if (value & 0x01) keys |= (1UL << (base + 0)); // KS odd, K1
    if (value & 0x02) keys |= (1UL << (base + 1)); // KS odd, K2
    if (value & 0x08) keys |= (1UL << (base + 2)); // KS even, K1
    if (value & 0x10) keys |= (1UL << (base + 3)); // KS even, K2
  }

  return keys;
}

bool DVDisplay::isKeyPressed(byte keyIndex) {
  if (keyIndex > 19) return false;
  return (readKeysRaw() & (1UL << keyIndex)) != 0;
}

void DVDisplay::setButtonMap(byte button1, byte button2, byte button3, byte button4, byte button5) {
  _buttonMap[0] = button1;
  _buttonMap[1] = button2;
  _buttonMap[2] = button3;
  _buttonMap[3] = button4;
  _buttonMap[4] = button5;
}

byte DVDisplay::readButtons() {
  return buttonsFromRaw(readKeysRaw());
}

byte DVDisplay::buttonsFromRaw(uint32_t keys) {
  byte buttons = 0;

  for (byte i = 0; i < 5; i++) {
    if (_buttonMap[i] <= 19 && (keys & (1UL << _buttonMap[i]))) {
      buttons |= (1 << i);
    }
  }

  return buttons;
}

int DVDisplay::readButton() {
  byte buttons = readButtons();
  for (byte i = 0; i < 5; i++) {
    if (buttons & (1 << i)) return i + 1;
  }

  return 0;
}

bool DVDisplay::isButtonPressed(byte buttonIndex) {
  if (buttonIndex < 1 || buttonIndex > 5) return false;
  return (readButtons() & (1 << (buttonIndex - 1))) != 0;
}

bool DVDisplay::isButtonPressed(DvdButton button) {
  if (button < DVD_BUTTON_CENTER || button > DVD_BUTTON_RIGHT) return false;
  return (readButtons() & (1 << button)) != 0;
}

// --- LOW-LEVEL SERIAL HELPERS ---

void DVDisplay::sendCmd(byte cmd) {
  digitalWrite(_stb, LOW);
  shiftOut(_dio, _clk, LSBFIRST, cmd);
  digitalWrite(_stb, HIGH);
}

void DVDisplay::writeByte(byte data) {
  for (byte i = 0; i < 8; i++) {
    digitalWrite(_clk, LOW);
    digitalWrite(_dio, (data & 0x01) ? HIGH : LOW);
    delayMicroseconds(1);
    digitalWrite(_clk, HIGH);
    delayMicroseconds(1);
    data >>= 1;
  }
}

byte DVDisplay::readByte() {
  byte value = 0;

  for (byte i = 0; i < 8; i++) {
    digitalWrite(_clk, LOW);
    delayMicroseconds(1);
    if (digitalRead(_dio)) value |= (1 << i);
    digitalWrite(_clk, HIGH);
    delayMicroseconds(1);
  }

  return value;
}

void DVDisplay::mapDigitToRam(int number, byte bitMask) {
  byte segments = _digits[number % 10];
  if (segments & 0x01) _displayRam[0] |= bitMask;
  if (segments & 0x02) _displayRam[2] |= bitMask;
  if (segments & 0x04) _displayRam[4] |= bitMask;
  if (segments & 0x08) _displayRam[6] |= bitMask;
  if (segments & 0x10) _displayRam[8] |= bitMask;
  if (segments & 0x20) _displayRam[10] |= bitMask;
  if (segments & 0x40) _displayRam[12] |= bitMask;
}
