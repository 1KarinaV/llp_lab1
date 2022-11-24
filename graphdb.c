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

// Возвращает количество выделенной памяти
int getOccupiedMemory() {
    return occupied_memory;
}

void register_free(int amount) {
    occupied_memory -= amount;
}

int getMaxLinksNum() {
    return reserved_for_links_in_node;
}

void initGraphsRuntime(char * configFileName) {
    FILE * F = fopen(configFileName, "rt");
    if (F) {
        char buf[512];
        while (!feof(F)) {
            if (fgets(buf, sizeof(buf), F) && strlen(buf) != 0 && strcmp(buf, "\n") != 0) {
                char * str = buf;
                char * name = str;
                while (*str != 0 && *str != '=' && *str != ' ') str++;
                if (*str == 0)
                    printf("Config: unrecognized line : %s\n", name);
                else {
                    char * val = str+1;
                    int ival;
                    *str = 0;
                    while (*val != 0 && (*val == '=' || *val == ' ')) val++;
                    sscanf(val, "%i", &ival);
                    if (strcmp(name, "BufferSize") == 0)
                        BufferSize = ival;
                    else if (strcmp(name, "relink_table_delta") == 0)
                        relink_table_delta = ival;
                    else if (strcmp(name, "reserved_for_links_in_node") == 0)
                        reserved_for_links_in_node = ival;
                    else
                        printf("Config: unrecognized var : %s\n", name);
                }
            }
        }
        fclose(F);
    }
}

typedef struct { // Структура для перекодировки загруженных из файла Ѕƒ и уже невалидных указателей на реальные
    memNodeSchemeRecord ** place; // Указатель на указатель, который должен получить новое значение
    memNodeSchemeRecord * old_value; // загруженное значение
    memNodeSchemeRecord * new_value; // новое значение
} relink_item;

void addRelinking(memNodeSchemeRecord ** place, memNodeSchemeRecord * old_value, memNodeSchemeRecord * new_value,
                  relink_item ** relink_table, int * relink_table_size, int * relink_table_capacity) {
    if (*relink_table_size == *relink_table_capacity) {
        *relink_table_capacity += relink_table_delta;
        *relink_table = (relink_item *) realloc(*relink_table, (*relink_table_capacity)*sizeof(relink_item));
    }
    (*relink_table)[*relink_table_size].place = place;
    (*relink_table)[*relink_table_size].old_value = old_value;
    (*relink_table)[*relink_table_size].new_value = new_value;
    (*relink_table_size)++;
}

void make_relinkings(relink_item ** relink_table, int * relink_table_size, int * relink_table_capacity) {
    int i;
    for (i = 0; i < *relink_table_size; i++)
        if ((*relink_table)[i].place != NULL) {
            int j = 0;
            while (j < *relink_table_size && (*relink_table)[j].place != NULL || (*relink_table)[j].old_value != (*relink_table)[i].old_value)
                j++;
            *(*relink_table)[i].place = (*relink_table)[j].new_value;
        }
}

void write_buffer(char * Buffer, int * nBuffer, float What) {
    char * what = (char *) &What;
    int i;
    Buffer += *nBuffer;
    for (i = 0; i < sizeof(float); i++, Buffer++, (*nBuffer)++)
        *Buffer = what[i];
}

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

memNodeDirectedTo * checkCanLinkTo(memNodeSchemeRecord * NodeScheme, memNodeSchemeRecord * ToNodeScheme) {
    memNodeDirectedTo * result = NodeScheme->DirectedToFirst;
    while (result != NULL)
        if (ToNodeScheme == result->NodeScheme)
            return result;
        else
            result = result->next;
    return NULL;
}


memNodeDirectedTo * addDirectedToNodeScheme(memNodeSchemeRecord * NodeScheme, memNodeSchemeRecord * ToNodeScheme) {
    memNodeDirectedTo * rec;
    if (checkCanLinkTo(NodeScheme, ToNodeScheme))
        return NULL;
    rec = (memNodeDirectedTo *) malloc(sizeof(memNodeDirectedTo));
    occupied_memory += sizeof(memNodeDirectedTo);
    rec->NodeScheme = ToNodeScheme;
    rec->next = NULL;
    if (NodeScheme->DirectedToFirst == NULL || NodeScheme->DirectedToLast == NULL) {
        NodeScheme->DirectedToFirst = rec;
        NodeScheme->DirectedToLast = rec;
    } else {
        NodeScheme->DirectedToLast->next = rec;
        NodeScheme->DirectedToLast = rec;
    }
    return rec;
}

void delDirectedToFromNodeType(memNodeSchemeRecord * NodeScheme, memNodeSchemeRecord * Deleting) {
    if (NodeScheme->DirectedToFirst != NULL && NodeScheme->DirectedToLast != NULL) {
        if (NodeScheme->DirectedToFirst == NodeScheme->DirectedToLast) {
            if (NodeScheme->DirectedToFirst->NodeScheme == Deleting) {
                occupied_memory -= sizeof(memNodeDirectedTo);
                free(NodeScheme->DirectedToFirst);
                NodeScheme->DirectedToFirst = NULL;
                NodeScheme->DirectedToLast = NULL;
            }
        } else if (NodeScheme->DirectedToFirst->NodeScheme == Deleting) {
            memNodeDirectedTo * deleted = NodeScheme->DirectedToFirst;
            NodeScheme->DirectedToFirst = NodeScheme->DirectedToFirst->next;
            occupied_memory -= sizeof(memNodeDirectedTo);
            free(deleted);
        } else {
            memNodeDirectedTo * prev = NodeScheme->DirectedToFirst;
            while (prev != NULL && prev->next->NodeScheme != Deleting)
                prev = prev->next;
            if (prev != NULL) {
                memNodeDirectedTo * deleted = prev->next;
                if (NodeScheme->DirectedToLast->NodeScheme == Deleting) {
                    NodeScheme->DirectedToLast = prev;
                    prev->next = NULL;
                } else {
                    prev->next = prev->next->next;
                }
                occupied_memory -= sizeof(memNodeDirectedTo);
                free(deleted);
            }
        }
    }
}

