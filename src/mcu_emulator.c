#include <time.h>
#include "mcu_main.h"
#include "drivers.h"
#include "modes.h"
#include "debug.h"


unsigned long time_ms(unsigned char reset) {
	static struct timespec time;
	static unsigned long reset_time;
	clock_gettime(CLOCK_MONOTONIC_RAW, &time);
	if (reset) {
		reset_time = time.tv_sec*1000 + time.tv_nsec*0.000001;
		return 0;
	}
	else {
		return time.tv_sec*1000 + time.tv_nsec*0.000001 - reset_time;
	}
}



/* State Machine Variables */
mcu_mode_t mcu_mode = MCU_MODE_BOOT;
mcu_mode_t mcu_next_mode = MCU_MODE_STANDBY;



/*******************************************************************************
 * Send Adverts to Controller
 *******************************************************************************/
#define ADVERT_PERIOD		500 // ms
unsigned long mcu_advert_timer;
unsigned char MCU_Advert(const unsigned long timer) {
	if (timer - mcu_advert_timer >= ADVERT_PERIOD) {
		xbee_advertize(mcu_mode);
		mcu_advert_timer = timer;
		return 1;
	}
	return 0;
}



/*******************************************************************************
 * Get Data from Sensors
 *******************************************************************************/
#define INERTIAL_PERIOD		10	// ms
unsigned long mcu_inertial_timer;
unsigned char MCU_Get_Inertial_Data(const unsigned long timer) {
	if (timer - mcu_inertial_timer >= INERTIAL_PERIOD) {
		lsm_read_inertial();
		mcu_inertial_timer = timer;
		return 1;
	}
	return 0;
}



/*******************************************************************************
 * Get Data from Controller
 *******************************************************************************/
#define CONTROLLER_TIMEOUT	2000 // ms
unsigned long mcu_controller_timer;
char MCU_Get_Controller_Data(const unsigned long timer) {
	if (xbee_read_frame()) {
		mcu_controller_timer = timer;
		return 1;
	}
	if (timer - mcu_controller_timer >= CONTROLLER_TIMEOUT) {
		mcu_controller_timer = timer;
		debug_print((float)timer*0.001, WARNING, "Controller RX Timeout");
		return -1;
	}
	return 0;
}



/*******************************************************************************
 * Get Data from Processor
 *******************************************************************************/
#define CPU_TIMEOUT		2000 // ms
unsigned long mcu_cpu_timer;
char MCU_Get_Processor_Data(const unsigned long timer) {
	if (cpu_read_frame()) {
		mcu_cpu_timer = timer;
		return 1;
	}
	if (timer - mcu_cpu_timer >= CPU_TIMEOUT) {
		mcu_cpu_timer = timer;
		debug_print((float)timer*0.001, WARNING, "CPU Communication Timeout");
		return -1;
	}
	return 0;
}

