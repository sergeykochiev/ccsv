#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "ccrv.h"

const size_t Encoding_Size[ENCODING_COUNT] = {
    [ASCII] = 1,
    [UTF8] = 8,
    [UTF16] = 16,
};

void ccsv_init(CCSV *ccsv, Encoding enc) {
    ccsv->charsize = Encoding_Size[enc];
}

int ccsv_get_cell(CCSV *c, size_t row, size_t col, CELL *dest, size_t *len) {
    if(col >= c->columns || row >= c->rows) {
        printf("Index out of range\n");
        return 1;
    }
    if(len != NULL) {
        *len = c->column_offsets[col + 1] - c->column_offsets[col] - 1;
    }
    *dest = c->buffer + c->row_size * row + c->column_offsets[col];
    return 0;
}

int ccsv_get_row_cell(CCSV *c, ROW *row, size_t col, CELL *dest, size_t *len) {
    if(col >= c->columns) {
        printf("Index out of range\n");
        return 1;
    }
    if(len != NULL) {
        *len = c->column_offsets[col + 1] - c->column_offsets[col] - 1;
    }
    *dest = (CELL)(*row + c->column_offsets[col]);
    return 0;
}

int ccsv_get_row(CCSV *c, size_t row, ROW *dest) {
    if(row >= c->rows) return 1;
    *dest = c->buffer + c->row_size * row;
    return 0;
}

void ccsv_deinit(CCSV *ccsv) {
    free(ccsv->buffer);
    free(ccsv->column_offsets);
}

int _ccsv_find_offsets(CCSV *ccsv, FILE *f) {
    char c;
    int col = 0, offset = 0;
    ccsv->column_offsets = malloc(ccsv->columns++);
    ccsv->column_offsets[0] = 0;
    int ignore_comma = 0;
    while(!ferror(f)) {
        c = fgetc(f);
        if(c == ',' && !ignore_comma) {
            offset++;
            col++;
            if(ccsv->columns == col) {
                ccsv->columns++;
                size_t *new_co = realloc(ccsv->column_offsets, sizeof(size_t) * ccsv->columns);
                if(new_co == NULL) {
                    printf("Reallocation error\n");
                    return 1;
                }
                ccsv->column_offsets = new_co;
                ccsv->column_offsets[ccsv->columns - 1] = 0;
            }
            if(ccsv->column_offsets[col] < offset) ccsv->column_offsets[col] = offset;
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
    size_t *new_co = realloc(ccsv->column_offsets, sizeof(size_t) * ccsv->columns);
    if(new_co == NULL) {
        printf("Reallocation error\n");
        return 1;
    }
    ccsv->column_offsets = new_co;
    ccsv->column_offsets[ccsv->columns] = ccsv->row_size;
    return 0;
}

int ccsv_parse(const char fp[], CCSV *ccsv) {
    FILE *f = fopen(fp, "r");
    if(f == NULL) {
        return 1;
    }
    _ccsv_find_offsets(ccsv, f);
    fclose(f);
    f = fopen(fp, "r");
    if(f == NULL) {
        return 1;
    }
    ccsv->buffer = malloc(ccsv->row_size * ccsv->rows + 1);
    char c;
    int col = 0, row = 0, cur = 0, num = 0, ignore_comma = 0, left;
    while(!ferror(f)) {
        c = fgetc(f);
        if(c == ',' && !ignore_comma || c == '\n' || feof(f)) {
            ccsv->buffer[cur + 1] = '\0';
            left = ccsv->column_offsets[col + 1] - ccsv->column_offsets[col] - num;
            printf("%d %d\n", row, col);
            // memset(ccsv->buffer + cur + 1, 0, left - 1);
            cur += left;
            num = 0;
            col++;
            if(c == '\n' || feof(f)) {
                row++;
                col = 0;
            }
            if(feof(f)) break;
        } else if(c == '"') {
            ignore_comma = !ignore_comma;
        } else {
            ccsv->buffer[cur] = c;
            cur++;
            num++;
        }
    }
    if(ferror(f)) {
        printf("Reading error\n");
        return 1;
    }
    ccsv->buffer[ccsv->row_size * ccsv->rows] = '\0';
    printf("Parsing finished, size: %lu rows and %lu cols\n", ccsv->rows, ccsv->columns);
    return 0;
}

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("Provide .csv!\n");
        return 1;
    }

    // init and parse
    CCSV ccsv;
    ccsv_init(&ccsv, ASCII);
    ccsv_parse(argv[1], &ccsv);

    // get cell by its coords
    CELL cell;
    ccsv_get_cell(&ccsv, 2348, 10, &cell, NULL);
    printf("row 2348 col 10 is \"%s\"\n", cell);

    // get row and iterate thought it's cells
    ROW dest_row;
    ccsv_get_row(&ccsv, 2348, &dest_row);
    for(int i = 0; i < ccsv.columns; i++) {
        ccsv_get_row_cell(&ccsv, &dest_row, i, &cell, NULL);
        printf("row 2348 col %d is \"%s\"\n", i, cell);
    }

    // free resources
    ccsv_deinit(&ccsv);
    return 0;
}
