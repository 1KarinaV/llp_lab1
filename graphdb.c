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

memNodeSchemeRecord * findNodeSchemeByTypeName(memDBScheme * Scheme, char * TypeName, int * n) {
    memNodeSchemeRecord * result = Scheme->FirstSchemeNode;
    *n = 0;
    while (result != NULL)
        if (strcmp(TypeName, result->TypeString) == 0)
            return result;
        else {
            result = result->NextNodeScheme;
            (*n)++;
        }
    *n = -1;
    return NULL;
}

memNodeSchemeRecord * addNodeTypeToScheme(memDBScheme * Scheme, char * TypeName) { // Добавляет новый тип узла в схему
    memNodeSchemeRecord * result;
    if (Scheme->FirstSchemeNode == NULL || Scheme->LastSchemeNode == NULL) {
        result = (memNodeSchemeRecord *) malloc(sizeof(memNodeSchemeRecord));
        occupied_memory += sizeof(memNodeSchemeRecord);
        Scheme->FirstSchemeNode = result;
        Scheme->LastSchemeNode = result;
    } else {
        // Проверяем, может быть уже есть узел с таким именем?
        int n; // порядковый идентификатор существующего узла
        if (findNodeSchemeByTypeName(Scheme, TypeName, &n)) // если нашли
            return NULL;
        result = (memNodeSchemeRecord *) malloc(sizeof(memNodeSchemeRecord));
        occupied_memory += sizeof(memNodeSchemeRecord);
        Scheme->LastSchemeNode->NextNodeScheme = result;
        Scheme->LastSchemeNode = result;
    }
    result->TypeString = (char *) malloc(1 + strlen(TypeName)*sizeof(char));
    occupied_memory += 1 + strlen(TypeName)*sizeof(char);
    strcpy(result->TypeString, TypeName);
    result->RootOffset = 0;
    result->FirstOffset = 0;
    result->LastOffset = 0;
    result->Buffer = (char *) malloc(BufferSize*sizeof(char)); // Буфер с данными текущего узла
    occupied_memory += BufferSize*sizeof(char);
    result->nBuffer = 0; // Число заполненных байт в буфере
    result->added = 0;
    result->PrevOffset = 0; // Смещение от начала файла предыдущего узла
    result->ThisOffset = 0; // Смещение от начала файла текущего узла
    result->DirectedToFirst = NULL;
    result->DirectedToLast = NULL;
    result->AttrsFirst = NULL;
    result->AttrsLast = NULL;
    result->NextNodeScheme = NULL;

    return result;
}

void delNodeTypeFromScheme(memDBScheme * Scheme, memNodeSchemeRecord * NodeScheme) {
    if (Scheme->FirstSchemeNode != NULL && Scheme->LastSchemeNode != NULL) {
        if (Scheme->FirstSchemeNode == Scheme->LastSchemeNode) {
            if (Scheme->FirstSchemeNode == NodeScheme) {
                freeDBSchemeNode(NodeScheme);
                Scheme->FirstSchemeNode = NULL;
                Scheme->LastSchemeNode = NULL;
            }
        } else if (Scheme->FirstSchemeNode == NodeScheme) {
            Scheme->FirstSchemeNode = NodeScheme->NextNodeScheme;
            freeDBSchemeNode(NodeScheme);
        } else {
            memNodeSchemeRecord * prev = Scheme->FirstSchemeNode;
            while (prev != NULL && prev->NextNodeScheme != NodeScheme)
                prev = prev->NextNodeScheme;
            if (prev != NULL) {
                if (Scheme->LastSchemeNode == NodeScheme) {
                    Scheme->LastSchemeNode = prev;
                    prev->NextNodeScheme = NULL;
                } else {
                    prev->NextNodeScheme = NodeScheme->NextNodeScheme;
                }
                freeDBSchemeNode(NodeScheme);
            }
        }
    }
}