int main() {
	/* Execution time (ms) */
	unsigned long cur_time = time_ms(1);
	debug_print(0.0, EMPTY, "");
	debug_print(0.0, EMPTY, "");
	debug_print(0.0, EMPTY, "");
	debug_print(0.0, EMPTY, "");
	char buff[20];
	time_t now = time(NULL);
	strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));
	debug_print(0.0, EMPTY, buff);
	debug_print((float)cur_time*0.001, INFO, "MCU Application Starts");

	crc_init();
	lsm_init();
	pca_init();
	xbee_init();
	cpu_init();

	unsigned long old_time = cur_time;
	cur_time = time_ms(0);
	debug_print((float)(cur_time - old_time)*0.001, INFO, "MCU Application Initialized");

	/* CPU Mode Status */
	cpu_state_t cpu_state = CPU_DISCONNECTED;

	/* Debug: log number of frames from peripherals */
	unsigned int cpu_cnt, xbee_cnt, lsm_cnt, adv_cnt;
	cpu_cnt = xbee_cnt = lsm_cnt = adv_cnt = 0;
	unsigned long debug_timer = 0;
	/* End of debug */

	while(1) {
		/* wait some time */
		nanosleep((const struct timespec[]){{0, 000001000L}}, NULL);
		/* Update execution time */
		cur_time = time_ms(0);
		if ((cur_time - old_time) > 2000) {
			debug_print((float)cur_time * 0.001, ERROR, "Timer has jumped more than 2s");
		}
		old_time = cur_time;

		/* Debug: log number of frame from peripherals */
		if (cur_time - debug_timer >= 10000) {
			/*printf("time: %d, cpu: %d, xbee: %d, lsm: %d, adv: %d\n",
					(int)cur_time/1000, cpu_cnt, xbee_cnt, lsm_cnt, adv_cnt);*/
			cpu_cnt = xbee_cnt = lsm_cnt = adv_cnt = 0;
			debug_timer = cur_time;
		}

		/* Check for new sensors values */
		if (MCU_Get_Inertial_Data(cur_time)) {
			lsm_cnt ++;
		}

		/* Check for cpu advertizing */
		switch (MCU_Get_Processor_Data(cur_time)) {
		case -1:
			cpu_state = CPU_DISCONNECTED;
			if (mcu_mode != MCU_MODE_BOOT && mcu_mode > MCU_MODE_ALEXKIDD) {
				debug_print((float)cur_time*0.001, MODE, "Require Standby Mode due to CPU Timeout");
				mcu_next_mode = MCU_MODE_STANDBY;
			}
			break;
		case 1:
			cpu_cnt ++;
			cpu_state = CPU_CONNECTED;
			if (cpu_get_mode_flag()) {
				mcu_next_mode = cpu_get_mode_data();
				if (mcu_next_mode != mcu_mode) {
					debug_print((float)cur_time*0.001, MODE, "Require New CPU Mode");
				}
			}
			break;
		}

		/* Check for new controller frame */
		switch (MCU_Get_Controller_Data(cur_time)) {
		case -1:
			if (cpu_state == CPU_CONNECTED) {
				cpu_send_command(MCU_MODE_STANDBY);
			}
			debug_print((float)cur_time*0.001, MODE, "Require Standby Mode due to Controller Timeout");
			mcu_next_mode = MCU_MODE_STANDBY;
			break;
		case 1:
			xbee_cnt ++;
			if (xbee_get_command_flag()) {
				rc_command_t command = xbee_get_command();
				if (cpu_state == CPU_CONNECTED) {
					debug_print((float)cur_time*0.001, MODE, "Transmit Controller request to CPU");
					cpu_send_command(command);
				}
				else if (command == RC_COMMAND_UNARM || command == RC_COMMAND_ALEXKIDD) {
					debug_print((float)cur_time*0.001, MODE, "Require a not connected Mode");
					mcu_next_mode = command;
				}
				else {
					debug_print((float)cur_time*0.001, MODE, "Request of Armed Mode lost due to CPU unavailable");
				}
			}
			break;
		}

		/* Change and init mode if needed */
		if (mcu_mode != mcu_next_mode) {
			mcu_mode = mcu_next_mode;
			switch (mcu_mode) {
			case MCU_MODE_STANDBY:
				debug_print((float)cur_time*0.001, MODE, "Init Standby Mode");
				Mode_Unarmed_Init();
				break;
			case MCU_MODE_ALEXKIDD:
				debug_print((float)cur_time*0.001, MODE, "Init AlexKidd Mode");
				Mode_AlexKidd_Init();
				break;
			case MCU_MODE_ACCEL_CAL:
			case MCU_MODE_GYRO_CAL:
			case MCU_MODE_MAGN_CAL:
			case MCU_MODE_ALEXKIDD2:
			case MCU_MODE_ACRO:
			case MCU_MODE_STABILIZED:
				debug_print((float)cur_time*0.001, MODE, "Init Armed Mode");
				Mode_Armed_Init(mcu_mode);
				break;
			default:
				break;
			}
			xbee_advertize(mcu_mode);
		}

		/* Execute mode process */
		switch (mcu_mode) {
		case MCU_MODE_STANDBY:
			Mode_Unarmed_Run(cpu_state);
			break;
		case MCU_MODE_ALEXKIDD:
			Mode_AlexKidd_Run(cpu_state);
			break;
		case MCU_MODE_ACCEL_CAL:
		case MCU_MODE_GYRO_CAL:
		case MCU_MODE_MAGN_CAL:
		case MCU_MODE_ALEXKIDD2:
		case MCU_MODE_ACRO:
		case MCU_MODE_STABILIZED:
			Mode_Armed_Run(cpu_state);
			break;
		default :
			break;
		}

		/* Send state to controller */
		if (MCU_Advert(cur_time)) {
			adv_cnt ++;
		}
	}

	return 0;
}