memAttrRecord * findAttrByName(memNodeSchemeRecord * NodeScheme, char * Name, int * n) {
    memAttrRecord * result = NodeScheme->AttrsFirst;
    *n = 0;
    while (result != NULL)
        if (strcmp(Name, result->NameString) == 0)
            return result;
        else {
            result = result->next;
            (*n)++;
        }
    *n = -1;
    return NULL;
}


memAttrRecord * addAttrToNodeScheme(memNodeSchemeRecord * NodeScheme, char * Name, unsigned char Type) {
    memAttrRecord * result;
    if (NodeScheme->AttrsFirst == NULL || NodeScheme->AttrsLast == NULL) {
        result = (memAttrRecord *) malloc(sizeof(memAttrRecord));
        occupied_memory += sizeof(memAttrRecord);
        NodeScheme->AttrsFirst = result;
        NodeScheme->AttrsLast = result;
    } else {
        // Проверяем, может быть уже есть атрибут с таким именем?
        int n; // порядковый идентификатор существующего атрибута
        if (findAttrByName(NodeScheme, Name, &n)) // если нашли
            return NULL;
        result = (memAttrRecord *) malloc(sizeof(memAttrRecord));
        occupied_memory += sizeof(memAttrRecord);
        NodeScheme->AttrsLast->next = result;
        NodeScheme->AttrsLast = result;
    }
    result->NameString = (char *) malloc(1 + strlen(Name)*sizeof(char));
    occupied_memory += 1 + strlen(Name)*sizeof(char);
    strcpy(result->NameString, Name);
    result->Type = Type;
    result->next = NULL;

    return result;
}

void delAttrFromNodeScheme(memNodeSchemeRecord * NodeScheme, memAttrRecord * Deleting) {
    if (NodeScheme->AttrsFirst != NULL && NodeScheme->AttrsLast != NULL) {
        occupied_memory -= 1 + strlen(Deleting->NameString);
        free(Deleting->NameString);
        if (NodeScheme->AttrsFirst == NodeScheme->AttrsLast) {
            if (NodeScheme->AttrsFirst == Deleting) {
                occupied_memory -= sizeof(memAttrRecord);
                free(NodeScheme->AttrsFirst);
                NodeScheme->AttrsFirst = NULL;
                NodeScheme->AttrsLast = NULL;
            }
        } else if (NodeScheme->AttrsFirst == Deleting) {
            memAttrRecord * deleted = NodeScheme->AttrsFirst;
            NodeScheme->AttrsFirst = NodeScheme->AttrsFirst->next;
            occupied_memory -= sizeof(memAttrRecord);
            free(deleted);
        } else {
            memAttrRecord * prev = NodeScheme->AttrsFirst;
            while (prev != NULL && prev->next != Deleting)
                prev = prev->next;
            if (prev != NULL) {
                memAttrRecord * deleted = prev->next;
                if (NodeScheme->AttrsLast == Deleting) {
                    NodeScheme->AttrsLast = prev;
                    prev->next = NULL;
                } else {
                    prev->next = prev->next->next;
                }
                occupied_memory -= sizeof(memAttrRecord);
                free(deleted);
            }
        }
    }
}

// Далее идут функции буферизованного ввода/вывода
void db_fwrite(void * buf, int item_size, int n_items, memDB * DB) {
    char * _buf = (char *) buf;
    int n_free = BufferSize - DB->nWriteBuffer;
    int n_bytes = item_size*n_items;
    int i = DB->nWriteBuffer;
    if (DB->iReadBuffer < DB->nReadBuffer) {
        fseek(DB->FileDB, DB->iReadBuffer - DB->nReadBuffer, SEEK_CUR);
        DB->iReadBuffer = 0;
        DB->nReadBuffer = 0;
    }
    if (n_free > 0) {
        int to_write = n_free < n_bytes ? n_free : n_bytes;
        DB->nWriteBuffer += to_write;
        n_bytes -= to_write;
        for ( ; to_write > 0; to_write--, i++)
            DB->WriteBuffer[i] = *_buf++;
    }
    if (DB->nWriteBuffer == BufferSize) {
        fwrite(DB->WriteBuffer, 1, BufferSize, DB->FileDB);
        fwrite(_buf, 1, n_bytes, DB->FileDB);
        DB->nWriteBuffer = 0;
    }
}

void db_fread(void * buf, int item_size, int n_items, memDB * DB) {
    char * _buf = (char *) buf;
    int n_have = DB->nReadBuffer - DB->iReadBuffer;
    int n_bytes = item_size*n_items;
    for ( ; n_bytes > 0 && n_have > 0; n_have--, n_bytes--)
        *_buf++ = DB->ReadBuffer[DB->iReadBuffer++];
    if (n_bytes > 0) {
        fread(_buf, 1, n_bytes, DB->FileDB);
        DB->iReadBuffer = 0;
        DB->nReadBuffer = fread(DB->ReadBuffer, 1, BufferSize, DB->FileDB);
    }
}

void db_fflush(memDB * DB) {
    if (DB->iReadBuffer < DB->nReadBuffer) {
        fseek(DB->FileDB, DB->iReadBuffer - DB->nReadBuffer, SEEK_CUR);
        DB->iReadBuffer = 0;
        DB->nReadBuffer = 0;
    }
    if (DB->nWriteBuffer > 0) {
        fwrite(DB->WriteBuffer, 1, DB->nWriteBuffer, DB->FileDB);
        fflush(DB->FileDB);
        DB->nWriteBuffer = 0;
    }
}

int db_feof(memDB * DB) {
    db_fflush(DB);
    return feof(DB->FileDB);
}

void db_fclose(memDB * DB) {
    db_fflush(DB);
    fclose(DB->FileDB);
}

long int db_ftell(memDB * DB) {
    db_fflush(DB);
    return ftell(DB->FileDB);
}

void db_fseek(memDB * DB, long int offset, int whence) {
    db_fflush(DB);
    fseek(DB->FileDB, offset, whence);
}

long int getDBSize(memDB * DB) {
    db_fseek(DB, 0, SEEK_END);
    return db_ftell(DB);
}

