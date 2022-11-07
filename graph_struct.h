#ifndef LLP_LAB1_GRAPH_STRUCT_H
#define LLP_LAB1_GRAPH_STRUCT_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
// Структуры, описывающие схему данных в памяти

// Опережающие объявления
struct memNodeSchemeRecord;
struct memAttrRecord;
struct memCondition;

typedef struct memNodeDirectedTo { // Структура-описатель элемента списка допустимых к подключению типов узлов
    struct memNodeSchemeRecord * NodeScheme; // Указатель на тип узла
    struct memNodeDirectedTo * next; // Указатель на следующий элемент списка
} memNodeDirectedTo;

typedef struct memNodeSchemeRecord { // Структура-описатель типа узла в памяти
    char * TypeString; // Запись с именем типа узла
    int RootOffset; // Смещение от начала файла корневого указателя на список узлов
    int FirstOffset; // Смещение от начала файла корневого указателя на первый элемент списка узлов
    int LastOffset; // Смещение от начала файла корневого указателя на последний элемент списка узлов
    char * Buffer; // Буфер с данными текущего узла
    int nBuffer; // Число заполненных байт в буфере
    int added; // Флаг того, что создается новый узел
    int PrevOffset; // Смещение от начала файла предыдущего узла
    int ThisOffset; // Смещение от начала файла текущего узла
    memNodeDirectedTo * DirectedToFirst; // Указатель на начало списка с типами узлов
    memNodeDirectedTo * DirectedToLast; // Указатель на конец списка с типами узлов
    struct memAttrRecord * AttrsFirst; // Указатель на начало списка атрибутов
    struct memAttrRecord * AttrsLast; // Указатель на конец списка атрибутов
    struct memNodeSchemeRecord * NextNodeScheme; // Указатель на следующий тип узла
} memNodeSchemeRecord;

typedef struct memAttrRecord { // Структура-описатель атрибута узла
    char * NameString; // Запись с именем атрибута
    unsigned char Type; // Тип атрибута
    struct memAttrRecord * next; // Указатель на следующий атрибут
} memAttrRecord;

typedef struct { // Структура-описатель структуры базы данных
    memNodeSchemeRecord * FirstSchemeNode; // Указатель на описатель первого типа узла
    memNodeSchemeRecord * LastSchemeNode; // Указатель на описатель последнего типа узла
} memDBScheme;

typedef struct { // Структура-описатель открытой базы данных
    memDBScheme * Scheme;
    char * WriteBuffer; // Буфер записи
    int nWriteBuffer;
    char * ReadBuffer; // Буфер чтения
    int nReadBuffer;
    int iReadBuffer;
    FILE * FileDB;
} memDB;

typedef struct memNodeSetItem { // Структура-элемент результата запроса к базе данных
    memNodeSchemeRecord * NodeScheme; // Ссылка на тип узла
    int PrevOffset; // Смещение предыдущего узла этого типа
    int ThisOffset; // Смещение текущего узла этого типа
    struct memNodeSetItem * next; // Следующий элемент
    struct memNodeSetItem * prev; // Предыдущий элемент
} memNodeSetItem;

typedef struct { // Операнд в операции условия
    unsigned char OperandType; // тип операнда
    union {
        struct memCondition * opCondition; // Другое условие
        char * opString; // Строка
        float opInt_Bool_Float; // Целое число или логическое значение или вещественное число
        char * opAttrName;
    };
} memConditionOperand;

typedef struct memCondition { // Элемент условия
    unsigned char OperationType; // Операция
    memConditionOperand * Operand1; // Первый операнд
    memConditionOperand * Operand2; // Второй операнд (или NULL, если операция унарная)
} memCondition;

memDBScheme *

createDBScheme(); // Создает новую схему базы данных
void freeDBSchemeAttr(memAttrRecord * Attr); // Удаляет из памяти описатель атрибута
void freeDBSchemeNode(memNodeSchemeRecord * NodeScheme); // Удаляет из памяти описатель типа узла
void freeDBScheme(memDBScheme * Scheme); // Удаляет из памяти схему базы данных
#endif //LLP_LAB1_GRAPH_STRUCT_H
