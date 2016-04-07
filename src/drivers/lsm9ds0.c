/* lsm9ds0.c
 * Source file for interfacing LSM9DS0 with the Edison MCU
 */

#include "drivers.h"
#include <mraa.h>
static mraa_i2c_context i2c = NULL;

/**
 * Fast call for single byte write to LSM9DS0 Gyroscope
 *
 */
void lsm_g_write(unsigned char reg, unsigned char value) {
	mraa_i2c_address(i2c, LSM_ADDRESS_G);
	if(mraa_i2c_write_byte_data(i2c, value, reg) != MRAA_SUCCESS) {
		printf("write single byte to LSM9DS0 G failed...\n");
	}
}

/**
 * Fast call for single byte read from LSM9DS0 Gyroscope
 *
 */
unsigned char lsm_g_read(unsigned char reg) {
	mraa_i2c_address(i2c, LSM_ADDRESS_G);
	return mraa_i2c_read_byte_data(i2c, reg);
}

/**
 * Fast call for multiple bytes read from LSM9DS0 Gyroscope
 *
 */
void lsm_g_read_bytes(unsigned char reg, unsigned char *buf, int len) {
	mraa_i2c_address(i2c, LSM_ADDRESS_G);
	if(mraa_i2c_read_bytes_data(i2c, reg|0x80, buf, len) == -1) {
		printf("read multiple bytes from LSM9DS0 G failed...\n");
	}
}

/**
 * Fast call for single byte write to LSM9DS0 Accelerometer / Magnetometer
 *
 */
void lsm_xm_write(unsigned char reg, unsigned char value) {
	mraa_i2c_address(i2c, LSM_ADDRESS_XM);
	if(mraa_i2c_write_byte_data(i2c, value, reg) != MRAA_SUCCESS) {
		printf("write single byte to LSM9DS0 XM failed...\n");
	}
}

/**
 * Fast call for single byte read to LSM9DS0 Accelerometer / Magnetometer
 *
 */
unsigned char lsm_xm_read(unsigned char reg) {
	mraa_i2c_address(i2c, LSM_ADDRESS_XM);
	return mraa_i2c_read_byte_data(i2c, reg);
}

/**
 * Fast call for multiple bytes read from LSM9DS0 Accelerometer / Magnetometer
 *
 */
void lsm_xm_read_bytes(unsigned char reg, unsigned char *buf, int len) {
	mraa_i2c_address(i2c, LSM_ADDRESS_XM);
	if(mraa_i2c_read_bytes_data(i2c, reg|0x80, buf, len) == -1) {
		printf("read multiple bytes from LSM9DS0 XM failed...\n");
	}
}

/**
 * Start and set gyroscope's parameters
 */
void lsm_gyro_start(gyro_scale_t scale, gyro_odr_t odr) {
	printf("LSM9DS0: Starting gyroscope...\t");
	/* Setup gyroscope control registers */
	lsm_g_write(LSM_CTRL_REG1_G, 0x0F | (odr << 4));
	lsm_g_write(LSM_CTRL_REG2_G, 0x00);
	lsm_g_write(LSM_CTRL_REG3_G, 0x00);
	lsm_g_write(LSM_CTRL_REG4_G, scale << 4);
	lsm_g_write(LSM_FIFO_CTRL_REG_G, 0x00);
	lsm_g_write(LSM_CTRL_REG5_G, 0x00);
	printf("[OK]\n");
}

/**
 * Start and set accelerometer's parameters
 */
void lsm_accel_start(accel_scale_t scale, accel_odr_t odr, accel_abw_t abw) {
	printf("LSM9DS0: Starting accelerometer...\t");
	lsm_xm_write(LSM_FIFO_CTRL_REG, 0x00);
	lsm_xm_write(LSM_CTRL_REG0_XM, 0x00);
	lsm_xm_write(LSM_CTRL_REG1_XM, 0x07 | (odr << 4));
	lsm_xm_write(LSM_CTRL_REG2_XM, (abw << 6) | (scale << 3));
	lsm_xm_write(LSM_CTRL_REG3_XM, 0x00);
	printf("[OK]\n");
}

/**
 * Start and set magnetometer's parameters
 */
void lsm_magn_start(magn_scale_t scale, magn_odr_t odr) {
	printf("LSM9DS0: Starting magnetometer...\t");
	lsm_xm_write(LSM_CTRL_REG5_XM, (odr << 2));
	lsm_xm_write(LSM_CTRL_REG6_XM, (scale << 5));
	lsm_xm_write(LSM_CTRL_REG7_XM, 0x00);
	printf("[OK]\n");
}

/*
 * Wake up the sensors and set up scales and rates
 */
void lsm_init() {
	int i2c_bus = 1;
	i2c = mraa_i2c_init_raw(i2c_bus);
	if (i2c == NULL) {
		printf("LSM9DS0: Failed to init i2c bus %d\n", i2c_bus);
		return;
	}
	lsm_gyro_start(G_SCALE_500DPS, G_ODR_190_BW_25);
	lsm_accel_start(A_SCALE_4G, A_ODR_400, A_ABW_50);
	lsm_magn_start(M_SCALE_2GS, M_ODR_100);

	nanosleep((const struct timespec[]){{0, 500000000L}}, NULL); // Wait 500ms for sensors to wake-up
}

/*
 * Returns accelerometer, magnetometer and gyroscope sensors values
 */
inertial_t lsm_data;
unsigned char lsm_data_flag = 0;
void lsm_read_inertial(void) {

	/* Read Gyroscope values */
	lsm_g_read_bytes(LSM_OUT_X_L_G, (unsigned char*)(&(lsm_data.gyro.x)), 6);
	/* Read Accelerometer values */
	lsm_xm_read_bytes(LSM_OUT_X_L_A, (unsigned char*)(&(lsm_data.accel.x)), 6);
	/* Read magnetometer values */
	lsm_xm_read_bytes(LSM_OUT_X_L_M, (unsigned char*)(&(lsm_data.magn.x)), 6);

	/* Correct axes signs */
	// Not in this version

	/* Raise flag */
	lsm_data_flag = 1;
	return;
}

inertial_t lsm_get_inertial_data(void) {
	lsm_data_flag = 0;
	return lsm_data;
}

unsigned char lsm_get_inertial_flag(void) {
	return lsm_data_flag;
}

void lsm_clear_flags(void) {
	lsm_data_flag = 0;
}