// Сохраняет в файл список с типами разрешенных для соединения узлов
void storeDirectedList(memDB * DB, memNodeDirectedTo * List) {
    while (List != NULL) {
        // пишем все указатели на элементы NodeScheme напрямую -- при чтении все их скорректируем
        db_fwrite(&List->NodeScheme, sizeof(List->NodeScheme), 1, DB);
        List = List->next;
    }
    db_fwrite(&List, sizeof(List), 1, DB);
}

// Сохранет в файл список с атрибутами некоторого типа узла
void storeAttrsList(memDB * DB, memAttrRecord * List) {
    int NameStringLength;
    while (List != NULL) {
        NameStringLength = 1 + strlen(List->NameString);
        db_fwrite(&NameStringLength, sizeof(NameStringLength), 1, DB);
        db_fwrite(List->NameString, NameStringLength, 1, DB);
        db_fwrite(&List->Type, sizeof(List->Type), 1, DB);
        List = List->next;
    }
    NameStringLength = 0;
    db_fwrite(&NameStringLength, sizeof(NameStringLength), 1, DB);
}

// Сохраняет в файл данных схему узла
void storeNodeScheme(memDB * DB, memNodeSchemeRecord * NodeScheme) {
    int TypeStringLength = 1 + strlen(NodeScheme->TypeString);
    // пишем все указатели на элементы NodeScheme напрямую
    db_fwrite(&NodeScheme, sizeof(NodeScheme), 1, DB);
    db_fwrite(&TypeStringLength, sizeof(TypeStringLength), 1, DB);
    db_fwrite(NodeScheme->TypeString, TypeStringLength, 1, DB);
    storeDirectedList(DB, NodeScheme->DirectedToFirst);
    storeAttrsList(DB, NodeScheme->AttrsFirst);
}

// Сохраняет в файл данных схему данных
void storeScheme(memDB * DB, memDBScheme * Scheme) {
    memNodeSchemeRecord * NodeScheme = Scheme->FirstSchemeNode;
    while (NodeScheme != NULL) {
        storeNodeScheme(DB, NodeScheme);
        NodeScheme = NodeScheme->NextNodeScheme;
    }
    db_fwrite(&NodeScheme, sizeof(NodeScheme), 1, DB);
    NodeScheme = Scheme->FirstSchemeNode;
    while (NodeScheme != NULL) {
        int EmptyOffset = 0;
        NodeScheme->RootOffset = db_ftell(DB);
        db_fwrite(&EmptyOffset, sizeof(EmptyOffset), 1, DB);
        db_fwrite(&EmptyOffset, sizeof(EmptyOffset), 1, DB);
        NodeScheme = NodeScheme->NextNodeScheme;
    }
}

// Загружает из файла список с атрибутами
void loadAttrsList(memDB * DB, memNodeSchemeRecord * NodeScheme) {
    int NameStringLength;
    char * NameString;
    unsigned char Type;
    do {
        db_fread(&NameStringLength, sizeof(NameStringLength), 1, DB);
        if (NameStringLength > 0) {
            NameString = (char *) malloc(NameStringLength*sizeof(char));
            occupied_memory += NameStringLength*sizeof(char);
            db_fread(NameString, NameStringLength, 1, DB);
            db_fread(&Type, sizeof(Type), 1, DB);
            addAttrToNodeScheme(NodeScheme, NameString, Type);
            occupied_memory -= NameStringLength;
            free(NameString);
        }
    } while (NameStringLength != 0);
}

// Загружает из файла данных разрешенные типы узлов для соединения дугами
void loadDirectedList(memDB * DB, memNodeSchemeRecord * NodeScheme, relink_item ** relink_table, int * relink_table_size, int * relink_table_capacity) {
    memNodeSchemeRecord * loadedNodeScheme;
    do {
        db_fread(&loadedNodeScheme, sizeof(loadedNodeScheme), 1, DB);
        if (loadedNodeScheme) {
            memNodeDirectedTo * rec = addDirectedToNodeScheme(NodeScheme, loadedNodeScheme);
            addRelinking(&rec->NodeScheme, loadedNodeScheme, NULL, relink_table, relink_table_size, relink_table_capacity);
        }
    } while (loadedNodeScheme != NULL);
}

// Загружает из файла данных схему узла
memNodeSchemeRecord * loadNodeScheme(memDB * DB, relink_item ** relink_table, int * relink_table_size, int * relink_table_capacity) {
    memNodeSchemeRecord * loadedNodeScheme;
    memNodeSchemeRecord * NodeScheme;
    char * TypeString;
    int TypeStringLength;
    db_fread(&loadedNodeScheme, sizeof(loadedNodeScheme), 1, DB);
    if (loadedNodeScheme == NULL) return NULL;

    db_fread(&TypeStringLength, sizeof(TypeStringLength), 1, DB);
    TypeString = (char *) malloc(TypeStringLength*sizeof(char));
    occupied_memory += TypeStringLength*sizeof(char);
    db_fread(TypeString, TypeStringLength, 1, DB);
    NodeScheme = addNodeTypeToScheme(DB->Scheme, TypeString);
    occupied_memory -= TypeStringLength;
    free(TypeString);
    addRelinking(NULL, loadedNodeScheme, NodeScheme, relink_table, relink_table_size, relink_table_capacity);
    loadDirectedList(DB, NodeScheme, relink_table, relink_table_size, relink_table_capacity);
    loadAttrsList(DB, NodeScheme);
    return NodeScheme;
}

// Дозагружает из файла данных схему данных
void loadScheme(memDB * DB, memDBScheme * Scheme) {
    memNodeSchemeRecord * NodeScheme;
    relink_item * relink_table = (relink_item *) malloc(relink_table_delta*sizeof(relink_item));
    occupied_memory += relink_table_delta*sizeof(relink_item);
    int relink_table_capacity = relink_table_delta;
    int relink_table_size = 0;
    while (loadNodeScheme(DB, &relink_table, &relink_table_size, &relink_table_capacity));
    make_relinkings(&relink_table, &relink_table_size, &relink_table_capacity);
    occupied_memory -= relink_table_capacity*sizeof(relink_item);
    free(relink_table);
    NodeScheme = Scheme->FirstSchemeNode;
    while (NodeScheme != NULL) {
        NodeScheme->RootOffset = db_ftell(DB);
        db_fread(&NodeScheme->FirstOffset, sizeof(NodeScheme->FirstOffset), 1, DB);
        db_fread(&NodeScheme->LastOffset, sizeof(NodeScheme->LastOffset), 1, DB);
        NodeScheme->ThisOffset = NodeScheme->FirstOffset;
        NodeScheme = NodeScheme->NextNodeScheme;
    }
}

