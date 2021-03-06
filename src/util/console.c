#include <string.h>             // Needed for memcpy
#include <stdlib.h>
#include "../drivers/stdio/emb-stdio.h"            // Needed for printf
#include "../drivers/sdcard/SDCard.h"
#include "../hal/hal.h"
#include "console.h"
#include "loader.h"

void DisplayDirectory(const char*);
void sysinfo();
void ls();
void cd();
void cat(char *filename);
void dump(char *filePath);
void exec_app(char *filename);

char linebuffer[1024] = { '\0' };
char currdirr[1024] = { '\0' };
long long cyclecounter = 0;
size_t pathlength = 0;
size_t lastlength = 0;

void runConsole()
{   
    uint8_t c;
    size_t index = 0;
   
    printf( "$ ");

    while(1)
    {
        ++cyclecounter;
		c = hal_io_serial_getc( SerialA );
		hal_io_serial_putc( SerialA, c );

        if(c == '\r')
        {
            printf("\n");
            
            if(strcmp(linebuffer,"sysinfo")==0)
            {
                sysinfo();
            }
            else if(strcmp(linebuffer,"ls")==0)
            {
                ls();
            }
            else if(linebuffer[0] == 'c' && linebuffer[1] == 'd' && linebuffer[2] ==' ')
            {
                cd();
            }
            else if(linebuffer[0] == 'c' && linebuffer[1] == 'a' && linebuffer[2] == 't' && linebuffer[3] == ' ')
            {
                char filename[1024] = { '\0' };
                sprintf(filename,"%s\\%s",currdirr,linebuffer+4);
                cat(filename);
            }
            else if(linebuffer[0] == 'd' && linebuffer[1] == 'u' && linebuffer[2] =='m' && linebuffer[3] =='p' && linebuffer[4] ==' ')
            {
                // dump command
                char filename[1024] = { '\0' };
                sprintf(filename,"%s\\%s",currdirr,linebuffer+5);
                dump(filename);
            }
            else if(linebuffer[0] != '\0') 
            {
                char filename[1024] = { '\0' };
                sprintf(filename,"%s\\%s",currdirr,linebuffer);
                exec_app(filename);

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

void DisplayDirectory(const char* dirName) 
{
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

void sysinfo() 
{
    // sysinfo command
    printf("Kernel               ---  .::|[O-SU]|::. \nCPU ARCHITECTURE     ---    aarch_64\nThe System Has Been Running for %d cycles\n",cyclecounter);
}

void ls() 
{
    // ls command
    char dirbuff[1024] = { '\0' };
    sprintf(dirbuff,"%s\\*.*",currdirr);
    DisplayDirectory(dirbuff);
}

void cd() 
{
    // cd command
    if(linebuffer[3] == '/')
    {
        memset(currdirr,'\0',1024);
        pathlength = lastlength = 0;
    }
    else if(linebuffer[3] == '.' && linebuffer[4] == '.')
    {
        memset(currdirr + lastlength,'\0',1024 - lastlength);
    } 
    else
    {
        lastlength = strlen(currdirr);
        strcpy(currdirr,strcat(currdirr,linebuffer+3));
    }
    pathlength = strlen(currdirr);
    
    printf("%s\n", currdirr);
}

void cat(char *filename) 
{
    // cat command
    HANDLE fHandle = sdCreateFile(filename, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (fHandle != 0)
    {
        uint32_t bytesRead;
        uint32_t fSize = sdGetFileSize(fHandle, NULL);
        char buffer[1024] = { '\0' };

        if ((sdReadFile(fHandle, &buffer[0], fSize, &bytesRead, 0) == true))
        {
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

void dump(char *filePath) 
{
    uint8_t *buffer = NULL;
    uint32_t *fSize;
    uint32_t bytesRead;
    
    buffer = loadBinaryFromFile(filePath, fSize);
    if(buffer != 0) {
        printf("Text version:\n");
        for(uint32_t i = 0; i <= *fSize; i++) {
            printf("%c ", *(buffer+i));
        }

        printf("\nHex:\n");
        for(uint32_t i = 0; i <= *fSize; i++) {
            printf("%02X ", *(buffer+i));
        }
        printf("\n");
        free(buffer);
        free(fSize);
    }
    else {
        printf( "No such file\n" );                    
    }
}

void exec_app(char *filename) 
{
    // if inputted app exists, execute it
    HANDLE fh = 0;
    FIND_DATA find;
    fh = sdFindFirstFile(filename, &find);
    if(fh != 0)
    {
        char *buf;
        int ret;
        uint32_t *fSize;
        buf = loadBinaryFromFile(filename, fSize);
        ret = ((int (*)(void))buf)();
        printf( "Return value from app: %d\n", ret );
        free(buf);
        free(fSize);
    }
    else {
        printf( "No such file\n" );                    
    }
}

