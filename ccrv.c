#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    ASCII,
    UTF8,
    UTF16,
    ENCODING_COUNT
} Encoding;

const size_t Encoding_Size[ENCODING_COUNT] = {
    [ASCII] = 1,
    [UTF8] = 8,
    [UTF16] = 16,
};

typedef struct {
    size_t charsize;
    size_t columns;
    size_t rows;
    size_t *column_offsets;
    size_t row_size;
    char *buffer;
} CCSV;

void ccsv_init(CCSV *ccsv, Encoding enc) {
    ccsv->charsize = Encoding_Size[enc];
}

int ccsv_get_by_coord(CCSV *c, size_t row, size_t col, char **dest, size_t *len) {
    if(col >= c->columns || row >= c->rows) return 1;
    *len = c->column_offsets[col + 1] - c->column_offsets[col] - 1;
    *dest = c->buffer + c->row_size * row + c->column_offsets[col];
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
            if(ccsv->column_offsets[col] < offset + 1) ccsv->column_offsets[col] = offset + 1;
        } else if(c == '\n' || feof(f)) {
            if(ccsv->row_size < offset + 1) ccsv->row_size = offset + 1;
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
    ccsv->row_size += ccsv->columns - 1;
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
    int col = 0, row = 0, cur = 0, relcur = 0, ignore_comma = 0, left;
    while(!ferror(f)) {
        c = fgetc(f);
        if(c == ',' && !ignore_comma || c == '\n' || feof(f)) {
            cur++;
            ccsv->buffer[cur] = '\0';
            left = ccsv->column_offsets[col + 1] - ccsv->column_offsets[col] - relcur - 1;
            // memset(ccsv->buffer + cur + 1, 0, left - 1);
            cur += left;
            relcur = 0;
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
            relcur++;
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

int main(void) {
    CCSV ccsv;
    ccsv_init(&ccsv, ASCII);
    ccsv_parse("example.csv", &ccsv);
    char *dest;
    size_t len;
    ccsv_get_by_coord(&ccsv, 2348, 3, &dest, &len);
    char *value = malloc(len + 1);
    memcpy(value, dest, len);
    value[len] = '\0';
    printf("row 2348 col 3 is \"%s\"\n", value);
    ccsv_deinit(&ccsv);
    free(value);
}