// Создает базу данных с описателем Scheme в файле с именем FileName
// Возвращает указатель на структуру данных, представляющую созданную базу
memDB * createNewDBbyScheme(memDBScheme * Scheme, char * FileName) {
    memDB * result = (memDB *) malloc(sizeof(memDB));
    occupied_memory += sizeof(memDB);
    result->FileDB = fopen(FileName, "w+b");
    if (result->FileDB) {
        long int SchemeLength;
        result->Scheme = Scheme;
        result->WriteBuffer = (char *) malloc(BufferSize);
        occupied_memory += BufferSize;
        result->nWriteBuffer = 0;
        result->ReadBuffer = (char *) malloc(BufferSize);
        occupied_memory += BufferSize;
        result->nReadBuffer = 0;
        result->iReadBuffer = 0;
        db_fseek(result, sizeof(long int), SEEK_SET); // оставляем место для указания размера схемы
        storeScheme(result, Scheme);
        SchemeLength = db_ftell(result);
        db_fseek(result, 0, SEEK_SET);
        db_fwrite(&SchemeLength, sizeof(SchemeLength), 1, result); // записываем размер схемы
        db_fflush(result);
        return result;
    } else {
        occupied_memory -= sizeof(*result);
        free(result);
        return NULL;
    }
}

// Закрывает открытую базу данных
void closeDB(memDB * DB) {
    freeDBScheme(DB->Scheme);
    db_fclose(DB);
    free(DB->WriteBuffer);
    free(DB->ReadBuffer);
    occupied_memory -= 2*BufferSize;
    free(DB);
    occupied_memory -= sizeof(*DB);
}

// Открывает существующую базу с именем FileName
memDB * openDB(char * FileName) {
    memDB * result = (memDB *) malloc(sizeof(memDB));
    occupied_memory += sizeof(memDB);
    result->FileDB = fopen(FileName, "r+b");
    if (result->FileDB) {
        result->Scheme = createDBScheme();
        result->WriteBuffer = (char *) malloc(BufferSize);
        occupied_memory += BufferSize;
        result->nWriteBuffer = 0;
        result->ReadBuffer = (char *) malloc(BufferSize);
        occupied_memory += BufferSize;
        result->nReadBuffer = 0;
        result->iReadBuffer = 0;
        db_fseek(result, sizeof(long int), SEEK_SET); // Пропускаем место указания размера схемы
        loadScheme(result, result->Scheme);
        return result;
    } else {
        occupied_memory -= sizeof(*result);
        free(result);
        return NULL;
    }
}

void createNode(memDB * DB, memNodeSchemeRecord * NodeScheme) {
    memAttrRecord * AList = NodeScheme->AttrsFirst;
    int i;
    cancelNode(DB, NodeScheme);
    NodeScheme->nBuffer = 0;
    NodeScheme->added = 1;
    // В начале записи резервируем место под атрибуты, каждый занимет 4 байта (для строк сохраняется смещение
    // созданной записи типа recString)
    while (AList != NULL) {
        write_buffer(NodeScheme->Buffer, &NodeScheme->nBuffer, 0.0);
        AList = AList->next;
    }
    // Записываем число присоединенных узлов
    write_buffer(NodeScheme->Buffer, &NodeScheme->nBuffer, 0.0);
    // Резервируем место для будущих дуг
    for (i = 0; i < reserved_for_links_in_node; i++) {
        write_buffer(NodeScheme->Buffer, &NodeScheme->nBuffer, 0.0);
        write_buffer(NodeScheme->Buffer, &NodeScheme->nBuffer, 0.0);
    }
}

int openNode(memDB * DB, memNodeSchemeRecord * NodeScheme) {
    unsigned char Type;
    int Dummy;
    int n;
    cancelNode(DB, NodeScheme);
    if (NodeScheme->ThisOffset == 0)
        return 0;
    NodeScheme->nBuffer = 0;
    NodeScheme->added = 0;
    db_fseek(DB, NodeScheme->ThisOffset, SEEK_SET);
    db_fread(&n, sizeof(n), 1, DB);
    db_fread(&Type, sizeof(Type), 1, DB);
    if (Type != recNodeData)
        return 0;
    db_fread(&Dummy, sizeof(int), 1, DB);
    NodeScheme->nBuffer = n - sizeof(n) - sizeof(Type) - sizeof(int);
    db_fread(NodeScheme->Buffer, NodeScheme->nBuffer, 1, DB);
    return 1;
}

void cancelNode(memDB * DB, memNodeSchemeRecord * NodeScheme) {
    NodeScheme->nBuffer = 0;
    NodeScheme->added = 0;
}

int nextNode(memDB * DB, memNodeSchemeRecord * NodeScheme) {
    cancelNode(DB, NodeScheme);
    if (NodeScheme->FirstOffset == 0 || NodeScheme->LastOffset == 0 ||
        NodeScheme->ThisOffset == 0)
        return 0;
    db_fseek(DB, NodeScheme->ThisOffset + sizeof(int) + sizeof(unsigned char), SEEK_SET);
    NodeScheme->PrevOffset = NodeScheme->ThisOffset;
    db_fread(&NodeScheme->ThisOffset, sizeof(int), 1, DB);
    return 1;
}

void rewindFirstNodes(memDB * DB, memNodeSchemeRecord * NodeScheme) {
    cancelNode(DB, NodeScheme);
    NodeScheme->ThisOffset = NodeScheme->FirstOffset;
    NodeScheme->PrevOffset = 0;
}

