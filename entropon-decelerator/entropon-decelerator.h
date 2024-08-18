#ifndef ENTROPON_DECELERATOR_H
#define ENTROPON_DECELERATOR_H

////////// Hardware configuration //////////
//Include the config file that matches your hardware setup. If needed, duplicate an existing one.

#include "configs/default.h"

////////////////////////////////////////////

void setup();
void loop();
void initInputs();
void checkInputs();
bool readBtn(byte btn);
void cycleSession();
void setClock(byte h, byte m, byte s);
void updateTime(bool force=false);
void initOutputs();
void getTimeNow();
void getTimeDiff(unsigned long a, unsigned long b);

#endif //ENTROPON_DECELERATOR_H