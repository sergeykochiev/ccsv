#ifndef CCRV
#define CCRV

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef const char* CELL;
typedef const char* ROW;

typedef enum {
    ASCII,
    UTF8,
    UTF16,
    ENCODING_COUNT
} Encoding;

// Main struct of the library, which corresponds to the state of a .csv file. Use "buffer" field to store parsed data (by default it is a char*)
typedef struct {
    size_t charsize;
    size_t columns;
    size_t rows;
    size_t *column_offsets;
    size_t row_size;
    size_t max_col_size;
    char *buffer;
    void *custom_buffer;
} CCSV;

// Type of a custom function pointer that is provided to ccsv_parse function
typedef int (*CCSV_Cell_Handler) (CCSV*, size_t, size_t, size_t, void*);

const size_t Encoding_Size[ENCODING_COUNT] = {
    [ASCII] = 1,
    [UTF8] = 8,
    [UTF16] = 16,
};

// Does basic initialization, essentially a simple helper function that does nothing (yet)
void ccsv_init(CCSV *ccsv, Encoding enc) {
    ccsv->charsize = Encoding_Size[enc];
    ccsv->max_col_size = 0;
    ccsv->row_size = 0;
    ccsv->columns = 0;
    ccsv->rows = 0;
}

// Default function. Get cell by it's coords (row and column). "dest" and "len" are output params, "len" is optional
int ccsv_get_cell(CCSV *c, size_t row, size_t col, CELL *dest, size_t *len) {
    if(col >= c->columns || row >= c->rows) {
        printf("CCSV: Index out of range\n");
        return 1;
    }
    if(len != NULL) {
        *len = c->column_offsets[col + 1] - c->column_offsets[col] - 1;
    }
    *dest = (char*)c->buffer + c->row_size * row + c->column_offsets[col];
    return 0;
}

// Default function. Get cell in row by column number. "dest" and "len" are output params, "len" is optional
int ccsv_get_row_cell(CCSV *c, ROW *row, size_t col, CELL *dest, size_t *len) {
    if(col >= c->columns) {
        printf("CCSV: Index out of range\n");
        return 1;
    }
    if(len != NULL) {
        *len = c->column_offsets[col + 1] - c->column_offsets[col] - 1;
    }
    *dest = (CELL)(*row + c->column_offsets[col]);
    return 0;
}

// Default function. Get row by it's number. "dest" is an output param
int ccsv_get_row(CCSV *c, size_t row, ROW *dest) {
    if(row >= c->rows) return 1;
    *dest = (char*)c->buffer + c->row_size * row;
    return 0;
}

// Frees the memory occupied by the CCSV struct
void ccsv_deinit(CCSV *ccsv) {
    free(ccsv->buffer);
    free(ccsv->column_offsets);
    free(ccsv->custom_buffer);
}

// Calculate max sizes for each columns, row size and absolute max size a column can be
int ccsv_calculate_sizes(const char fp[], CCSV *ccsv) {
    FILE *f = fopen(fp, "r");
    if(f == NULL) {
        return 1;
    }
    char c;
    int col = 0, offset = 0, ignore_comma = 0;
    ccsv->max_col_size = 0;
    ccsv->column_offsets = (size_t*)malloc(ccsv->columns++);
    ccsv->column_offsets[0] = 0;
    while(ferror(f) == 0) {
        c = fgetc(f);
        if(c == ',' && !ignore_comma) {
            offset++;
            col++;
            if(ccsv->columns == col) {
                ccsv->columns++;
                size_t *new_co = (size_t*)realloc(ccsv->column_offsets, sizeof(size_t) * ccsv->columns);
                if(new_co == NULL) {
                    printf("CCSV: Reallocation error\n");
                    return 1;
                }
                ccsv->column_offsets = new_co;
                ccsv->column_offsets[ccsv->columns - 1] = 0;
            }
            if(ccsv->column_offsets[col] < offset) {
                ccsv->column_offsets[col] = offset;
                if(ccsv->column_offsets[col] > ccsv->max_col_size) {
                    ccsv->max_col_size = ccsv->column_offsets[col];
                }
            }
        } else if(c == '\n' || feof(f)) {
            offset++;
            if(ccsv->row_size < offset) ccsv->row_size = offset;
            if(feof(f)) break;
            ccsv->rows++;
            col = 0;
            offset = 0;
        } else if(c == '"') {
            ignore_comma = !ignore_comma;
        } else {
            offset++;
        }
    }
    size_t *new_co = (size_t*)realloc(ccsv->column_offsets, sizeof(size_t) * ccsv->columns);
    if(new_co == NULL) {
        printf("CCSV: Reallocation error\n");
        return 1;
    }
    ccsv->column_offsets = new_co;
    ccsv->column_offsets[ccsv->columns] = ccsv->row_size;
    printf("CCSV: Calculated sizes: table is %lux%lu, row size is %llu, max col size is %llu\n", ccsv->rows, ccsv->columns, ccsv->row_size, ccsv->max_col_size);
    fclose(f);
    return 0;
}

// Parses given .csv filename, ccsv_init first. If you want to handle each cell using custom logic, provide your custom function pointer to "custom_f", but then all the default functions won't work
int ccsv_parse(const char fp[], CCSV *ccsv, CCSV_Cell_Handler custom_f, void* param) {
    FILE *f = fopen(fp, "r");
    if(f == NULL) {
        return 1;
    }
    int using_defaults = custom_f == NULL;
    if(!using_defaults) {
        ccsv->buffer = (char*)malloc(sizeof(char) * (ccsv->max_col_size + 1));
    }
    char c;
    int col = 0, row = 0, cur = 0, num = 0, ignore_comma = 0, left = 0;
    while(ferror(f) == 0) {
        c = fgetc(f);
        if(c == ',' && !ignore_comma || c == '\n' || feof(f)) {
            ccsv->buffer[cur] = '\0';
            if(using_defaults) {
                left = ccsv->column_offsets[col + 1] - ccsv->column_offsets[col] - num;
            } else {
                if(custom_f(ccsv, row, col, cur + 1, param)) return 1;
            }
            // memset(ccsv->buffer + cur + 1, 0, left - 1);
            cur = (cur + left) * (int)using_defaults;
            num = 0;
            col++;
            if(feof(f)) break;
            if(c == '\n' || feof(f)) {
                row++;
                if(row % 1000 == 0) printf("\rCCSV: Parsed %d rows", row);
                col = 0;
            }
        } else if(c == '"') {
            ignore_comma = !ignore_comma;
        } else {
            ccsv->buffer[cur] = c;
            cur++;
            num++;
        }
    }
    if(ferror(f)) {
        printf("CCSV: Reading error\n");
        return 1;
    }
    if(using_defaults) ccsv->buffer[ccsv->row_size * ccsv->rows] = '\0';
    printf("\rCCSV: Parsing finished, parsed %d rows in total\n", row);
    return 0;
}

// Wrapper around ccsv_calculate_sizes and ccsv_parse. Use it in case if you want default behavior. By default, each cell is stored as cstr in a buffer and is accessed by using pointer arithmetics
int ccsv_default_parse(const char fp[], CCSV *ccsv) {
    if(ccsv_calculate_sizes(fp, ccsv)) return 1;
    ccsv->buffer = (char*)malloc(ccsv->row_size * ccsv->rows + 1);
    return ccsv_parse(fp, ccsv, NULL, NULL);
}

#endif