int deleteNode(memDB * DB, memNodeSchemeRecord * NodeScheme) {
    unsigned char Type = recEmpty;
    int AfterDeletedOffs;
    cancelNode(DB, NodeScheme);
    if (NodeScheme->FirstOffset == 0 || NodeScheme->LastOffset == 0 || NodeScheme->ThisOffset == 0)
        return 0;
    db_fseek(DB, NodeScheme->ThisOffset + sizeof(int), SEEK_SET);
    db_fwrite(&Type, sizeof(Type), 1, DB);
    db_fflush(DB);
    db_fread(&AfterDeletedOffs, sizeof(AfterDeletedOffs), 1, DB);
    if (NodeScheme->FirstOffset == NodeScheme->LastOffset) { // ”дал€ем единственный узел
        int EmptyOffset = 0;
        db_fseek(DB, NodeScheme->RootOffset, SEEK_SET);
        db_fwrite(&EmptyOffset, sizeof(EmptyOffset), 1, DB);
        NodeScheme->FirstOffset = 0;
        db_fwrite(&EmptyOffset, sizeof(EmptyOffset), 1, DB);
        NodeScheme->LastOffset = 0;
        NodeScheme->PrevOffset = 0;
        NodeScheme->ThisOffset = 0;
    } else if (NodeScheme->FirstOffset == NodeScheme->ThisOffset) { // ”дал€ем первый узел
        db_fseek(DB, NodeScheme->RootOffset, SEEK_SET);
        db_fwrite(&AfterDeletedOffs, sizeof(AfterDeletedOffs), 1, DB);
        NodeScheme->FirstOffset = AfterDeletedOffs;
        NodeScheme->PrevOffset = 0;
        NodeScheme->ThisOffset = AfterDeletedOffs;
    } else if (NodeScheme->LastOffset == NodeScheme->ThisOffset) { // ”дал€ем последний узел
        int EmptyOffset = 0;
        db_fseek(DB, NodeScheme->PrevOffset + sizeof(int) + sizeof(unsigned char), SEEK_SET);
        db_fwrite(&EmptyOffset, sizeof(EmptyOffset), 1, DB);
        db_fseek(DB, NodeScheme->RootOffset + sizeof(int), SEEK_SET);
        db_fwrite(&NodeScheme->PrevOffset, sizeof(int), 1, DB);
        NodeScheme->LastOffset = NodeScheme->PrevOffset;
        NodeScheme->PrevOffset = 0;
        NodeScheme->ThisOffset = 0;
    } else {
        db_fseek(DB, NodeScheme->PrevOffset + sizeof(int) + sizeof(unsigned char), SEEK_SET);
        db_fwrite(&AfterDeletedOffs, sizeof(AfterDeletedOffs), 1, DB);
        NodeScheme->ThisOffset = AfterDeletedOffs;
    }
    db_fflush(DB);
    return 1;
}

// Cоздает в базе новую строку и возвращает ее смещение от начала файла
int createString(memDB * DB, char * S) {
    unsigned char Type = recString;
    int L = strlen(S);
    int n = sizeof(int) + sizeof(unsigned char) + 1 + L;
    int result;
    db_fseek(DB, 0, SEEK_END);
    result = db_ftell(DB);
    db_fwrite(&n, sizeof(n), 1, DB);
    db_fwrite(&Type, sizeof(Type), 1, DB);
    db_fwrite(S, L + 1, 1, DB);
    db_fflush(DB);
    return result;
}

// Загружает из базы новую строку по ее смещению. Cтрока создается в динамической памяти
char * getString(memDB * DB, int Offset) {
    unsigned char Type;
    char * result;
    int L;
    int n;
    db_fseek(DB, Offset, SEEK_SET);
    db_fread(&n, sizeof(n), 1, DB);
    db_fread(&Type, sizeof(Type), 1, DB);
    if (Type != recString)
        return NULL;
    L = n - sizeof(int) - sizeof(unsigned char);
    result = (char *) malloc(L);
    occupied_memory += L;
    db_fread(result, L, 1, DB);
    return result;
}

// Устанавливает значение атрибута текущего узла
void setNodeAttr(memDB * DB, memNodeSchemeRecord * NodeScheme, char * AttrName, float Value) {
    int n;
    if (NodeScheme->nBuffer > 0 && findAttrByName(NodeScheme, AttrName, &n)) {
        n *= sizeof(float);
        write_buffer(NodeScheme->Buffer, &n, Value);
    }
}

// Загружает значение атрибута текущего узла
float getNodeAttr(memDB * DB, memNodeSchemeRecord * NodeScheme, char * AttrName) {
    int n;
    if (NodeScheme->nBuffer > 0 && findAttrByName(NodeScheme, AttrName, &n)) {
        float * buf = (float *) NodeScheme->Buffer;
        return buf[n];
    } else
        return 0.0;
}
int LinkCurrentNodeToCurrentNode(memDB * DB, memNodeSchemeRecord * NodeSchemeFrom, memNodeSchemeRecord * NodeSchemeTo) {
    memAttrRecord * AList = NodeSchemeFrom->AttrsFirst;
    memNodeDirectedTo * DList = NodeSchemeFrom->DirectedToFirst;
    float * buf;
    int n, i;
    int offs = 0;
    while (DList != NULL && DList->NodeScheme != NodeSchemeTo) {
        DList = DList->next;
    }
    if (DList == NULL)
        return 0;
    while (AList != NULL) {
        offs += sizeof(float);
        AList = AList->next;
    }
    // число присоединенных узлов
    buf = (float *) (NodeSchemeFrom->Buffer + offs);
    n = *buf;
    for (i = 0; i < n; i++)
        if (buf[2*i+1] == NodeSchemeTo->RootOffset && buf[2*i+2] == NodeSchemeTo->ThisOffset)
            return 1;
    if (n < reserved_for_links_in_node) {
        (*buf)++;
        buf[2*n+1] = NodeSchemeTo->RootOffset;
        buf[2*n+2] = NodeSchemeTo->ThisOffset;
        return 1;
    } else
        return 0;
}

