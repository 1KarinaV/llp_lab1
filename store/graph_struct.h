#ifndef LLP_LAB1_GRAPH_STRUCT_H
#define LLP_LAB1_GRAPH_STRUCT_H

#include <stddef.h>
#include <stdint.h>
#include "cells.h"

#define BLOCK_SIZE 1024
#define LABELS_IN_BLOCK ((BLOCK_SIZE-sizeof(block_data))/sizeof(label_cell))
#define ATTRIBUTES_IN_BLOCK ((BLOCK_SIZE-sizeof(block_data))/sizeof(attribute_cell))
#define RELATIONS_IN_BLOCK ((BLOCK_SIZE-sizeof(block_data))/sizeof(relation_cell))
#define NODES_IN_BLOCK ((BLOCK_SIZE-sizeof(block_data)-sizeof(int32_t))/sizeof(node_cell))
#define NODES_IN_CONTROL_BLOCK ((BLOCK_SIZE-sizeof(block_data)-7*sizeof(int32_t)-5*sizeof(int16_t))/sizeof(node_cell))

#pragma pack(push, 1)

typedef enum TYPE {
   CONTROL=-1,
    NODE=-2,
    LABEL=-3,
    ATTRIBUTE=-4,
    RELATION=-5,
    STRING=-6
} block_types;

typedef enum TYPE TYPE;

typedef union {
    TYPE type;
    int32_t next_empty_block;
} block_data;

typedef struct {
    block_data b_data;
    char reserved [1020];
}block;

typedef struct {
    block_data b_data;
    label_cell labels[LABELS_IN_BLOCK];
}label_block;

typedef struct {
    block_data b_data;
    char reserved[12];
    attribute_cell attributes[ATTRIBUTES_IN_BLOCK];
}attribute_block;

typedef struct {
    block_data b_data;
    char reserved [12];
    relation_cell relations[RELATIONS_IN_BLOCK];
}relation_block;

typedef struct {
    block_data b_data;
    char data[1020];
}str_block;

typedef struct {
    block_data b_data;
    int32_t prev_block;
    char reserved[9];
    node_cell nodes[NODES_IN_BLOCK];
}node_block;

typedef struct {
    block_data b_data;
    int32_t prev_node_block;
    int32_t empty_block;
    int32_t fragmented_node_block;
    int16_t empty_node_number;
    int32_t fragmented_label_block;
    int16_t empty_label_number;
    int32_t fragmented_attribute_block;
    int16_t empty_attribute_number;
    int32_t fragmented_relation_block;
    int16_t empty_relation_number;
    int32_t fragmented_string_block;
    int16_t empty_string_offset;
    char reserved[13];
    node_cell nodes[NODES_IN_CONTROL_BLOCK];
} control_block;

#pragma pack(pop)

#endif //LLP_LAB1_GRAPH_STRUCT_H
