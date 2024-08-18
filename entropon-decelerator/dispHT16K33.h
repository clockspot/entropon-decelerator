#ifndef DISPLAY_HT16K33_H
#define DISPLAY_HT16K33_H

void initDisplay();
void editDisplay(byte which, byte h, byte m, byte s, bool colon=false);

#endif //DISPLAY_HT16K33