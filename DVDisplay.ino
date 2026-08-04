#include "DVDisplay.h"

DVDisplay dvd(23, 18, 5);

const unsigned long SERIAL_BAUD = 9600;
const byte REQUIRED_STABLE_READS = 1;
const unsigned long LONG_PRESS_MS = 1000;

int timerM = 1, timerS = 30;

bool modeClock = true;
unsigned long lastSec = 0;
unsigned long lastAnim = 0;
int frame = 0;
bool colon = false;

int h = 12, m = 0, s = 0;

unsigned long lastKeyScan = 0;
uint32_t candidateRawKeys = 0;
uint32_t lastRawKeys = 0;
byte lastButtons = 0;
byte stableReads = 0;
unsigned long buttonPressedAt[5] = {0, 0, 0, 0, 0};
bool buttonLongReported[5] = {false, false, false, false, false};
bool discAnimationPaused = false;

const char* buttonName(byte buttonIndex) {
  switch (buttonIndex) {
    case DVD_BUTTON_CENTER: return "CENTER";
    case DVD_BUTTON_UP: return "UP";
    case DVD_BUTTON_DOWN: return "DOWN";
    case DVD_BUTTON_LEFT: return "LEFT";
    case DVD_BUTTON_RIGHT: return "RIGHT";
  }

  return "?";
}

void printButtonLabel(byte buttonIndex) {
  Serial.print("Button ");
  Serial.print(buttonIndex + 1);
  Serial.print("(");
  Serial.print(buttonName(buttonIndex));
  Serial.print(")");
}

void printMatrixKey(byte ks, byte kLine) {
  Serial.print("KS");
  Serial.print(ks);
  Serial.print("/K");
  Serial.print(kLine);
}

void printKeyDebug(byte buttons, uint32_t rawKeys) {
  Serial.print("Buttons mask: 0b");
  for (int i = 4; i >= 0; i--) {
    Serial.print((buttons >> i) & 0x01);
  }

  Serial.print(" | pressed:");
  bool anyButton = false;
  for (byte i = 0; i < 5; i++) {
    if (buttons & (1 << i)) {
      Serial.print(" ");
      printButtonLabel(i);
      anyButton = true;
    }
  }
  if (!anyButton) Serial.print(" none");

  Serial.print(" | raw:");
  bool anyRaw = false;
  for (byte ks = 1; ks <= 10; ks++) {
    for (byte kLine = 1; kLine <= 2; kLine++) {
      byte keyIndex = DVD_KEY(ks, kLine);
      if (rawKeys & (1UL << keyIndex)) {
        Serial.print(" ");
        printMatrixKey(ks, kLine);
        anyRaw = true;
      }
    }
  }
  if (!anyRaw) Serial.print(" none");

  Serial.println();
}

void setDiscAnimationPaused(bool paused) {
  if (discAnimationPaused == paused) return;

  discAnimationPaused = paused;
  Serial.print("Disc animation: ");
  Serial.println(discAnimationPaused ? "PAUSED" : "RUNNING");
}

void handleButtonEvents(byte buttons, byte previousButtons, unsigned long now) {
  for (byte i = 0; i < 5; i++) {
    byte mask = (1 << i);
    bool isPressed = (buttons & mask) != 0;
    bool wasPressed = (previousButtons & mask) != 0;

    if (isPressed && !wasPressed) {
      buttonPressedAt[i] = now;
      buttonLongReported[i] = false;

      Serial.print("DOWN ");
      printButtonLabel(i);
      Serial.println();

      if (i == DVD_BUTTON_CENTER) {
        setDiscAnimationPaused(!discAnimationPaused);
      }
    }

    if (!isPressed && wasPressed) {
      unsigned long heldMs = now - buttonPressedAt[i];

      Serial.print("UP ");
      printButtonLabel(i);
      Serial.print(" held=");
      Serial.print(heldMs);
      Serial.print(" ms (");
      Serial.print(heldMs / 1000.0, 2);
      Serial.print(" s) ");
      Serial.println(heldMs >= LONG_PRESS_MS ? "LONG" : "SHORT");
    }
  }
}

void reportLongHolds(byte buttons, unsigned long now) {
  for (byte i = 0; i < 5; i++) {
    if ((buttons & (1 << i)) && !buttonLongReported[i]) {
      unsigned long heldMs = now - buttonPressedAt[i];
      if (heldMs >= LONG_PRESS_MS) {
        Serial.print("HOLD ");
        printButtonLabel(i);
        Serial.print(" >= ");
        Serial.print(LONG_PRESS_MS);
        Serial.println(" ms");
        buttonLongReported[i] = true;
      }
    }
  }
}

void scanButtons() {
  unsigned long now = millis();
  if (now - lastKeyScan < 25) return;
  lastKeyScan = now;

  uint32_t rawKeys = dvd.readKeysRaw();
  if (rawKeys != candidateRawKeys) {
    candidateRawKeys = rawKeys;
    stableReads = 0;
    return;
  }

  if (stableReads < REQUIRED_STABLE_READS) {
    stableReads++;
    return;
  }

  byte buttons = dvd.buttonsFromRaw(rawKeys);

  if (rawKeys != lastRawKeys || buttons != lastButtons) {
    printKeyDebug(buttons, rawKeys);
    handleButtonEvents(buttons, lastButtons, now);
    lastRawKeys = rawKeys;
    lastButtons = buttons;
  }

  reportLongHolds(buttons, now);
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(200);
  Serial.println();
  Serial.println("DVDisplay LK1628 button test");
  Serial.println("Open Serial Monitor at 9600 baud and press panel buttons.");
  Serial.println("Map: Button 1=CENTER, 2=UP, 3=DOWN, 4=LEFT, 5=RIGHT.");
  Serial.println("Release prints hold time. CENTER toggles disc animation pause.");

  dvd.begin();
  dvd.setDiscFull(true);
  dvd.setIcon(ICON_DVD, true);
  dvd.update();
  delay(1000);
  dvd.setDiscFull(false);
}
void loop() {
  scanButtons();

  unsigned long now = millis();
  if (now - lastSec >= 1000) {
    lastSec = now;
    colon = !colon;
    if (modeClock) {
      s++;
      if (s > 59) { s=0; m++; }
      if (m > 59) { m=0; h++; }
      if (h > 23) h=0;
      dvd.showTime(h, m, s);
      dvd.setIcon(ICON_PLAY, false);
      dvd.setIcon(ICON_DTS, true);
      dvd.setColon(colon);
    } else {
      if (timerS > 0 || timerM > 0) {
        timerS--;
        if (timerS < 0) { timerS = 59; timerM--; }
      }
      dvd.setValues(timerM, timerS, -1);
      dvd.setIcon(ICON_PLAY, true);
      dvd.setIcon(ICON_DTS, false);
      dvd.setColon(true);
    }
    if (s % 10 == 0 && s != 0) modeClock = !modeClock;
    dvd.update();
  }
  if (!discAnimationPaused && now - lastAnim >= 100) {
    lastAnim = now;
    frame++;
    if (frame > 8) frame = 0;
    dvd.setDiscProgress(frame); 
    dvd.update();
  }
}
