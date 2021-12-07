/*+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
+- itemsdat_read, copyright (c) 2021 houz                                                              	        +-
+- A parser-library for Growtopia's items.dat, written fully in C & embeddable in C/C++ code.                   +-
+- For license, please read license.txt included in the project repo.                                           +-
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-*/

#ifndef ITEMSDAT_READ_H
#define ITEMSDAT_READ_H

#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define _ITEMSDAT_MAX_VERSION 14 /* Maximum items.dat version supported by this parser */

#define _ITEMSDAT_SECRET "PBG892FXX982ABC*" /* Magic string for decrypting item names */
#define _ITEMSDAT_SECRET_LEN 16 /* Length of magic string for decrypting item names */

/* item struct */
typedef struct 
{
    int id;
    unsigned short properties;
    unsigned char type;
    char material;
    /* Warning: name field in DB v2 is plaintext, v3and forwards is encrypted using XOR with a shared key. */
    char* name;
    char* file_name;
    int file_hash;
    char visual_type;
    int cook_time;
    char tex_x;
    char tex_y;
    char storage_type;
    char layer;
    char collision_type;
    char hardness;
    int regen_time;
    char clothing_type;
    short rarity;
    unsigned char max_hold;
    char* alt_file_path;
    int alt_file_hash;
    int anim_ms;

    /* DB v4 - Pet name / prefix / suffix */
    char* pet_name;
    char* pet_prefix;
    char* pet_suffix;

    /* DB v5 - Pet ability */
    char* pet_ability;

    /* Back to normal */
    char seed_base;
    char seed_over;
    char tree_base;
    char tree_over;
    int bg_col;
    int fg_col;

    /* These two seed values are always zero and they're unused. */
    short seed1;
    short seed2;

    int bloom_time;

    /* DB v7 - animation type. */
    int anim_type;

    /* DB v8 - animation stuff continued. */
    char* anim_string;
    char* anim_tex;
    char* anim_string2;
    int dlayer1;
    int dlayer2;

    /* DB v9 - second properties enum, ran out of space for first. */
    unsigned int properties2;

    /* DB v10 - tile/pile range - used by extractors and such. */
    int tile_range;
    int pile_range;

    /* DB v11 - custom punch, a failed experiment that doesn't seem to work ingame. */
    char* custom_punch;

    /* DB v12,v13,v14 - some flags at end of item */
} itemsdat_item;

typedef struct 
{
    short version;
    int item_count;

    itemsdat_item* items;
} itemsdat;

/* _read_byte - reads 1 byte from buf, copies to dest.Increments bp(buffer position) by 1. */
static inline void _read_byte(char* buf, unsigned int* bp, void* dest)
{
    memcpy(dest, buf + *bp, 1);
    *bp += 1;
}

/* _read_short - reads 2 bytes from buf, copies to dest. Increments bp (buffer position) by 2. */
static inline void _read_short(char* buf, unsigned int* bp, void* dest)
{
    memcpy(dest, buf + *bp, 2);
    *bp += 2;
}

/* _read_int32 - reads 4 bytes from buf, copies to dest. Increments bp (buffer position) by 4. */
static inline void _read_int32(char* buf, unsigned int* bp, void* dest)
{
    memcpy(dest, buf + *bp, 4);
    *bp += 4;
}

/* _read_str - reads chars from buf, copies to *dest. Increments bp by string size. */
static inline void _read_str(char* buf, unsigned int* bp, char** dest)
{
    unsigned int bp_in = *bp;
    unsigned short len = 0;
    _read_short(buf, bp, &len);
    *dest = malloc(len + 2); //+terminator +seperator

    memset(*dest + len, '\0', 1);
    strncpy(*dest, buf + *bp, len);
    *bp += len;
}

/* _read_str - reads chars from buf, copies to *dest. Increments bp by string size. Used for encrypted item names. */
static inline void _read_str_encr(char* buf, unsigned int* bp, char** dest, int itemID)
{
    unsigned int bp_in = *bp;
    // Reverse of MemorySerializeStringEncrypted from Proton SDK

    unsigned short len = 0;
    _read_short(buf, bp, &len);
    *dest = malloc(len + 2); //+terminator +seperator
    memset(*dest + len, '\0', 1);

    itemID %= _ITEMSDAT_SECRET_LEN;
    for (unsigned int i = 0; i < len; i++)
    {
        char y = buf[*bp + i] ^ _ITEMSDAT_SECRET[itemID++];
        memcpy(*dest + i, &y, 1);
        if (itemID >= _ITEMSDAT_SECRET_LEN) itemID = 0;
    }
    *bp += len;
}

