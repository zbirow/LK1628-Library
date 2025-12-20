/*
  DVDDisplay.h - Zaawansowana biblioteka do modułu DVD (TM1628).
  Wersja 2.0 - Pełna kontrola segmentów i animacji.
*/

#ifndef DVDisplay_h
#define DVDisplay_h

#include "Arduino.h"

// Enum z nazwami ikon dla czytelności
enum DvdIcon {
  ICON_DVD,
  ICON_VCD,
  ICON_MP3,
  ICON_PBC,
  ICON_PLAY,
  ICON_STOP,    // Pauza/Stop
  ICON_REPEAT,  // Strzałka pętli
  ICON_DTS,
  ICON_DOLBY
};

class DVDisplay {
  public:
    DVDisplay(int dioPin, int clkPin, int stbPin);

    // --- PODSTAWY ---
    void begin();
    void setBrightness(int level); // 0 (min) - 7 (max)
    void clear();  // Czyści wszystko (RAM)
    void update(); // Wysyła dane do wyświetlacza (konieczne po zmianach!)

    // --- SEKCJE LICZBOWE (HH MM SS) ---
    // Wyświetla czas HH:MM:SS.
    void showTime(int h, int m, int s);
    
    // Wpisuje dowolne liczby w sekcje: [Lewa] [Srodek] [Prawa]
    // Przydatne do minutnika, daty, licznika. Zakres 0-99.
    // Użyj -1 aby wygasić daną sekcję.
    void setValues(int left, int center, int right);

    // --- DWUKROPEK ---
    void setColon(bool state);

    // --- IKONY ---
    void setIcon(DvdIcon icon, bool state);

    // --- KÓŁKO (DISC) ---
    // Ustawia jeden segment (0-7). Reszta gaśnie. -1 = zgaś wszystkie.
    void setDiscSingle(int position);

    // Ustawia pasek postępu (0-8). Np. 4 zapali połowę kółka.
    void setDiscProgress(int count);

    // Zapala całe kółko (true) lub gasi (false).
    void setDiscFull(bool state);

    // Zaawansowane: Ustawia dowolny wzór bitowy (np. 0b10101010).
    void setDiscRaw(byte mask);

  private:
    int _dio, _clk, _stb;
    byte _displayRam[14];
    
    // Struktura mapy segmentów kółka
    struct DiscMapping {
      byte addrIndex; // Indeks w tablicy RAM (0-13)
      byte bitVal;    // Wartość bitu
    };
    
    // Mapa segmentów kółka (0-7)
    static const DiscMapping _discMap[8];

    void sendCmd(byte cmd);
    void mapDigitToRam(int number, byte bitMask);
    const byte _digits[10] = {
      0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110,
      0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01101111
    };
};

#endif