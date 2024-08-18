#ifndef DISPLAY_HT16K33_H
#define DISPLAY_HT16K33_H

void initDisplay();
void editDisplay(byte which, byte h=0, byte m=0, byte s=0, bool colon=1);

#endif //DISPLAY_HT16K33