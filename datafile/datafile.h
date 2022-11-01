#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "../store/graph_struct.h"

#ifndef LLP_LAB1_DATAFILE_H
#define LLP_LAB1_DATAFILE_H

typedef struct {
    FILE *file;
    control_block *ctrl_block;
};

struct relation_info {
    bool has_relation;
    char rel_name[RELATION_NAME_SIZE];
};

typedef struct relation_info relation_info;

#endif //LLP_LAB1_DATAFILE_H
