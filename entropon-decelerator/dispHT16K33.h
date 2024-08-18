#ifndef DISPLAY_HT16K33_H
#define DISPLAY_HT16K33_H

void initDisplay();
void editDisplay(byte which, bool colon, byte h=25, byte m=0, byte s=0);

#endif //DISPLAY_HT16K33