void postNode(memDB * DB, memNodeSchemeRecord * NodeScheme) {
    if (NodeScheme->nBuffer > 0) {
        // Считываем случаи единственного (первого+последнего) и последнего узла
        if (NodeScheme->added) {
            int n = sizeof(int) + sizeof(unsigned char) + sizeof(int) + NodeScheme->nBuffer;
            unsigned char Type = recNodeData;
            int EmptyOffset = 0;
            db_fseek(DB, 0, SEEK_END);
            NodeScheme->ThisOffset = db_ftell(DB);
            db_fwrite(&n, sizeof(n), 1, DB);
            db_fwrite(&Type, sizeof(Type), 1, DB);
            db_fwrite(&EmptyOffset, sizeof(EmptyOffset), 1, DB);
            db_fwrite(NodeScheme->Buffer, NodeScheme->nBuffer, 1, DB);
            NodeScheme->PrevOffset = NodeScheme->LastOffset;
            if (NodeScheme->FirstOffset == 0 && NodeScheme->LastOffset == 0) { // ¬ базе пока нет узлов
                db_fseek(DB, NodeScheme->RootOffset, SEEK_SET);
                db_fwrite(&NodeScheme->ThisOffset, sizeof(int), 1, DB);
                db_fwrite(&NodeScheme->ThisOffset, sizeof(int), 1, DB);
                NodeScheme->FirstOffset = NodeScheme->ThisOffset;
                NodeScheme->LastOffset = NodeScheme->ThisOffset;
            } else {
                db_fseek(DB, NodeScheme->PrevOffset + sizeof(int) + sizeof(unsigned char), SEEK_SET);
                db_fwrite(&NodeScheme->ThisOffset, sizeof(int), 1, DB);
                db_fseek(DB, NodeScheme->RootOffset + sizeof(int), SEEK_SET);
                db_fwrite(&NodeScheme->ThisOffset, sizeof(int), 1, DB);
                NodeScheme->LastOffset = NodeScheme->ThisOffset;
            }
            NodeScheme->added = 0;
        } else {
            db_fseek(DB, NodeScheme->ThisOffset + sizeof(int) + sizeof(unsigned char) + sizeof(int), SEEK_SET);
            db_fwrite(NodeScheme->Buffer, NodeScheme->nBuffer, 1, DB);
        }
        db_fflush(DB);
    }
}

int testNodeCondition(memDB * DB, memNodeSchemeRecord * NodeScheme, memCondition * Condition) {
/*
  enum { oprndIntBoolFloat = 0, oprndString, oprndAttrName, oprndCondition } memOperandsType; // “ипы операндов в услови€х
  enum { opEqual = 0, opNotEqual, opLess, opGreater, opNot, opAnd, opOr } memOperationsType; // “ипы операций
*/
    memConditionOperand Left;
    memConditionOperand Right;
    int result;
    if (NodeScheme->nBuffer == 0)
        return 0;
    if (Condition == NULL)
        return 1;

    Left = *Condition->Operand1;
    if (Condition->Operand1->OperandType == oprndAttrName) {
        int i;
        memAttrRecord * desc = findAttrByName(NodeScheme, Condition->Operand1->opAttrName, &i);
        float val = getNodeAttr(DB, NodeScheme, Condition->Operand1->opAttrName);
        switch (desc->Type) {
            case tpString:
                Left.OperandType = oprndString;
                Left.opString = getString(DB, val);
                break;
            default:
                Left.OperandType = oprndIntBoolFloat;
                Left.opInt_Bool_Float = val;
                break;
        }
    }
    Right = *Condition->Operand2;
    if (Condition->Operand2->OperandType == oprndAttrName) {
        int i;
        memAttrRecord * desc = findAttrByName(NodeScheme, Condition->Operand2->opAttrName, &i);
        float val = getNodeAttr(DB, NodeScheme, Condition->Operand2->opAttrName);
        switch (desc->Type) {
            case tpString:
                Right.OperandType = oprndString;
                Right.opString = getString(DB, val);
                break;
            default:
                Right.OperandType = oprndIntBoolFloat;
                Right.opInt_Bool_Float = val;
                break;
        }
    }

    switch (Condition->OperationType) {
        case opEqual: {
            if (Left.OperandType == Right.OperandType) {
                if (Left.OperandType == oprndString)
                    result = strcmp(Left.opString, Right.opString) == 0;
                else
                    result = Left.opInt_Bool_Float == Right.opInt_Bool_Float;
            } else
                result = 0;
            break;
        }
        case opNotEqual: {
            if (Left.OperandType == Right.OperandType) {
                if (Left.OperandType == oprndString)
                    result = strcmp(Left.opString, Right.opString) != 0;
                else
                    result = Left.opInt_Bool_Float != Right.opInt_Bool_Float;
            } else
                result = 0;
            break;
        }
        case opLess: {
            if (Left.OperandType == Right.OperandType) {
                if (Left.OperandType == oprndString)
                    result = strcmp(Left.opString, Right.opString) < 0;
                else
                    result = Left.opInt_Bool_Float < Right.opInt_Bool_Float;
            } else
                result = 0;
            break;
        }
        case opGreater: {
            if (Left.OperandType == Right.OperandType) {
                if (Left.OperandType == oprndString)
                    result = strcmp(Left.opString, Right.opString) > 0;
                else
                    result = Left.opInt_Bool_Float > Right.opInt_Bool_Float;
            } else
                result = 0;
            break;
        }
        case opNot: {
            result = !testNodeCondition(DB, NodeScheme, Left.opCondition);
            break;
        }
        case opAnd: {
            result = testNodeCondition(DB, NodeScheme, Left.opCondition) &&
                     testNodeCondition(DB, NodeScheme, Right.opCondition);
            break;
        }
        case opOr: {
            result = testNodeCondition(DB, NodeScheme, Left.opCondition) ||
                     testNodeCondition(DB, NodeScheme, Right.opCondition);
            break;
        }
    }

    if (Condition->Operand1->OperandType == oprndAttrName && Left.OperandType == oprndString) {
        occupied_memory -= 1 + strlen(Left.opString);
        free(Left.opString);
    }
    if (Condition->Operand2->OperandType == oprndAttrName && Right.OperandType == oprndString) {
        occupied_memory -= 1 + strlen(Right.opString);
        free(Right.opString);
    }
    return result;
}

