/* xbee_zb.h
 * Source file for interfacing XBEE ZB module with EDISON on UART1
 */

#include <termios.h>		// struct termios, tcgetattr, tcsetattr, tcflush
#include <errno.h>			// errno
#include <string.h>			// memset()
#include <sys/ioctl.h>		// ioctl(FIONREAD)
#include <fcntl.h>			// O_RDWR O_NDELAY O_NOCTTY
#include "drivers.h"

const char *uart_file = "/dev/ttyMFD1";
static int uart;

/* Declare rc data variables and associated flags */
joystick_t joystick;
unsigned char joystick_flag = 0;
rc_command_t rc_command;
unsigned char rc_command_flag = 0;

int set_interface_attribs (int fd, int speed, int parity)
{
	struct termios tty;
	memset (&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0)
	{
		printf ("error %d from tcgetattr\n", errno);
		return -1;
	}

	cfsetospeed (&tty, speed);
	cfsetispeed (&tty, speed);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
	// disable IGNBRK for mismatched speed tests; otherwise receive break
	// as \000 chars
	tty.c_iflag &= ~IGNBRK;         // disable break processing
	tty.c_lflag = 0;                // no signaling chars, no echo,
								// no canonical processing
	tty.c_oflag = 0;                // no remapping, no delays
	tty.c_cc[VMIN]  = 0;            // read doesn't block
	tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

	tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
								// enable reading
	tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
	tty.c_cflag |= parity;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag |= CRTSCTS;

	if (tcsetattr (fd, TCSANOW, &tty) != 0)
	{
		printf ("error %d from tcsetattr\n", errno);
		return -1;
	}
	return 0;
}

void set_blocking (int fd, int should_block)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                printf ("error %d from tggetattr\n", errno);
                return;
        }

        tty.c_cc[VMIN]  = should_block ? 1 : 0;
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
                printf ("error %d setting term attributes\n", errno);
}

/*
 * Set up UART for communication with module
 */
void xbee_init() {
	if((uart = open(uart_file, O_RDWR | O_NOCTTY | O_NDELAY)) < 0) {
		printf("XBEE ZB: Unable to open UART %s\n", uart_file);
		return;
	}
	printf("XBEE ZB: Configuring UART...\t");
	set_interface_attribs(uart, B115200, 0);
	set_blocking(uart, 0);
	nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
	tcflush(uart, TCIOFLUSH);
	nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
	printf("[OK]\n");
}

/*
 * Send state to controller
 */
void xbee_advertize(unsigned char state) {
	unsigned char buffer[6] = {0xFF, 0xFF, 0x02, 0x00, state, 0x00};
	buffer[5] = crc_fast(buffer+3, 2);
	if (write(uart, buffer, 6) == -1) {
		printf("XBEE ZB: Error sending state");
	}
}

/*
 * Read UART to get new channel values;
 */
unsigned short xbee_read_frame(void) {
	/* First part : check for RC data on UART */
	short rc_data[16];
	unsigned short rc_channel = 0;
	unsigned char buffer[128];
	unsigned char *buffer_ptr = buffer;
	unsigned char length;
	rc_t *rc;
	unsigned char i;

	int n;
	ioctl(uart, FIONREAD, &n);
	if (n < 5) {
		return 0;
	}
	n = read(uart, buffer_ptr, n);
	if (buffer[0] == 0xFF && buffer[1] == 0xFF) {
		length = buffer[2];
		if (length > 8) {
			printf("XBEE ZB: length higher than 8 !\n");
			return 0;
		}
		if (n < length+4) {
			int n2 = 0;
			while (n2 < length+4-n) {
				ioctl(uart, FIONREAD, &n2);
			}
			read(uart, buffer_ptr+n, length+4-n);
		}
		buffer_ptr += 3;
		for (i=0; i<length; i+=2) {
			rc = (rc_t*)(buffer_ptr+i);
			rc_data[rc->channel] = rc->data;
			rc_channel |= 1 << rc->channel;
		}
		if (*(buffer_ptr+length) != crc_fast(buffer_ptr, length)) {
			printf("XBEE ZB: RX CRC\n");
			int j;
			for (j=0; j<n; j++) {
				printf("%x ", buffer[j]);
			}
			printf("\n");
			rc_channel = 0;
		}
	} else {
		printf("XBEE ZB: RX Preamble\n");
		int j;
		for (j=0; j<n; j++) {
			printf("%x ", buffer[j]);
		}
		printf("\n");
		rc_channel = 0;
	}
	/* Second part : store data and raise flags */
	if (rc_channel & (1 << CH_RIGHT_Y)) { // assume that if throttle is present, yaw pitch and roll are also
		joystick.right.y = rc_data[CH_RIGHT_Y];	// throttle
		joystick.right.x = rc_data[CH_RIGHT_X];	// yaw
		joystick.left.y  = rc_data[CH_LEFT_Y];		// pitch
		joystick.left.x  = rc_data[CH_LEFT_X];		// roll
		joystick_flag = 1;
	}
	if (rc_channel & (1 << CH_COMMAND)) {
		rc_command = rc_data[CH_COMMAND];
		rc_command_flag = 1;
	}
	return rc_channel; // return a non-zero value to reinit timeout
}

joystick_t xbee_get_joystick_data(void) {
	joystick_flag = 0;
	return joystick;
}

unsigned char xbee_get_joystick_flag(void) {
	return joystick_flag;
}

rc_command_t xbee_get_command(void) {
	rc_command_flag = 0;
	return rc_command;
}

unsigned char xbee_get_command_flag(void) {
	return rc_command_flag;
}

void xbee_clear_flags(void) {
	joystick_flag = 0;
	rc_command_flag = 0;
}
