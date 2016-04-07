/*
 * mode_unarmed.h
 *
 *  Created on: 22 déc. 2015
 *      Author: Bylos
 */

#ifndef MODE_ARMED_H_
#define MODE_ARMED_H_

#include "mcu_main.h"
#include "modes.h"
#include "drivers.h"

void Mode_Armed_Init(unsigned char mode);
void Mode_Armed_Run(cpu_state_t cpu_state);

#endif /* MODE_UNARMED_H_ */