/* Reads the items.dat file, stores everything in the itemsdat struct. The FILE pointer must refer to an opened items.dat file in read-binary mode.
Return codes: 0 - all is good, 1 - invalid FILE*, 2 - bad items.dat version, 3 - malformed/corrupted items.dat.
The itemsdat->items array requires to be freed once the user is done using it. You may use destroy_itemsdat() for clean-up purposes. */
static inline int itemsdat_read(FILE* f, itemsdat* itemsdat)
{
    if (f == NULL) return 1;
    if (ferror(f)) return 1;

    /* get file size */
    fseek(f, 0, SEEK_END);
    unsigned int file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    /* allocate temporary buffer and read file contents to it */
    char* buf = (char*)malloc(file_size);
    unsigned int buf_pos = 0;
    fread(buf, 1, file_size, f);

    _read_short(buf, &buf_pos, &itemsdat->version);
    _read_int32(buf, &buf_pos, &itemsdat->item_count);

    /* check if items.dat version is valid */
    if (itemsdat->version > _ITEMSDAT_MAX_VERSION)
    {
        free(buf);
        return 2;
    }

    /* allocate the items array */
    itemsdat->items = calloc(itemsdat->item_count, sizeof(itemsdat_item));

    /* parse every item appropriately */
    for (int i = 0; i < itemsdat->item_count; i++)
    {
        itemsdat_item item;
        _read_int32(buf, &buf_pos, &item.id);

        /* if this condition passes, then the items.dat is corrupted */
        if (item.id != i)
        {
            free(buf);
            return 3;
        }
        _read_short(buf, &buf_pos, &item.properties);
        _read_byte(buf, &buf_pos, &item.type);
        _read_byte(buf, &buf_pos, &item.material);

        if (itemsdat->version >= 3)
            _read_str_encr(buf, &buf_pos, &item.name, item.id); /* encrypted on itemsdat ver > 3 */
        else
            _read_str(buf, &buf_pos, &item.name); /* otherwise normal string */

        _read_str(buf, &buf_pos, &item.file_name);
        _read_int32(buf, &buf_pos, &item.file_hash);
        _read_byte(buf, &buf_pos, &item.visual_type);
        _read_int32(buf, &buf_pos, &item.cook_time);
        _read_byte(buf, &buf_pos, &item.tex_x);
        _read_byte(buf, &buf_pos, &item.tex_y);
        _read_byte(buf, &buf_pos, &item.storage_type);
        _read_byte(buf, &buf_pos, &item.layer);
        _read_byte(buf, &buf_pos, &item.collision_type);
        _read_byte(buf, &buf_pos, &item.hardness);
        _read_int32(buf, &buf_pos, &item.regen_time);
        _read_byte(buf, &buf_pos, &item.clothing_type);
        _read_short(buf, &buf_pos, &item.rarity);
        _read_byte(buf, &buf_pos, &item.max_hold);
        _read_str(buf, &buf_pos, &item.alt_file_path);
        _read_int32(buf, &buf_pos, &item.alt_file_hash);
        _read_int32(buf, &buf_pos, &item.anim_ms);

        if (itemsdat->version >= 4)
        {
            _read_str(buf, &buf_pos, &item.pet_name);
            _read_str(buf, &buf_pos, &item.pet_prefix);
            _read_str(buf, &buf_pos, &item.pet_suffix);

            if (itemsdat->version >= 5)
                _read_str(buf, &buf_pos, &item.pet_ability);
        }

        _read_byte(buf, &buf_pos, &item.seed_base);
        _read_byte(buf, &buf_pos, &item.seed_over);
        _read_byte(buf, &buf_pos, &item.tree_base);
        _read_byte(buf, &buf_pos, &item.tree_over);
        _read_int32(buf, &buf_pos, &item.bg_col);
        _read_int32(buf, &buf_pos, &item.fg_col);
        _read_short(buf, &buf_pos, &item.seed1);
        _read_short(buf, &buf_pos, &item.seed2);
        _read_int32(buf, &buf_pos, &item.bloom_time);

        if (itemsdat->version >= 7)
        {
            _read_int32(buf, &buf_pos, &item.anim_type);
            _read_str(buf, &buf_pos, &item.anim_string);
        }
        if (itemsdat->version >= 8)
        {
            _read_str(buf, &buf_pos, &item.anim_tex);
            _read_str(buf, &buf_pos, &item.anim_string2);
            _read_int32(buf, &buf_pos, &item.dlayer1);
            _read_int32(buf, &buf_pos, &item.dlayer2);
        }
        if (itemsdat->version >= 9)
        {
            _read_int32(buf, &buf_pos, &item.properties2);
            buf_pos += 60;
            /* places where buf_pos is manually incremented contain
            fields that are considered irrelevant... or I don't know
            yet what they do! 
            possible todo: figure these out. */
        }
        if (itemsdat->version >= 10)
        {
            _read_int32(buf, &buf_pos, &item.tile_range);
            _read_int32(buf, &buf_pos, &item.pile_range);
        }
        if (itemsdat->version >= 11)
        {
            _read_str(buf, &buf_pos, &item.custom_punch);
        }
        if (itemsdat->version >= 12)
        {
            buf_pos += 13;
        }
        if (itemsdat->version >= 13)
        {
            buf_pos += 4;
        }
        if (itemsdat->version >= 14)
        {
            buf_pos += 4;
        }

        itemsdat->items[i] = item;
    }
    free(buf);
    return 0;
}

/* Clean-up the struct. */
static inline void itemsdat_destroy(itemsdat* itemsdat)
{
    free(itemsdat->items);
    itemsdat->items = NULL;
    itemsdat->item_count = 0;
    itemsdat->version = 0;
}

#endif /* ITEMSDAT_READ_H */