/*
 * mode_alexkidd.h
 *
 *  Created on: 22 déc. 2015
 *      Author: Bylos
 */

#ifndef MODE_ALEXKIDD_H_
#define MODE_ALEXKIDD_H_

#include "mcu_main.h"
#include "modes.h"
#include "drivers.h"

void Mode_AlexKidd_Init(void);
void Mode_AlexKidd_Run(cpu_state_t cpu_state);

#endif /* MODE_ALEXKIDD_H_ */
