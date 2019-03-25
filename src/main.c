/**
*     	main.c
*
*				Kernel's Entry Point
*
*/

#include <stdbool.h>			// Neede for bool
#include <stdint.h>				// Needed for uint32_t, uint16_t etc
#include <string.h>				// Needed for memcpy
#include <stdarg.h>				// Needed for variadic arguments
#include "drivers/stdio/emb-stdio.h"			// Needed for printf
#include "boot/rpi-smartstart.h"		// Needed for smart start API
#include "drivers/sdcard/SDCard.h"
#include "hal/hal.h"
#include "util/console.h"

int main (void) 
{
	PiConsole_Init(0, 0, 0, &printf);								// Auto resolution console, show resolution to screen

	displaySmartStart(&printf);
	printf("\n");															// Display smart start details
	ARM_setmaxspeed(&printf);										// ARM CPU to max speed and confirm to screen

	sdInitCard (&printf, &printf, true);
	printf("\n");

	hal_io_video_init();

	//Typewriter
	hal_io_serial_init();
	hal_io_serial_puts( SerialA, "Typewriter:" );

	runConsole();

	return(0);
}

