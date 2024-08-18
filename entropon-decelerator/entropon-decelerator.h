#ifndef ENTROPON_DECELERATOR_H
#define ENTROPON_DECELERATOR_H

////////// Hardware configuration //////////
//Include the config file that matches your hardware setup. If needed, duplicate an existing one.

#include "configs/default.h"

////////////////////////////////////////////

void setup();
void loop();
void startTime();
void updateTime();
void initOutputs();

#endif //ARDUINO_CLOCK_H