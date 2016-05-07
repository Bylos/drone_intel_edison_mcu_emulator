/*
 * debug.c
 *
 *  Created on: 6 mai 2016
 *      Author: Bertrand Massot
 */

#include "debug.h"
#include <stdio.h>

void debug_print(float time, const char* level, const char* message) {
	FILE* dbg_file = fopen("/etc/drone/log_mcu_emulator.txt", "a");
	fprintf(dbg_file, "%08.3f %s %s\n", time, level, message);
	fclose(dbg_file);
}
