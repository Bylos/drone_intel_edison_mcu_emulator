/*
 * mode_unarmed.c
 *
 *  Created on: 22 déc. 2015
 *      Author: Bylos
 */

#include "modes.h"
#include "drivers.h"

/*******************************************************************************
 * UNARMED MODE
 *******************************************************************************/
void Mode_Unarmed_Init(void) {
	pca_channel_0_3(PCA_ONE_SHOT_1526_HZ_OFFSET, PCA_ONE_SHOT_1526_HZ_OFFSET, PCA_ONE_SHOT_1526_HZ_OFFSET, PCA_ONE_SHOT_1526_HZ_OFFSET);
	lsm_clear_flags();
	xbee_clear_flags();
	cpu_clear_flags();
	printf(" ---> Entering Unarmed State\n");
}

void Mode_Unarmed_Run(cpu_state_t cpu_state) {
	/* Check for new sensors values */
	if (lsm_get_inertial_flag()) {
		/* Send to CPU */
		if (cpu_state != CPU_DISCONNECTED) {
			cpu_send_inertial(lsm_get_inertial_data());
		}
	}
	/* Check for new joystick values */
	if (xbee_get_joystick_flag()) {
		/* Send to CPU */
		if (cpu_state != CPU_DISCONNECTED) {
			cpu_send_joystick(xbee_get_joystick_data());
		}
	}
	/* Check for new esc values */
	if (cpu_get_esc_pwm_flag()) {
		/* Do nothing but clear data */
		cpu_get_esc_pwm_data();
	}

}