memNodeSetItem * queryAllNodesOfType(memDB * DB, memNodeSchemeRecord * NodeScheme, memCondition * Condition) {
    memNodeSetItem * result = NULL;
    memNodeSetItem * prev = NULL;
    rewindFirstNodes(DB, NodeScheme);
    while (openNode(DB, NodeScheme)) {
        if (testNodeCondition(DB, NodeScheme, Condition)) {
            memNodeSetItem * item = (memNodeSetItem *) malloc(sizeof(memNodeSetItem));
            occupied_memory += sizeof(memNodeSetItem);
            item->NodeScheme = NodeScheme;
            item->PrevOffset = NodeScheme->PrevOffset;
            item->ThisOffset = NodeScheme->ThisOffset;
            item->next = NULL;
            item->prev = prev;
            if (prev != NULL) prev->next = item;
            prev = item;
            if (result == NULL) result = item;
        }
        nextNode(DB, NodeScheme);
    }
    return result;
}


void navigateByNodeSetItem(memDB * DB, memNodeSetItem * NodeSet) {
    NodeSet->NodeScheme->PrevOffset = NodeSet->PrevOffset;
    NodeSet->NodeScheme->ThisOffset = NodeSet->ThisOffset;
}

memNodeSetItem * queryNodeSet(memDB * DB, memNodeSetItem * NodeSet, memCondition * Condition) {
    memNodeSetItem * result = NULL;
    memNodeSetItem * prev = NULL;
    if (NodeSet == NULL)
        return NULL;
    rewindFirstNodes(DB, NodeSet->NodeScheme);
    while (NodeSet != NULL && openNode(DB, NodeSet->NodeScheme)) {
        if (NodeSet->NodeScheme->ThisOffset == NodeSet->ThisOffset) {
            NodeSet->PrevOffset = NodeSet->NodeScheme->PrevOffset;
            if (testNodeCondition(DB, NodeSet->NodeScheme, Condition)) {
                memNodeSetItem * item = (memNodeSetItem *) malloc(sizeof(memNodeSetItem));
                occupied_memory += sizeof(memNodeSetItem);
                item->NodeScheme = NodeSet->NodeScheme;
                item->PrevOffset = NodeSet->NodeScheme->PrevOffset;
                item->ThisOffset = NodeSet->NodeScheme->ThisOffset;
                item->next = NULL;
                item->prev = prev;
                if (prev != NULL) prev->next = item;
                prev = item;
                if (result == NULL) result = item;
            }
            cancelNode(DB, NodeSet->NodeScheme);
            NodeSet = NodeSet->next;
        } else {
            cancelNode(DB, NodeSet->NodeScheme);
            nextNode(DB, NodeSet->NodeScheme);
        }
    }
    return result;
}

void freeNodeSet(memDB * DB, memNodeSetItem * NodeSet) {
    while (NodeSet != NULL) {
        memNodeSetItem * to_delete = NodeSet;
        NodeSet = NodeSet->next;
        occupied_memory -= sizeof(*to_delete);
        free(to_delete);
    }
}

float * getDirectedToList(memDB * DB, memNodeSchemeRecord * NodeScheme, int * n) {
    memAttrRecord * AList = NodeScheme->AttrsFirst;
    float * buf;
    float * result = NULL;
    int offs = 0;
    while (AList != NULL) {
        offs += sizeof(float);
        AList = AList->next;
    }
    // число присоединенных узлов
    buf = (float *) (NodeScheme->Buffer + offs);
    *n = *buf;
    if (*n != 0) {
        int i;
        int nbytes = 2*(*n)*sizeof(float);
        result = (float *) malloc(nbytes);
        occupied_memory += nbytes;
        for (i = 0; i < *n; i++) {
            unsigned char Type;
            // ѕровер€ем, не удален ли узел, на который ссылаемс€
            db_fseek(DB, buf[2*i+2] + sizeof(int), SEEK_SET);
            db_fread(&Type, sizeof(Type), 1, DB);
            if (Type != recNodeData) {
                result[2*i] = 0.0;
                result[2*i+1] = 0.0;
            } else {
                result[2*i] = buf[2*i+1];
                result[2*i+1] = buf[2*i+2];
            }
        }
    }
    return result;
}

// Внутренняя функция
// Выбирает все узлы, соответствующие запросу в Cypher-стиле длиной nLinks элементов
// Каждый элемент запроса включает ссылку на тип узла и наложенное на него условие
memNodeSetItem * _queryCypherStyle(memDB * DB, int nLinks, va_list args) {
    memNodeSetItem * set;
    memNodeSchemeRecord * NodeScheme;
    memCondition * Condition;
    if (nLinks == 0)
        return NULL;
    NodeScheme = va_arg(args, memNodeSchemeRecord *);
    Condition = va_arg(args, memCondition *);
    set = queryAllNodesOfType(DB, NodeScheme, Condition);
    nLinks--;
    while (nLinks > 0) {
        memNodeSchemeRecord * NodeSchemeNext;
        memCondition * ConditionNext;
        memNodeSetItem * setNext0 = NULL;
        memNodeSetItem * setNext;
        memNodeSetItem * set_ptr;
        memNodeSetItem * prev = NULL;
        if (set == NULL)
            return NULL;
        NodeSchemeNext = va_arg(args, memNodeSchemeRecord *);
        ConditionNext = va_arg(args, memCondition *);
        set_ptr = set;
        while (set_ptr != NULL) {
            NodeScheme->PrevOffset = set_ptr->PrevOffset;
            NodeScheme->ThisOffset = set_ptr->ThisOffset;
            if (openNode(DB, NodeScheme)) {
                int i, n;
                float * links = getDirectedToList(DB, NodeScheme, &n);
                // Фильтруем set/DirectedTo(NodeSchemeNext) в setNext0(NodeSchemeNext)
                for (i = 0; i < n; i++) {
                    if (links[2*i] == NodeSchemeNext->RootOffset) {
                        memNodeSetItem * item = (memNodeSetItem *) malloc(sizeof(memNodeSetItem));
                        occupied_memory += sizeof(memNodeSetItem);
                        item->NodeScheme = NodeSchemeNext;
                        item->PrevOffset = 0;
                        item->ThisOffset = links[2*i+1];
                        item->next = NULL;
                        item->prev = prev;
                        if (prev != NULL) prev->next = item;
                        prev = item;
                        if (setNext0 == NULL) setNext0 = item;
                    }
                }
                cancelNode(DB, NodeScheme);
                occupied_memory -= 2*n*sizeof(float);
                free(links);
            }
            set_ptr = set_ptr->next;
        }
        if (setNext0 == NULL)
            setNext = NULL;
        else
            setNext = queryNodeSet(DB, setNext0, ConditionNext);
        freeNodeSet(DB, set);
        freeNodeSet(DB, setNext0);
        set = setNext;
        NodeScheme = NodeSchemeNext;
        Condition = ConditionNext;
        nLinks--;
    }
    return set;
}

