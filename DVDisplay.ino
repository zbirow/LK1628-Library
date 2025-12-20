#include "DVDisplay.h"

DVDisplay dvd(23, 18, 5);

int timerM = 1, timerS = 30;

bool modeClock = true;
unsigned long lastSec = 0;
unsigned long lastAnim = 0;
int frame = 0;
bool colon = false;

int h = 12, m = 0, s = 0;

void setup() {
  dvd.begin();
  dvd.setDiscFull(true);
  dvd.setIcon(ICON_DVD, true);
  dvd.update();
  delay(1000);
  dvd.setDiscFull(false);
}
void loop() {
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
  if (now - lastAnim >= 100) {
    lastAnim = now;
    frame++;
    if (frame > 8) frame = 0;
    dvd.setDiscProgress(frame); 
    dvd.update();
  }
}