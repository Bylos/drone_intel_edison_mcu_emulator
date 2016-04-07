/* pca9685.c
 * Source file for interfacing PCA9685 with the Edison MCU
 * Sparkfun's PWM Block only exposes 8 channels so only them
 * are implemented for now.
 * This is a direct implementation of OneShot125 for ESCs
 * connected on channels 0-3
 */

#include "drivers.h"

static mraa_i2c_context i2c = NULL;

/**
 * Fast call for single byte write to PCA9685
 */
void pca_write(unsigned char reg, unsigned char value) {
	mraa_i2c_address(i2c, PCA_ADDRESS);
	if(mraa_i2c_write_byte_data(i2c, value, reg) != MRAA_SUCCESS) {
		printf("write single byte to PCA9685 failed...\n");
	}
}

/**
 * Fast call for multiple bytes write to PCA9685
 */
void pca_write_bytes(unsigned char *buf, int len) {
	mraa_i2c_address(i2c, PCA_ADDRESS);
	if(mraa_i2c_write(i2c, buf, len) != MRAA_SUCCESS) {
		printf("write multiple bytes to PCA9685 failed...\n");
	}
}

/**
 * Set up PCA9685 mode and frequency
 */
void pca_init() {
	int i2c_bus = 1;
	i2c = mraa_i2c_init_raw(i2c_bus);
	if (i2c == NULL) {
		printf("PCA9685: Failed to init i2c bus %d\n", i2c_bus);
		return;
	}
	printf("PCA9685: Starting PWM Periods...\t");
	pca_write(PCA_MODE1, 0x00);
	pca_write(PCA_MODE2, 0x00);
	pca_write(PCA_MODE1, PCA_SLEEP);
	pca_write(PCA_PRE_SCALE, PCA_SCALER_1526_HZ);
	pca_write(PCA_MODE1, PCA_RESTART | PCA_AI);
	pca_write(PCA_MODE2, 0x00);
	pca_stop();
	printf("[OK]\n");
}

/**
 * Calibrate ESCs to ONESHOT125 mode
 * Must be done just ONCE without previous activations
 */
unsigned char allow_calibration = 1;
void pca_calibrate() {
	if (allow_calibration) {
		printf("PCA9685: Calibrating ESCs...\t");
		pca_channel_0_3(
				PCA_ONE_SHOT_1526_HZ_OFFSET + PCA_ONE_SHOT_1526_HZ_RANGE,
				PCA_ONE_SHOT_1526_HZ_OFFSET + PCA_ONE_SHOT_1526_HZ_RANGE,
				PCA_ONE_SHOT_1526_HZ_OFFSET + PCA_ONE_SHOT_1526_HZ_RANGE,
				PCA_ONE_SHOT_1526_HZ_OFFSET + PCA_ONE_SHOT_1526_HZ_RANGE);
		nanosleep((const struct timespec[]){{5, 000000000L}}, NULL);
		pca_channel_0_3(
					PCA_ONE_SHOT_1526_HZ_OFFSET,
					PCA_ONE_SHOT_1526_HZ_OFFSET,
					PCA_ONE_SHOT_1526_HZ_OFFSET,
					PCA_ONE_SHOT_1526_HZ_OFFSET);
		nanosleep((const struct timespec[]){{5, 000000000L}}, NULL);
		printf("[OK]\n");
	} else {
		printf("PCA9685: Calibration requested is forbidden !\n");
	}
}

/**
 * Stop all channels
 */
void pca_stop() {
	unsigned char stop_bytes[5] = {PCA_ALL_LED, 0x00, 0x00, 0x00, 0x10};
	pca_write_bytes(stop_bytes, 5);
}

/**
 * Set channels 0-3 on value in a single sequence
 */
void pca_channel_0_3(int ch0, int ch1, int ch2, int ch3) {
	allow_calibration = 0;
	unsigned char buffer[17] = {PCA_LED0,
			0x00, 0x00, (unsigned char)(ch0), (unsigned char)(ch0>>8),
			0x00, 0x00, (unsigned char)(ch1), (unsigned char)(ch1>>8),
			0x00, 0x00, (unsigned char)(ch2), (unsigned char)(ch2>>8),
			0x00, 0x00, (unsigned char)(ch3), (unsigned char)(ch3>>8),
	};
	pca_write_bytes(buffer, 17);
}
