#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "ccsv.h"

typedef struct {
    unsigned int id;
    unsigned int age;
    char *gender;
    unsigned int income;
    char *education;
    char *region;
    char *loyalty_status;
    char *purchase_frequency;
    unsigned int purchase_amount;
    char *product_category;
    unsigned int promotion_usage;
    unsigned int satisfaction_score;
} CSV_Data_Row;

int custom_f(CCSV *c, size_t row, size_t col, size_t len, void *param) {
    #define list ((CSV_Data_Row*)c->custom_buffer)[row]
    switch(col) {
        case 0: list.id = atoi(c->buffer); break;
        case 1: list.age = atoi(c->buffer); break;
        case 2: memcpy_s(list.gender, len, c->buffer, len); break;
        case 3: list.income = atoi(c->buffer); break;
        case 4: memcpy_s(list.education, len, c->buffer, len); break;
        case 5: memcpy_s(list.region, len, c->buffer, len); break;
        case 6: memcpy_s(list.loyalty_status, len, c->buffer, len); break;
        case 7: memcpy_s(list.purchase_frequency, len, c->buffer, len); break;
        case 8: list.purchase_amount = atoi(c->buffer); break;
        case 9: memcpy_s(list.product_category, len, c->buffer, len); break;
        case 10: list.promotion_usage = atoi(c->buffer); break;
        case 11: list.satisfaction_score = atoi(c->buffer); break;
    }
    #undef list
    return 0;
}

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("Provide .csv!\n");
        return 1;
    }

    // init
    CCSV ccsv;
    ccsv_init(&ccsv, ASCII);

    // DEFAULT

    // parse into structured buffer (default method)
    ccsv_default_parse(argv[1], &ccsv);

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

    // CUSTOM

    ccsv_init(&ccsv, ASCII);

    // calculatie sizes "by hand"
    ccsv_calculate_sizes(argv[1], &ccsv);

    // allocate an array of structs to store data from .csv
    ccsv.custom_buffer = calloc(ccsv.rows + 1, sizeof(CSV_Data_Row));

    // passing custom_f function to ccsv_parse here so it will use it when handling cell values
    ccsv_parse(argv[1], &ccsv, custom_f, NULL);

    printf("row 5 column \"age\" value is: %d\n", ((CSV_Data_Row*)ccsv.custom_buffer)[4].age);

    ccsv_deinit(&ccsv);
    return 0;
}