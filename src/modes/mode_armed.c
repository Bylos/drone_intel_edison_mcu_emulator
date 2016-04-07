/*
 * mode_alexkidd.c
 *
 *  Created on: 22 déc. 2015
 *      Author: Bylos
 */

#include "modes.h"
#include "drivers.h"

/*******************************************************************************
 * ALEXKID MODE
 *******************************************************************************/
void Mode_Armed_Init(unsigned char mode) {
	printf(" ---> Entering Armed State %d\n", mode);
}

void Mode_Armed_Run(cpu_state_t cpu_state) {
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
		/* Feed motors */
		esc_pwm_t pwm = cpu_get_esc_pwm_data();
		pca_channel_0_3(
				pwm.front_left + PCA_ONE_SHOT_1526_HZ_OFFSET,
				pwm.front_right + PCA_ONE_SHOT_1526_HZ_OFFSET,
				pwm.rear_right + PCA_ONE_SHOT_1526_HZ_OFFSET,
				pwm.rear_left + PCA_ONE_SHOT_1526_HZ_OFFSET);
	}
}
