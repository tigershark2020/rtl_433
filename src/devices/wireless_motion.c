#include "decoder.h"
#include <stdlib.h>
#include "fatal.h"

static uint8_t bcd_decode8(uint8_t x)
{
    return ((x & 0xF0) >> 4) * 10 + (x & 0x0F);
}

struct flex_map {
    unsigned key;
    const char *val;
};

#define GETTER_MAP_SLOTS 16

struct flex_get {
    unsigned bit_offset;
    unsigned bit_count;
    unsigned long mask;
    const char *name;
    struct flex_map map[GETTER_MAP_SLOTS];
};

#define GETTER_SLOTS 8

struct flex_params {
    char *name;
    unsigned min_rows;
    unsigned max_rows;
    unsigned min_bits;
    unsigned max_bits;
    unsigned min_repeats;
    unsigned max_repeats;
    unsigned invert;
    unsigned reflect;
    unsigned unique;
    unsigned count_only;
    unsigned match_len;
    bitrow_t match_bits;
    unsigned preamble_len;
    bitrow_t preamble_bits;
    struct flex_get getter[GETTER_SLOTS];
};

static void wireless_motion_print_row_bytes(char *row_bytes, uint8_t *bits, int num_bits)
{
    row_bytes[0] = '\0';
    // print byte-wide
    for (int col = 0; col < (num_bits + 7) / 8; ++col) {
        sprintf(&row_bytes[2 * col], "%02x", bits[col]);
    }
    // remove last nibble if needed
    row_bytes[2 * (num_bits + 3) / 8] = '\0';
}

static void wireless_motion_render_getters(data_t *data, uint8_t *bits, struct flex_params *params)
{
    // add a data line for each getter
    for (int g = 0; g < GETTER_SLOTS && params->getter[g].bit_count > 0; ++g) {
        struct flex_get *getter = &params->getter[g];
        unsigned long val;
        if (getter->mask)
            val = compact_number(bits, getter->bit_offset, getter->mask);
        else
            val = extract_number(bits, getter->bit_offset, getter->bit_count);
        int m;
        for (m = 0; getter->map[m].val; m++) {
            if (getter->map[m].key == val) {
                data_append(data,
                        getter->name, "", DATA_STRING, getter->map[m].val,
                        NULL);
                break;
            }
        }
        if (!getter->map[m].val) {
            data_append(data,
                    getter->name, "", DATA_INT, val,
                    NULL);
        }
    }
}

static void set_row_bytes(char *row_bytes, uint8_t *bits, int num_bits)
{
    row_bytes[0] = '\0';
    // print byte-wide
    for (int col = 0; col < (num_bits + 7) / 8; ++col) {
        sprintf(&row_bytes[2 * col], "%02x", bits[col]);
    }
    // remove last nibble if needed
    row_bytes[2 * (num_bits + 3) / 8] = '\0';
}

static int wireless_motion_callback(r_device *decoder, bitbuffer_t *bitbuffer)
{
    if(bitbuffer->num_rows > 10) {

        char row_bytes[BITBUF_COLS * 2 + 1];
        set_row_bytes(row_bytes, bitbuffer->bb[2], bitbuffer->bits_per_row[2]);

        if(strlen(row_bytes) > 3)
        {
            data_t *data;
            data = data_make(
            "model",    "",  DATA_STRING, "Wireless-Motion",
            "code",     "",  DATA_STRING, row_bytes,
            NULL);

            decoder_output_data(decoder, data);
        }

        return 1;
    }
    else{
        return DECODE_ABORT_EARLY;
    }
}

static char *output_fields[] = {
        "model",
        "code",
        NULL};

r_device wireless_motion = {
        .name        = "Wireless Motion Sensor",
        .modulation  = OOK_PULSE_PWM,
        .short_width = 252,
        .long_width  = 768,
        .gap_limit   = 792,
        .tolerance   = 200,
        .reset_limit = 4168,
        .decode_fn   = &wireless_motion_callback,
        .disabled    = 0,
        .fields      = output_fields
};
