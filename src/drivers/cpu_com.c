/*
 * cpu_com.c
 *
 *  Created on: 22 déc. 2015
 *      Author: Bylos
 */

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "drivers.h"

const char *mcu2cpu_file = "/tmp/mcu2cpu";
const char *cpu2mcu_file = "/tmp/cpu2mcu";

static int mcu2cpu_port, cpu2mcu_port;

void cpu_send_joystick(joystick_t joystick) {
	unsigned char buffer[5+sizeof(joystick_t)];
	int buffer_length = 5+sizeof(joystick_t);	// 2 bytes preamble, 1 byte length, 1 byte type, 1 byte crc, joystick struct
	unsigned char *joystick_ptr = (unsigned char*)&joystick;
	buffer[0] = buffer[1] = 0xFF;
	buffer[2] = 1+sizeof(joystick_t);
	buffer[3] = MCU_FRAME_JOYSTICK;
	int i;
	for (i = 0; i < sizeof(joystick_t); i++) {
		buffer[4+i] = joystick_ptr[i];
	}
	buffer[4+sizeof(joystick_t)] = crc_fast(buffer+3, 1+sizeof(joystick_t));
	write(mcu2cpu_port, buffer, buffer_length);
}

void cpu_send_inertial(inertial_t inertial) {
	unsigned char buffer[5+sizeof(inertial_t)];
	int buffer_length = 5+sizeof(inertial_t);
	unsigned char *inertial_ptr = (unsigned char*)&inertial;
	buffer[0] = buffer[1] = 0xFF;
	buffer[2] = 1+sizeof(inertial_t); // length of payload without crc
	buffer[3] = MCU_FRAME_INERTIAL;
	int i;
	for (i = 0; i < sizeof(inertial_t); i++) {
		buffer[4+i] = inertial_ptr[i];
	}
	buffer[4+sizeof(inertial_t)] = crc_fast(buffer+3, 1+sizeof(inertial_t));
	write(mcu2cpu_port, buffer, buffer_length);
}

void cpu_send_command(cpu_cmd_t command) {
	unsigned char buffer[6] = {0xFF, 0xFF, 2, MCU_FRAME_COMMAND, (unsigned char) command, 0x00};
	buffer[5] = crc_fast(buffer+3, 2);
	write(mcu2cpu_port, buffer, 6);
}

void cpu_send_event(cpu_evt_t event) {
	unsigned char buffer[6] = {0xFF, 0xFF, 2, MCU_FRAME_EVENT, (unsigned char) event, 0x00};
	buffer[5] = crc_fast(buffer+3, 2);
	write(mcu2cpu_port, buffer, 6);
}

unsigned char cpu_mode_flag = 0;
unsigned char cpu_mode = 0;

unsigned char cpu_get_mode_flag(void) {
	return cpu_mode_flag;
}

unsigned char cpu_get_mode_data(void) {
	cpu_mode_flag = 0;
	return cpu_mode;
}

unsigned char cpu_esc_pwm_flag = 0;
esc_pwm_t cpu_esc_pwm;

unsigned char cpu_get_esc_pwm_flag(void) {
	return cpu_esc_pwm_flag;
}

esc_pwm_t cpu_get_esc_pwm_data(void) {
	cpu_esc_pwm_flag = 0;
	return cpu_esc_pwm;
}

void cpu_clear_flags(void) {
	cpu_mode_flag = 0;
	cpu_esc_pwm_flag = 0;
}

unsigned char cpu_read_frame(void) {
	static unsigned char buffer[256];
	unsigned char valid_frame = 0;
	int n = 0;
	ioctl(cpu2mcu_port, FIONREAD, &n);
	if (n > 4) {
		read(cpu2mcu_port, buffer, n);
		valid_frame = 1;
		/* look for a preamble */
		int i = 0;
		while (i < n-2) {
			if (buffer[i] == 0xFF && buffer[i+1] == 0xFF) {	// we have a preamble
				int length = buffer[i+2];
				if (i+length+3 < n) {	// we may have a complete frame
					if (buffer[i+length+3] == crc_fast(buffer+i+3, length)) { // we have a crc
						/* check frame type */
						switch (buffer[i+3]) {
						case CPU_FRAME_MODE:
							cpu_mode = buffer[i+4];
							cpu_mode_flag = 1;
							valid_frame = 1;
							break;
						case CPU_FRAME_ESC:
							cpu_esc_pwm = *(esc_pwm_t *)(buffer+i+4);
							cpu_esc_pwm_flag = 1;
							valid_frame = 1;
							break;
						default: // Should never be there anyhow
							break;
						}
						i += 3+length; // Set cursor to last byte of frame
					} else {
						/* There is a crc error */
						printf("CPU COM: RX CRC\n");
					}
				} else {
					/* There is an incomplete frame */
					printf("CPU COM: RX CONTENT\n");
				}
			}
			i++;
		}
	}
	return valid_frame;
}

void cpu_init(void) {
	printf("CPU: Initializing pipes...\t");
	mkfifo(mcu2cpu_file, 0666);
	mkfifo(cpu2mcu_file, 0666);
	mcu2cpu_port = open(mcu2cpu_file, O_RDWR | O_NONBLOCK);
	cpu2mcu_port = open(cpu2mcu_file, O_RDONLY | O_NONBLOCK);
	printf("[OK]\n");
}

void cpu_send_test(void) {
	const char *test = "this is a test\n";
	write(mcu2cpu_port, test, sizeof("this is a test\n"));
}
