#ifndef DISPLAY_HT16K33_H
#define DISPLAY_HT16K33_H

void initDisplay();
//void editDisplay(byte which, byte h=0, byte m=0, byte s=0, bool colon=1);
void displayOuterTime(byte h, byte m, byte s, bool colon=0);
void displayInnerTime(byte h, byte m, byte s, bool colon=0);
void displayPowerLevel(byte p);
void displaySession(byte s);
void displayState(char a, char b, bool dotA=0, bool dotB=0);
void displaySecondsSaved(byte s);

#endif //DISPLAY_HT16K33