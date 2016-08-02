#include "bitbox.h" 
#include <stdlib.h> // rand
#include <math.h>
#include <stdio.h>
#include <string.h>

uint16_t color_ring[256] CCM_MEMORY;
uint8_t color_ring_begin CCM_MEMORY;
uint8_t color_ring_end CCM_MEMORY;
uint8_t stripe CCM_MEMORY;

static inline int push_color(uint16_t c)
{
    if (((color_ring_end+1)&255) == color_ring_begin)
        return 1;
    color_ring[color_ring_end++] = c;
    return 0;
}

static inline int pop_color()
{
    if (color_ring_begin == color_ring_end)
        return -1;
    return color_ring[color_ring_begin++];
}

static inline int color_ring_empty()
{
    return color_ring_begin == color_ring_end;
}
uint8_t music_on CCM_MEMORY;
uint32_t press_hold CCM_MEMORY;

#include "fatfs/ff.h"

#define MAX_READ_BYTES 512

FATFS fat_fs CCM_MEMORY;
FIL fat_file CCM_MEMORY;
uint8_t mount_ok CCM_MEMORY;
uint8_t load_next_file_please CCM_MEMORY;
int file_count CCM_MEMORY;
int file_index CCM_MEMORY;
#define MAX_FILES 64
char filenames[MAX_FILES][13];

#ifdef EMULATOR
#define ROOT_DIR "."
#else
#define ROOT_DIR ""
#endif

void get_file_index()
{
    if (!mount_ok)
    {
        push_color(RGB(255,255,255));
        return;
    }
    UINT bytes_get;
    if (f_open(&fat_file, "SONGNOSE.TXT", FA_READ | FA_OPEN_EXISTING) != FR_OK)
    {
        file_index = 0;
        message("error opening SONGNOSE.TXT file\n");
        push_color(RGB(255,150,0));
        return;
    }
    if (f_read(&fat_file, &file_index, sizeof(file_index), &bytes_get) != FR_OK ||
        bytes_get != sizeof(file_index))
    {
        message("error reading SONGNOSE.TXT file\n");
        push_color(RGB(255,0,0));
        file_index = 0;
    }
    message("got file_index %d\n", file_index);
    f_close(&fat_file);
}

