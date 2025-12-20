#include "DVDisplay.h"

// Definicja mapy segmentów kółka (zgodnie z Twoimi pomiarami)
// {IndeksRAMu, WartośćBitu}
// Indeksy: C0=0, C1=1 ... CD=13
const DVDisplay::DiscMapping DVDisplay::_discMap[8] = {
  {3, 1},  // 0: 1/8 (C3)
  {11, 1}, // 1: 2/8 (CB)
  {13, 1}, // 2: 3/8 (CD bit 1)
  {7, 1},  // 3: 4/8 (C7)
  {5, 1},  // 4: 5/8 (C5)
  {9, 1},  // 5: 6/8 (C9)
  {13, 2}, // 6: 7/8 (CD bit 2 - Wyjątek!)
  {1, 1}   // 7: 8/8 (C1)
};

DVDisplay::DVDisplay(int dioPin, int clkPin, int stbPin) {
  _dio = dioPin;
  _clk = clkPin;
  _stb = stbPin;
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

// --- SEKCJE LICZBOWE ---

void DVDisplay::showTime(int h, int m, int s) {
  setValues(h, m, s);
}

void DVDisplay::setValues(int left, int center, int right) {
  // Bity cyfr: [2, 128], [4, 8], [16, 32]
  byte digitMask = 2 | 128 | 4 | 8 | 16 | 32;
  
  // Czyścimy tylko miejsca na cyfry (ikony zostają)
  for(int i=0; i<14; i+=2) {
     _displayRam[i] &= ~digitMask; 
  }

  // Lewa sekcja (Godziny)
  if (left >= 0) {
    mapDigitToRam((left / 10) % 10, 2);
    mapDigitToRam(left % 10, 128);
  }

  // Środkowa sekcja (Minuty)
  if (center >= 0) {
    mapDigitToRam((center / 10) % 10, 4);
    mapDigitToRam(center % 10, 8);
  }

  // Prawa sekcja (Sekundy)
  if (right >= 0) {
    mapDigitToRam((right / 10) % 10, 16);
    mapDigitToRam(right % 10, 32);
  }
}

// --- DWUKROPEK ---

void DVDisplay::setColon(bool state) {
  if (state) {
    _displayRam[2] |= 64; // C2
    _displayRam[4] |= 64; // C4
  } else {
    _displayRam[2] &= ~64;
    _displayRam[4] &= ~64;
  }
}

// --- IKONY ---

void DVDisplay::setIcon(DvdIcon icon, bool state) {
  int addr = -1; 
  int bitVal = 0;

  switch(icon) {
    case ICON_DVD:    addr=1; bitVal=2; break;
    case ICON_VCD:    addr=3; bitVal=2; break;
    case ICON_MP3:    addr=5; bitVal=2; break;
    case ICON_PBC:    addr=7; bitVal=2; break;
    case ICON_PLAY:   addr=9; bitVal=2; break;
    case ICON_STOP:   addr=11; bitVal=2; break;
    case ICON_REPEAT: addr=0; bitVal=64; break;
    case ICON_DTS:    addr=6; bitVal=64; break;
    case ICON_DOLBY:  addr=8; bitVal=64; break;
  }

  if (addr != -1) {
    if (state) _displayRam[addr] |= bitVal;
    else       _displayRam[addr] &= ~bitVal;
  }
}

// --- KÓŁKO (DISC) ---

void DVDisplay::setDiscSingle(int position) {
  setDiscRaw(0); // Wyczyść kółko
  if (position >= 0 && position <= 7) {
    _displayRam[_discMap[position].addrIndex] |= _discMap[position].bitVal;
  }
}

void DVDisplay::setDiscProgress(int count) {
  setDiscRaw(0); // Wyczyść
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
  // Najpierw czyścimy wszystkie segmenty kółka
  for (int i = 0; i < 8; i++) {
    _displayRam[_discMap[i].addrIndex] &= ~_discMap[i].bitVal;
  }
  
  // Teraz zapalamy wg maski
  for (int i = 0; i < 8; i++) {
    if ((mask >> i) & 0x01) {
      _displayRam[_discMap[i].addrIndex] |= _discMap[i].bitVal;
    }
  }
}

// --- POMOCNICZE ---

void DVDisplay::sendCmd(byte cmd) {
  digitalWrite(_stb, LOW);
  shiftOut(_dio, _clk, LSBFIRST, cmd);
  digitalWrite(_stb, HIGH);
}

void DVDisplay::mapDigitToRam(int number, byte bitMask) {
  byte segments = _digits[number % 10];
  if (segments & 0x01) _displayRam[0] |= bitMask;
  if (segments & 0x02) _displayRam[2] |= bitMask;
  if (segments & 0x04) _displayRam[4] |= bitMask;
  if (segments & 0x08) _displayRam[6] |= bitMask;
  if (segments & 0x10) _displayRam[8] |= bitMask;
  if (segments & 0x20) _displayRam[10]|= bitMask;
  if (segments & 0x40) _displayRam[12]|= bitMask;
}