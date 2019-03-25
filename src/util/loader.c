#include <stdbool.h>            // Neede for bool
#include <stdint.h>             // Needed for uint32_t, uint16_t etc
#include <string.h>             // Needed for memcpy
#include <stdarg.h>             // Needed for variadic arguments
#include <stdlib.h>
#include "../drivers/stdio/emb-stdio.h"            // Needed for printf
#include "../boot/rpi-smartstart.h"        // Needed for smart start API
#include "../drivers/sdcard/SDCard.h"
#include "../hal/hal.h"
#include "console.h"
#include "loader.h"

uint8_t* loadBinaryFromFile(char *filePath, uint32_t *fSize) {
    HANDLE fHandle = sdCreateFile(filePath, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    uint8_t *buffer = NULL;

    if (fHandle) {
        *fSize = sdGetFileSize(fHandle, NULL);

        uint32_t bytesRead;
        buffer = malloc(*fSize + 1);

        sdReadFile(fHandle, &buffer[0], *fSize, &bytesRead, 0);

        sdCloseHandle(fHandle);

        return buffer;
    }

    return 0;
}

