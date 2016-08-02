#include "bitbox.h" 
#include "font.h"
#include <stdlib.h> // rand
#include <math.h>
#include <stdio.h>
#include <string.h>

uint64_t song_pos CCM_MEMORY;
uint16_t color_ring[32] CCM_MEMORY;
uint8_t color_ring_end CCM_MEMORY;
uint8_t message_ring[16][32] CCM_MEMORY;
uint8_t message_ring_end CCM_MEMORY;
uint8_t stripe CCM_MEMORY;

static inline void push_color(uint16_t c)
{
    #ifndef NO_VGA
    if (++color_ring_end >= 32)
        color_ring_end = 0;
    color_ring[color_ring_end] = c;
    #endif
}

static inline void push_message(const char *c)
{
    #ifndef NO_VGA
    if (++message_ring_end >= 16)
        message_ring_end = 0;
    strncpy((char *)message_ring[message_ring_end], c, 31);
    message_ring[message_ring_end][31] = 0; // ensure null termination
    #endif
}

uint8_t music_on CCM_MEMORY;
uint32_t press_hold CCM_MEMORY;

#include "fatfs/ff.h"

#define MAX_READ_BYTES 512

FATFS fat_fs;
FIL fat_file;
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
        push_message("error opening SONGNOSE.TXT");
        push_color(RGB(255,150,0));
        return;
    }
    if (f_read(&fat_file, &file_index, sizeof(file_index), &bytes_get) != FR_OK ||
        bytes_get != sizeof(file_index))
    {
        push_message("error reading SONGNOSE.TXT");
        push_color(RGB(255,0,0));
        file_index = 0;
    }
    push_message(" file_index: ");
    int digits = file_index;
    message_ring[message_ring_end][21] = 0;
    for (int i=20; i>=14; --i)
    {
        message_ring[message_ring_end][i] = digits%10;
        digits/=10;
    }
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
        push_message("bad dir\n");
        return;
    }
    int looped = 0;
    for (file_count=0;;) 
    {
        res = f_readdir(&dir, &fno);                   /* Read a directory item */
        fn = fno.fname;
        if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
        /* Ignore dot entry and dirs */
        if (fn[0] == '.') continue;
        push_message("check: ");
        strcpy((char *)&message_ring[message_ring_end][7], fn);
        if (fno.fattrib & AM_DIR) 
        {
            push_message(" was a directory\n");
            continue;
        }
        // check extension : only keep .w8v
        if (strstr(fn,".W8V") || strstr(fn,".w8v")) // search ignoring case
        {
            push_message(" got w8v.");
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
    if (file_count > MAX_FILES)
    {
        message(" we were aiming at index [%d, +64)\n", file_index);
        file_count = MAX_FILES;
    }
    set_file_index(file_index + file_count/2);
    if (!file_count)
        return;
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
    song_pos = 0;

    music_on = 0;
    if (!mount_ok)
    {
        push_color(RGB(255,255,255));
        push_message("no mount");
        return;
    }
    if (!file_count)
    {
        push_color(RGB(255,255,0));
        push_message("no files");
        return;
    }
    
    if (file_index >= file_count)
    {
        // stop after reaching end of playlist
        push_color(RGB(255,0,0));
        // but reset in case pressing button again
        file_index = 0;
        return;
    }
    push_color(RGB(0,255,255));
    message("opening the %d music, %s: ", file_index, filenames[file_index]);
    push_message("file: ");
    strcpy((char *)&message_ring[message_ring_end][6], filenames[file_index]);
    if (f_open(&fat_file, filenames[file_index++], FA_READ | FA_OPEN_EXISTING) != FR_OK)
    {
        push_message(" NO OPEN!\n");
        push_color(RGB(255,0,255));
        return;
    }
    push_color(RGB(0,255,0));
    push_message(" ok!");
    music_on = 1;
}

void game_init()
{
    message_ring_end = 15;
    font_init();
    stripe = 0;
    mount_ok = 0;
    music_on = 0;
        
    if (f_mount(&fat_fs, "", 1) != FR_OK)
    {
        push_color(RGB(255,255,255));
        push_message("mount failure");
        return;
    }
    push_color(RGB(100,100,100));
    push_message("mount ok");
    mount_ok = 255;
    list_music();
}

void game_frame()
{
    if (load_next_file_please)
    {
        push_color(RGB(100,100,100));
        push_message("requesting next file");
        load_next_file();
    }
}

#ifndef NO_VGA
void graph_frame() 
{
}

void graph_line() 
{
    static int line_length = 0;
    if (vga_odd)
        return;
    uint16_t color = color_ring[color_ring_end];
    uint16_t stripe = 10*color_ring_end;
    draw_buffer[stripe] = RGB(255,255,255);
    if ((vga_line/2)%16 == 0 && vga_frame % 16 == 0)
        draw_buffer[stripe + 1 + rand()%8] = color;
    draw_buffer[stripe+9] = RGB(255,255,255);

    if (vga_line < 48 || vga_line >= 48+16*10)
        return;
    int line = (vga_line-48);
    int internal_line = line%10;
    line /= 10;
    if (internal_line < 8)
        line_length = font_render_line_doubled(message_ring[line], 16, internal_line, 
            line == message_ring_end ? RGB(255,0,0) : 65535, 
        0);
    else
        memset(&draw_buffer[16], 0, line_length*10*2);
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
    static uint32_t press=0;
    static uint32_t seek_forward=0;
    if (press)
    {
        press = button_state();
        if (!press)
        {
            // button release, do something fancier for holding down the button
            message("got hold %u\n", press_hold);
            if (music_on)
            {
                if (press_hold < 10)
                {
                    music_on = 0;
                    message("pre-empted\n");
                    push_color(RGB(255,150,100));
                    f_close(&fat_file);
                    load_next_file_please = 1;
                }
                else
                {
                    push_color(RGB(200,100,0));
                    seek_forward = 512*(press_hold + press_hold*press_hold);
                }
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
            color_ring[(color_ring_end-1)&255] =  ((rand()%max_color)<<10)|((rand()%max_color)<<5)|(rand()%max_color);
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
            ++color_ring_end;
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
    if (seek_forward)
    {
        song_pos += seek_forward; 
        f_lseek(&fat_file, song_pos);
        seek_forward = 0;
    }
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
            push_message(" file not read well");
            push_color(RGB(255,0,255));
            f_close(&fat_file);
            music_on = 0;
            return;
        }
        else if (bytes_read != bytes_to_read)
        {
            push_message(" file ended.");
            push_color(RGB(255,0,0));
            buffer += bytes_to_read/2;
            while (buffer < buffer_end)
                *buffer++ = 128;
            f_close(&fat_file);
            load_next_file_please = 1;
            return;
        }
        buffer += bytes_to_read/2;
    }
    song_pos += 2*len;
    return;
}
