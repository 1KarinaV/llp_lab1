//
// Created by Карина Владыкина on 07.11.2022.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "graph_struct.h"

// Значения по умолчанию. Реальные значения загружаются из конфигурационного файла в функции initGraphsRuntime()

        int BufferSize = 1024*32; // Размеры всех буферов: узла, ввода и вывода. Это ограничение не влияет на
// максимальный размер сохраняемых строк (до 2Гб)

        int relink_table_delta = 20; // Приращение таблицы связей в случае, если свободное место кончислось
        int reserved_for_links_in_node = 8; // Максимально допустимое количество связей на узел
        int occupied_memory = 0;
memDBScheme * createDBScheme() { // Создает новую схему базы данных
    memDBScheme * result = (memDBScheme *) malloc(sizeof(memDBScheme));
    occupied_memory += sizeof(memDBScheme);
    result->FirstSchemeNode = NULL;
    result->LastSchemeNode = NULL;
    return result;
}

void freeDBSchemeAttr(memAttrRecord * Attr) { // Удаляет из памяти описатель атрибута
    occupied_memory -= strlen(Attr->NameString)+1;
    free(Attr->NameString);
    occupied_memory -= sizeof(memAttrRecord);
    free(Attr);
}

void freeDBSchemeNode(memNodeSchemeRecord * NodeScheme) { // Удаляет из памяти описатель типа узла
    memNodeDirectedTo * directed = NodeScheme->DirectedToFirst;
    memAttrRecord * attr = NodeScheme->AttrsFirst;
    occupied_memory -= 1 + strlen(NodeScheme->TypeString);
    free(NodeScheme->TypeString);
    while (directed != NULL) {
        memNodeDirectedTo * to_delete = directed;
        directed = directed->next;
        occupied_memory -= sizeof(memNodeDirectedTo);
        free(to_delete);
    }
    while (attr != NULL) {
        memAttrRecord * to_delete = attr;
        attr = attr->next;
        freeDBSchemeAttr(to_delete);
    }
    occupied_memory -= BufferSize;
    free(NodeScheme->Buffer);
    occupied_memory -= sizeof(memNodeSchemeRecord);
    free(NodeScheme);
}

void freeDBScheme(memDBScheme * Scheme) { // Удаляет из памяти схему базы данных
    memNodeSchemeRecord * result = Scheme->FirstSchemeNode;
    while (result != NULL) {
        memNodeSchemeRecord * to_delete = result;
        result = result->NextNodeScheme;
        freeDBSchemeNode(to_delete);
    }
    occupied_memory -= sizeof(memDBScheme);
    free(Scheme);
}