void set_file_index(int skip)
{
    if (!mount_ok)
    {
        push_color(RGB(255,255,255));
        return;
    }
    UINT bytes_get;
    
    if (f_open(&fat_file, "SONGNOSE.TXT", FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) 
    {
        message("error opening SONGNOSE.TXT file to write\n");
        push_color(RGB(255,0,255));
        return;
    }
    if (f_write(&fat_file, &skip, sizeof(skip), &bytes_get) != FR_OK ||
        bytes_get != sizeof(skip))
    {
        push_color(RGB(100,0,255));
        message("error in writing file_index to SONGNOSE.TXT\n");
    }
    f_close(&fat_file);
}


static int cmp(const void *p1, const void *p2){
    return strcmp( (char * const ) p1, (char * const ) p2);
}

void list_music() // adapted from list_roms from bitbox/2nd_boot
{
    message("listing music\n");
    get_file_index();

    FRESULT res;
    FILINFO fno;
    DIR dir;

    char *fn;   /* This function is assuming non-Unicode cfg. */

    if (f_opendir(&dir, ROOT_DIR) != FR_OK) 
    {
        push_color(RGB(255, 255, 0));
        message("bad dir\n");
        return;
    }
    int looped = 0;
    for (file_count=0;;) 
    {
        res = f_readdir(&dir, &fno);                   /* Read a directory item */
        message("checking file %s\n", fno.fname);
        if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
        /* Ignore dot entry and dirs */
        if (fno.fname[0] == '.')
        {
            message("this was a dot\n");
            continue;
        }
        if (fno.fattrib & AM_DIR) 
        {
            message("this was a directory\n");
            continue;
        }
        fn = fno.fname;
        // check extension : only keep .w8v
        if (strstr(fn,".W8V") || strstr(fn,".w8v")) // search ignoring case
        {
            message("got w8v\n");
            int buffer_index = file_count % MAX_FILES;
            if (buffer_index == file_index % MAX_FILES)
            {
                if (looped == 1 + file_index/MAX_FILES)
                    break;
                else
                    ++looped;
            }
            strcpy(filenames[buffer_index],fn);
            ++file_count;
        }
    }
    f_closedir(&dir);
    message("got to end with count %d\n", file_count);
    // check if we looped more than once:
    int original_file_count = file_count;
    if (file_count > MAX_FILES)
    {
        message(" we were aiming at index [%d, +64)\n", file_index);
        file_count = MAX_FILES;
    }
    if (!file_count)
        return;
    set_file_index((file_index + file_count/2)%original_file_count);
    file_index %= file_count;

    // sort it
    qsort(filenames, file_count, 13, cmp);
    for (int i=0; i<file_count; ++i)
    {
        message("got filename %s\n", filenames[i]);
    }
}

void load_next_file()
{
    load_next_file_please = 0;

    music_on = 0;
    if (!mount_ok)
    {
        push_color(RGB(255,255,255));
        return;
    }
    if (!file_count)
    {
        push_color(RGB(255,255,0));
        return;
    }
    
    if (file_index >= file_count)
        file_index = 0;
    push_color(RGB(0,255,255));
    message("opening the %d music, %s: ", file_index, filenames[file_index]);
    if (f_open(&fat_file, filenames[file_index++], FA_READ | FA_OPEN_EXISTING) != FR_OK)
    {
        message("NO OPEN!\n");
        push_color(RGB(255,0,255));
        music_on = 0;
        return;
    }
    push_color(RGB(0,255,0));
    message("ok!\n");
    music_on = 1;
}

void game_init()
{
    stripe = 0;
    mount_ok = 0;
    music_on = 0;
        
    if (f_mount(&fat_fs, "", 1) != FR_OK)
    {
        push_color(RGB(255,255,255));
        return;
    }
    mount_ok = 255;
    list_music();
}

void game_frame()
{
    if (load_next_file_please)
        load_next_file();
}

#ifndef NO_VGA
void graph_frame() 
{
}

void graph_line() 
{
    if (vga_odd)
        return;
    //const uint8_t max_color = 8*(music_on+1);
    //color = ((rand()%max_color)<<10)|((rand()%max_color)<<5)|(rand()%max_color);
    if (color_ring_empty())
        return;
    // check if there is more than one object in the ring:
    if (((color_ring_begin+1)&255) != color_ring_end)
    {
        if (vga_line/2 == 0)
        {
            uint16_t old_color = color_ring[color_ring_begin];
            draw_buffer[stripe*10] = old_color;
            draw_buffer[stripe*10+1] = old_color;
            draw_buffer[stripe*10+8] = old_color;
            draw_buffer[stripe*10+9] = old_color;
        }
        else if (vga_line == 240-1)
        {
            stripe = (stripe+1)%32;
            pop_color();
        }
        return;
    }
    // only one color remains
    uint16_t color = color_ring[color_ring_begin];
    draw_buffer[stripe*10] = RGB(255,255,255);
    draw_buffer[stripe*10+1] = RGB(255,255,255);
    if ((vga_line/2)%16 == 0 && vga_frame % 16 == 0)
        draw_buffer[stripe*10 + 2 + rand()%6] = color;
    draw_buffer[stripe*10+8] = RGB(255,255,255);
    draw_buffer[stripe*10+9] = RGB(255,255,255);
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
    if (press)
    {
        press = button_state();
        if (!press)
        {
            // button release, do something fancier for holding down the button
            if (music_on)
            {
                music_on = 0;
                message("pre-empted\n");
                push_color(RGB(255,150,100));
                f_close(&fat_file);
            }
            else
            {
                load_next_file_please = 1;
            }
            press_hold = 0;
        }
        else
        {
            // still pressing the button
            ++press_hold;
            const uint8_t max_color = 8*(music_on+1);
            color_ring[color_ring_end] =  ((rand()%max_color)<<10)|((rand()%max_color)<<5)|(rand()%max_color);
        }
        memset(buffer, 128, 2*len);
        return;
    }
    else
    {
        press = button_state();
        if (press)
        {
            message("got press %d?\n", press);
            memset(buffer, 128, 2*len);
            return;
        }
    }

    if (!music_on)
    {
        for (int i=0; i<len; i++)
            buffer[i] = gen_sample();
        return;
    }
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
            push_color(RGB(255,0,255));
            f_close(&fat_file);
            music_on = 0;
            return;
        }
        else if (bytes_read != bytes_to_read)
        {
            message("file ended\n");
            push_color(RGB(255,0,0));
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