// Выбирает все узлы, соответствующие запросу в Cypher-стиле длиной nLinks элементов
// Каждый элемент запроса включает ссылку на тип узла и наложенное на него условие
memNodeSetItem * queryCypherStyle(memDB * DB, int nLinks, ...) {
    memNodeSetItem * result;
    va_list args;
    va_start(args, nLinks);
    result = _queryCypherStyle(DB, nLinks, args);
    va_end(args);
    return result;
}

void deleteCypherStyle(memDB * DB, int nLinks, ...) {
    memNodeSetItem * set;
    memNodeSetItem * set1;
    va_list args;
    va_start(args, nLinks);
    set = _queryCypherStyle(DB, nLinks, args);
    set1 = set;
    va_end(args);
    // ќбрабатываем set
    while (set != NULL && set->next != NULL)
        set = set->next;
    // ”дал€ем узлы в обратном пор€дке, чтобы не мен€лс€ PrevOffset
    while (set != NULL) {
        navigateByNodeSetItem(DB, set);
        deleteNode(DB, set->NodeScheme);
        set = set->prev;
    }
    freeNodeSet(DB, set1);
}

void setCypherStyle(memDB * DB, char * AttrName, float AttrVal, int nLinks, ...) {
    memNodeSetItem * set;
    memNodeSetItem * set1;
    va_list args;
    va_start(args, nLinks);
    set = _queryCypherStyle(DB, nLinks, args);
    set1 = set;
    va_end(args);
    // ќбрабатываем set
    while (set != NULL) {
        navigateByNodeSetItem(DB, set);
        if (openNode(DB, set->NodeScheme)) {
            setNodeAttr(DB, set->NodeScheme, AttrName, AttrVal);
            postNode(DB, set->NodeScheme);
        }
        set = set->next;
    }
    freeNodeSet(DB, set1);
}

memCondition * createLogicCondition(unsigned char operation, memCondition * operand1, memCondition * operand2) {
    memCondition * result = (memCondition *) malloc(sizeof(memCondition));
    memConditionOperand * _operand1 = (memConditionOperand *) malloc(sizeof(memConditionOperand));
    memConditionOperand * _operand2 = (memConditionOperand *) malloc(sizeof(memConditionOperand));
    occupied_memory += sizeof(memCondition) + 2*sizeof(memConditionOperand);
    _operand1->OperandType = oprndCondition;
    _operand1->opCondition = operand1;
    _operand2->OperandType = oprndCondition;
    _operand2->opCondition = operand2;
    result->OperationType = operation;
    result->Operand1 = _operand1;
    result->Operand2 = _operand2;
    return result;
}

// ”слови€-отношени€ на атрибуты
memCondition * createStringAttrCondition(unsigned char operation, char * AttrName, char * Val) {
    memCondition * result = (memCondition *) malloc(sizeof(memCondition));
    memConditionOperand * operand1 = (memConditionOperand *) malloc(sizeof(memConditionOperand));
    memConditionOperand * operand2 = (memConditionOperand *) malloc(sizeof(memConditionOperand));
    occupied_memory += sizeof(memCondition) + 2*sizeof(memConditionOperand);
    operand1->OperandType = oprndAttrName;
    operand1->opAttrName = (char *) malloc((strlen(AttrName)+1)*sizeof(char));
    occupied_memory += 1 + strlen(AttrName);
    strcpy(operand1->opAttrName, AttrName);
    operand2->OperandType = oprndString;
    operand2->opString =  (char *) malloc((strlen(Val)+1)*sizeof(char));
    occupied_memory += 1 + strlen(Val);
    strcpy(operand2->opString, Val);
    result->OperationType = operation;
    result->Operand1 = operand1;
    result->Operand2 = operand2;
    return result;
}

memCondition * createIntOrBoolAttrCondition(unsigned char operation, char * AttrName, int Val) {
    return createFloatAttrCondition(operation, AttrName, (float) Val);
}

memCondition * createFloatAttrCondition(unsigned char operation, char * AttrName, float Val) {
    memCondition * result = (memCondition *) malloc(sizeof(memCondition));
    memConditionOperand * operand1 = (memConditionOperand *) malloc(sizeof(memConditionOperand));
    memConditionOperand * operand2 = (memConditionOperand *) malloc(sizeof(memConditionOperand));
    occupied_memory += sizeof(memCondition) + 2*sizeof(memConditionOperand);
    operand1->OperandType = oprndAttrName;
    operand1->opAttrName = (char *) malloc((strlen(AttrName)+1)*sizeof(char));
    occupied_memory += 1 + strlen(AttrName);
    strcpy(operand1->opAttrName, AttrName);
    operand2->OperandType = oprndIntBoolFloat;
    operand2->opInt_Bool_Float = Val;
    result->OperationType = operation;
    result->Operand1 = operand1;
    result->Operand2 = operand2;
    return result;
}

void freeOperand(memConditionOperand * op) {
    switch (op->OperandType) {
        case oprndCondition:
            freeCondition(op->opCondition);
            break;
        case oprndString:
            occupied_memory -= 1 + strlen(op->opString);
            free(op->opString);
            break;
        case oprndAttrName:
            occupied_memory -= 1 + strlen(op->opAttrName);
            free(op->opAttrName);
            break;
    }
    occupied_memory -= sizeof(*op);
    free(op);
}

void freeCondition(memCondition * Cond) {
    freeOperand(Cond->Operand1);
    freeOperand(Cond->Operand2);
    occupied_memory -= sizeof(*Cond);
    free(Cond);
}
