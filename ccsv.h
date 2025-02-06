#ifndef CCRV
#define CCRV

#include <stddef.h>

typedef const char* CELL;
typedef const char* ROW;

typedef enum {
    ASCII,
    UTF8,
    UTF16,
    ENCODING_COUNT
} Encoding;

typedef struct {
    size_t charsize;
    size_t columns;
    size_t rows;
    size_t *column_offsets;
    size_t row_size;
    char *buffer;
} CCSV;

void ccsv_init(CCSV *ccsv, Encoding enc);

// Parses given .csv filename, ccsv_init first
int ccsv_parse(const char fp[], CCSV *ccsv);

// Get cell by it's coords (row and column). "dest" and "len" are output params, "len" is optional
int ccsv_get_cell(CCSV *c, size_t row, size_t col, CELL *dest, size_t *len);

// Get row by it's number. "dest" is an output param
int ccsv_get_row(CCSV *c, size_t row, ROW *dest);

// Get cell in row by column number. "dest" and "len" are output params, "len" is optional
int ccsv_get_row_cell(CCSV *c, ROW *row, size_t col, CELL *dest, size_t *len);

void ccsv_deinit(CCSV *ccsv);

#endif
