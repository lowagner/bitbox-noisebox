#include "bitbox.h" 
#include <stdlib.h> // rand
#include <math.h>
#include <stdio.h>
#include <string.h>

uint16_t color CCM_MEMORY;
uint8_t music_on CCM_MEMORY;

#include "fatfs/ff.h"

#define MAX_READ_BYTES 512

FATFS fat_fs CCM_MEMORY;
FIL fat_file CCM_MEMORY;
uint8_t mount_ok CCM_MEMORY;
int file_count;
int file_index;
#define MAX_FILES 50
char filenames[MAX_FILES][13];

#ifdef EMULATOR
#define ROOT_DIR "."
#else
#define ROOT_DIR ""
#endif

static int cmp(const void *p1, const void *p2){
    return strcmp( (char * const ) p1, (char * const ) p2);
}

void list_music() // adapted from list_roms from bitbox/2nd_boot
{
    message("listing music\n");
    file_index = 0;

    FRESULT res;
    FILINFO fno;
    DIR dir;

    char *fn;   /* This function is assuming non-Unicode cfg. */

    if (f_opendir(&dir, ROOT_DIR) != FR_OK) 
    {
        color = RGB(255, 255, 0);
        message("bad dir\n");
        return;
    }
    for (file_count=0; file_count<MAX_FILES;) 
    {
        res = f_readdir(&dir, &fno);                   /* Read a directory item */
        if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
        if (fno.fname[0] == '.' || (fno.fattrib & AM_DIR)) /* Ignore dot entry and dirs */
            continue;
        fn = fno.fname;
        message("checking file %s\n", fn);
        // check extension : only keep .w8v
        if (strstr(fn,".W8V") || strstr(fn,".w8v")) // search ignoring case
            strcpy(filenames[file_count++],fn);
    }
    f_closedir(&dir);

    // sort it
    message("got to end with count %d\n", file_count);
    qsort(filenames, file_count, 13, cmp);
    for (int i=0; i<file_count; ++i)
    {
        message("got filename %s\n", filenames[i]);
    }
}

void load_next_file()
{
    music_on = 0;
    if (!mount_ok)
        return;
    
    if (file_index >= file_count)
        file_index = 0;
    message("opening the %d .w8v: ", file_index);
    if (f_open(&fat_file, filenames[file_index++], FA_READ | FA_OPEN_EXISTING) != FR_OK)
    {
        message("NO OPEN!\n");
        color = RGB(255,0,255);
        music_on = 0;
        return;
    }
    message("ok!\n");
    music_on = 1;
}

void game_init()
{
    color = 0;
    mount_ok = 0;
    music_on = 0;
        
    if (f_mount(&fat_fs, "", 1) != FR_OK)
    {
        color = RGB(255,255,255);
        return;
    }
    mount_ok = 255;
    list_music();
}

void game_frame()
{
}

#ifndef NO_VGA
void graph_frame() 
{
}

void graph_line() 
{
    //const uint8_t max_color = 8*(music_on+1);
    //color = ((rand()%max_color)<<10)|((rand()%max_color)<<5)|(rand()%max_color);
    draw_buffer[rand()%320] = color;
}
#endif

static inline uint16_t gen_sample()
{
    // This is a simple noise generator based on an LFSR (linear feedback shift
    // register). It is fast and simple and works reasonably well for audio.
    // Note that we always run this so the noise is not dependent on the
    // oscillators frequencies.
    static uint32_t noiseseed[2] = {1, 150};
    static uint32_t rednoise[2] = {0, 3};
    for (int i=0; i<2; ++i)
    {
        uint32_t newbit = 0;
        if (noiseseed[i] & 0x80000000L) newbit ^= 1;
        if (noiseseed[i] & 0x01000000L) newbit ^= 1;
        if (noiseseed[i] & 0x00000040L) newbit ^= 1;
        if (noiseseed[i] & 0x00000200L) newbit ^= 1;
        noiseseed[i] = (noiseseed[i] << 1) | newbit;
        rednoise[i] = 3*rednoise[i]/4 + (noiseseed[i]&255)/4;
    }

    uint8_t value[2];
    // Now compute the value of each oscillator and mix them
    value[0] = 128-32+(rednoise[0]&255)/4;
    value[1] = 128-32+(rednoise[1]&255)/4;
    // Now put the two channels together in the output word
    return *((uint16_t *)value);
}

void game_snd_buffer(uint16_t *buffer, int len) 
{
    static uint16_t press=0;
    static uint32_t press_hold=0;
    if (press)
    {
        press = button_state();
        if (!press)
        {
            // button release, do something fancier for holding down the button
            music_on = (music_on+1)%2;
            if (music_on == 1)
            {
                color = RGB(0,255,255);
                load_next_file();
            }
        }
        else
        {
            // still pressing the button
            ++press_hold;
            const uint8_t max_color = 8*(music_on+1);
            color = ((rand()%max_color)<<10)|((rand()%max_color)<<5)|(rand()%max_color);
           
            memset(buffer, 128, 2*len);
            return;
        }
    }
    else
    {
        press_hold = 0;
        press = button_state();
    }
    if (!music_on)
    {
        for (int i=0; i<len; i++)
            buffer[i] = gen_sample();
        return;
    }
    color = RGB(0, 255, 0);
    int remaining_bytes = 2*len;
    const uint16_t *buffer_end = &buffer[len];
    while (remaining_bytes > 0)
    {
        size_t bytes_to_read;
        if (remaining_bytes > MAX_READ_BYTES)
        {
            bytes_to_read = MAX_READ_BYTES;
            remaining_bytes -= MAX_READ_BYTES;
        }
        else
        {
            bytes_to_read = remaining_bytes;
            remaining_bytes = 0;
        }
        
        UINT bytes_read;
        if (f_read(&fat_file, buffer, bytes_to_read, &bytes_read) != FR_OK)
        {
            message("file not read well\n");
            color = RGB(255,0,255);
            f_close(&fat_file);
            music_on = 0;
            return;
        }
        else if (bytes_read != bytes_to_read)
        {
            message("file ended\n");
            color = RGB(255,0,0);
            buffer += bytes_to_read/2;
            while (buffer < buffer_end)
                *buffer++ = 128;
            f_close(&fat_file);
            music_on = 0;
            return;
        }
        buffer += bytes_to_read/2;
    }
    return;
}
