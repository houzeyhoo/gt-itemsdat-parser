/*+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
+-    c-deltaparser, copyright (c) 2021 houz                                                                 -+
+-    A parser for growtopia items.dat, written fully in C. Optimized to maximize efficiency.                -+
+-    Usage: .\c-deltaparser [output file name] (default is "itemsdat_parsed")                               -+
+-    For license, please read license.txt included in the project repo.                                     -+
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+*/

#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "itemsdat_read.h"

#define OUT_BUFFER_SIZE 1024*1024*3 /* 3 megabytes, way more than enough as of 2021 */

/*+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
+-                                             Misc Optimized Functions                                         +-
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

char* strcat_delim(char* dest, char* src, char delimiter)
{
    /* find null term, copy bytes, append delimiter & null term */
    while (*dest) dest++;
    while (*dest++ = *src++);
    *(dest - 1) = delimiter;
    *(dest) = '\0';
    return --dest;
}

/* itoa that writes straight to the buffer, adds a delimiter */
char* strcat_delim_int(char* dest, int value, char delimiter)
{
    /* absolute value */
    int n = value >= 0 ? value : -1 * value;

    /* find null terminator */
    while (*dest) dest++;

    int i = 0;
    while (n)
    {
        int r = n % 10;
        *(dest + i++) = '0' + r;
        n /= 10;
    }

    if (i == 0)
        *(dest + i++) = '0';

    if (value < 0)
        *(dest + i++) = '-';

    /* Reverse string */
    char* beg = dest;
    char* end = dest + i - 1;
    while (beg < end)
    {
        char tmp = *beg;
        *beg++ = *end;
        *end-- = tmp;
    }

    /* Append delimiter & null term */
    *(dest + i++) = delimiter;
    *(dest + i) = '\0';

    return (dest + i);
}

/*+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
+-                                               MAIN                                                             +-
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/
int main(int argc, char** argv)
{
    clock_t begin_clock = clock();
    /* Custom output filename */
    const char* out_filename = "itemsdat_parsed.txt";
    int minified = 0;

    for (int cur_arg = 1; cur_arg < argc; cur_arg++) {
        char* arg_name = argv[cur_arg];

        if (!strcmp(arg_name, "-o") || !strcmp(arg_name, "--output"))
        {
            if (++cur_arg > argc) {
                printf("No argument supplied for '%s'\n!", arg_name);
                return 1;
            }

            out_filename = argv[cur_arg];
            continue;
        }

        else if (!strcmp(arg_name, "-m") || !strcmp(arg_name, "--min") || !strcmp(arg_name, "--minified")) {
            minified = 1;
        }
        else {
            printf("Invalid argument detected: '%s'!\n", arg_name);
        }
    }

    FILE* f = fopen("items.dat", "rb");
    itemsdat itd;
    int res = itemsdat_read(f, &itd);
    switch (res)
    {
    case 1:
        printf("Missing items.dat file. Are you sure it's in the same directory as this program?");
        return 1;
    case 2:
        printf("Invalid version of items.dat.");
        return 1;
    case 3:
        printf("Corrupt or malformed items.dat.");
        return 1;
    }

    FILE* file_output = fopen(out_filename, "wb");
    if (file_output == NULL)
    {
        printf("Failed to open output file. Yikes. Perhaps the file name you passed is too long?\n");
        return 1;
    }
    
    char* buf_out = malloc(OUT_BUFFER_SIZE); 
    char* buf_ptr = buf_out;
    buf_out[0] = '\0';

    /* create whole output buffer */
    for (int i = 0; i < itd.item_count; i++)
    {
        buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].id, '|');
        if (!minified)
        {
            buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].properties, '|');
            buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].type, '|');
            buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].material, '|');
        }
        buf_ptr = strcat_delim(buf_ptr, itd.items[i].name, '|');
        if (!minified)
        {
            buf_ptr = strcat_delim(buf_ptr, itd.items[i].file_name, '|');
            buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].file_hash, '|');
            buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].visual_type, '|');
            buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].cook_time, '|');
            buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].tex_x, '|');
            buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].tex_y, '|');
            buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].storage_type, '|');
            buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].layer, '|');
            buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].collision_type, '|');
            buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].hardness, '|');
            buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].regen_time, '|');
            buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].clothing_type, '|');
            buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].rarity, '|');
            buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].max_hold, '|');
            buf_ptr = strcat_delim(buf_ptr, itd.items[i].alt_file_path, '|');
            buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].alt_file_hash, '|');
            buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].anim_ms, '|');
            if (itd.version >= 4)
            {
                buf_ptr = strcat_delim(buf_ptr, itd.items[i].pet_name, '|');
                buf_ptr = strcat_delim(buf_ptr, itd.items[i].pet_prefix, '|');
                buf_ptr = strcat_delim(buf_ptr, itd.items[i].pet_suffix, '|');
                if (itd.version >= 5)
                    buf_ptr = strcat_delim(buf_ptr, itd.items[i].pet_ability, '|');
            }
            buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].seed_base, '|');
            buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].seed_over, '|');
            buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].tree_base, '|');
            buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].tree_over, '|');
            buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].bg_col, '|');
            buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].fg_col, '|');
            buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].seed1, '|');
            buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].seed2, '|');
            buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].bloom_time, '|');
            if (itd.version >= 7)
            {
                buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].anim_type, '|');
                buf_ptr = strcat_delim(buf_ptr, itd.items[i].anim_string, '|');
            }
            if (itd.version >= 8)
            {
                buf_ptr = strcat_delim(buf_ptr, itd.items[i].anim_tex, '|');
                buf_ptr = strcat_delim(buf_ptr, itd.items[i].anim_string2, '|');
                buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].dlayer1, '|');
                buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].dlayer2, '|');
            }
            if (itd.version >= 9)
            {
                buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].properties2, '|');
            }
            if (itd.version >= 10)
            {
                buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].tile_range, '|');
                buf_ptr = strcat_delim_int(buf_ptr, itd.items[i].pile_range, '|');
            }
            if (itd.version >= 11)
            {
                buf_ptr = strcat_delim(buf_ptr, itd.items[i].custom_punch, '|');
            }
        }

        /* change last seperator to newline */
        *(buf_ptr) = '\n';
    }

    /* finally, write the output buffer */
    fwrite(buf_out, 1, strlen(buf_out), file_output);
    fclose(file_output);

    clock_t end_clock = clock();
    double time_spent = (double)(end_clock - begin_clock) / CLOCKS_PER_SEC;

    printf("Info: items.dat version: %d, item count: %d\n", itd.version, itd.item_count);
    printf("Success: wrote %d bytes to %s in %.4f seconds.\n", strlen(buf_out), out_filename, time_spent);
    return 0;
}