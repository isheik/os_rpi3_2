//#include <stdbool.h>            // Neede for bool
//#include <stdint.h>             // Needed for uint32_t, uint16_t etc
#include <string.h>             // Needed for memcpy
#include <stdlib.h>
#include "../drivers/stdio/emb-stdio.h"            // Needed for printf
#include "../drivers/sdcard/SDCard.h"
#include "../hal/hal.h"
#include "console.h"
#include "loader.h"

void DisplayDirectory(const char*);
void dump(char *filePath);
uint8_t* loadBinaryFromFile(char *filePath);

void runConsole()
{
    
    uint8_t c;

    char linebuffer[1024] = { '\0' };
    size_t index = 0;
    char currdirr[1024] = { '\0' };
    char buffer[1024];


    long long cyclecounter = 0;

    printf( "$ ");

    while(1)
    {
        ++cyclecounter;
		c = hal_io_serial_getc( SerialA );
		hal_io_serial_putc( SerialA, c );

        if(c == '\r')
        {
            printf("\n");
            
            // sysinfo command
            if(strcmp(linebuffer,"sysinfo")==0)
            {
                printf("Kernel               ---  .::|[O-SU]|::. \nCPU ARCHITECTURE     ---    aarch_64\nThe System Has Been Running for %d cycles\n",cyclecounter);
            }

            // ls command
            else if(strcmp(linebuffer,"ls")==0)
            {
                char dirbuff[1024] = { '\0' };
                sprintf(dirbuff,"%s\\*.*",currdirr);
                DisplayDirectory(dirbuff);
            }
            // cd command
            else if(linebuffer[0] == 'c' && linebuffer[1] == 'd' && linebuffer[2] ==' ')
            {
                strcpy(currdirr,strcat(currdirr,linebuffer+3));
                printf("%s\n", currdirr);
            }
            
            // cat command
            else if(linebuffer[0] == 'c' && linebuffer[1] == 'a' && linebuffer[2] == 't' && linebuffer[3] == ' ')
            {
                char filename[1024] = { '\0' };
                strcpy(filename,linebuffer+4);
                //strcpy(currdirr,strcat(currdirr,linebuffer+4));
                
                HANDLE fHandle = sdCreateFile(filename, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
                if (fHandle != 0)
                {
                    uint32_t bytesRead;
                    uint32_t fSize = sdGetFileSize(fHandle, NULL);

                    
//                    if ((sdReadFile(fHandle, &buffer[0], 500, &bytesRead, 0) == true))
                    if ((sdReadFile(fHandle, &buffer[0], fSize, &bytesRead, 0) == true))
                    {
                            //buffer[bytesRead+1] = '\0';  ///insert null char
                            printf("File Contents: %s\n", &buffer[0]);
                    }
                    else
                    {
                        printf("Failed to read" );
                    }
                    
                    // Close the file
                    sdCloseHandle(fHandle);
                }
                else {
                    printf( "No such file\n" );                    
                }
            }

            // dump command
            else if(linebuffer[0] == 'd' && linebuffer[1] == 'u' && linebuffer[2] =='m' && linebuffer[3] =='p' && linebuffer[4] ==' ')
            {
                char filename[1024] = { '\0' };
                strcpy(filename,linebuffer+5);
//                strcpy(currdirr,strcat(currdirr,linebuffer+5));


                //strcpy(currdirr,strcat(currdirr,linebuffer+3));
                //printf("dump");
                dump(filename);
            }

            // exec command
            else if(linebuffer[0] == 'e' && linebuffer[1] == 'x' && linebuffer[2] =='e' && linebuffer[3] =='c' && linebuffer[4] ==' ')
            {
                char filename[1024] = { '\0' };
                strcpy(filename,linebuffer+5);
//                strcpy(currdirr,strcat(currdirr,linebuffer+5));

                char *buf;
                int ret;
                buf = loadBinaryFromFile(filename);
                ret = ((int (*)(void))buf)();
                printf( "%d", ret );

            }

            // if inputted app exists, execute it
            else if(linebuffer[0] != '\0') 
            {
                char filename[1024] = { '\0' };
                strcpy(filename,linebuffer);
//                strcpy(currdirr,strcat(currdirr,linebuffer+5));

                HANDLE fh = 0;
                FIND_DATA find;
                fh = sdFindFirstFile(filename, &find);
                if(fh != 0)
                {
                    char *buf;
                    int ret;
                    buf = loadBinaryFromFile(filename);
                    ret = ((int (*)(void))buf)();
                    printf( "Return value from app: %d\n", ret );
                    free(buf);
                }
                else {
                    printf( "No such file\n" );                    
                }
            }


            memset(linebuffer,'\0',1024);
            index = 0;
            hal_io_serial_puts(SerialA,"\n");
        }
        else 
        {
            linebuffer[index++] = c;
        }
		printf( "%c", c );

        if (index == 0) {
            printf( "$ ");
        }
	}
}

void DisplayDirectory(const char* dirName) {
    HANDLE fh;
    FIND_DATA find;
    char* month[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    fh = sdFindFirstFile(dirName, &find);                           // Find first file
    do {
        if (find.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
            printf("%s <DIR>\n", find.cFileName);
        else printf("%c%c%c%c%c%c%c%c.%c%c%c Size: %9lu bytes, %2d/%s/%4d, LFN: %s\n",
            find.cAlternateFileName[0], find.cAlternateFileName[1],
            find.cAlternateFileName[2], find.cAlternateFileName[3],
            find.cAlternateFileName[4], find.cAlternateFileName[5],
            find.cAlternateFileName[6], find.cAlternateFileName[7],
            find.cAlternateFileName[8], find.cAlternateFileName[9],
            find.cAlternateFileName[10],
            (unsigned long)find.nFileSizeLow,
            find.CreateDT.tm_mday, month[find.CreateDT.tm_mon],
            find.CreateDT.tm_year + 1900,
            find.cFileName);                                        // Display each entry
    } while (sdFindNextFile(fh, &find) != 0);                       // Loop finding next file
    sdFindClose(fh);                                                // Close the serach handle
}

void dump(char *filePath) {
    HANDLE fHandle = sdCreateFile(filePath, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    uint8_t *buffer = NULL;

    if (fHandle) {
            uint32_t fSize = sdGetFileSize(fHandle, NULL);
            printf("filesize in uint8_t is: %i\n", (uint8_t) fSize);

            uint32_t bytesRead;
            buffer = malloc(fSize + 1);

            sdReadFile(fHandle, &buffer[0], fSize, &bytesRead, 0);

            sdCloseHandle(fHandle);
            
            printf("Text version:\n");
            for(uint8_t i = 0; i <= (uint8_t)fSize; i++) {
                printf("%c ", *(buffer+i));
            }

            printf("\nHex:\n");
            for(uint8_t i = 0; i <= (uint8_t)fSize; i++) {
                printf("%02X ", *(buffer+i));
            }
            printf("\n");
            free(buffer);
    }
}
