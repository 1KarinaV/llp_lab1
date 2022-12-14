#ifndef LLP_LAB1_GRAPH_STRUCT_H
#define LLP_LAB1_GRAPH_STRUCT_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

enum { recEmpty = 0, recString, recNodeData } tpRecords; // Типы записей в файле
enum { tpInt32 = 0, tpFloat, tpString, tpBoolean } tpDataItems; // Типы данных атрибутов

enum { oprndIntBoolFloat = 0, oprndString, oprndAttrName, oprndCondition }; // типы операндов в условиях
enum { opEqual = 0, opNotEqual, opLess, opGreater, opNot, opAnd, opOr }; // типы операций

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

int getOccupiedMemory();  // Возвращает количество выделенной памяти
void register_free(int amount);   // Считывает освобожденную в программе память
void initGraphsRuntime(char * configFileName);
int getMaxLinksNum();    // Возвращает максимально возможное число связей узла
long int getDBSize(memDB * DB);     // Возвращает физический размер файла базы данных

memDBScheme * createDBScheme();          // Создает новую схему базы данных
void freeDBSchemeAttr(memAttrRecord * Attr);         // Удаляет из памяти описатель атрибута
void freeDBSchemeNode(memNodeSchemeRecord * NodeScheme);       // Удаляет из памяти описатель типа узла
void freeDBScheme(memDBScheme * Scheme);            // Удаляет из памяти схему базы данных


// Поиск описателя узла в схеме Scheme по имени TypeName, порядковый номер описателя попадает в *n,
// возвращает найденный описатель или NULL, если не найден
memNodeSchemeRecord * findNodeSchemeByTypeName(memDBScheme * Scheme, char * TypeName, int * n);

memNodeSchemeRecord * addNodeTypeToScheme(memDBScheme * Scheme, char * TypeName); // Добавляет новый тип узла в схему

void delNodeTypeFromScheme(memDBScheme * Scheme, memNodeSchemeRecord * NodeScheme);    // Удаляет описатель типа узла из схемы


// Проверяет, может ли из узла типа NodeScheme вести дуга в узел типа ToNodeScheme
// при успехе возвращает ссылку на регистрационную запись, иначе NULL
memNodeDirectedTo * checkCanLinkTo(memNodeSchemeRecord * NodeScheme, memNodeSchemeRecord * ToNodeScheme);

// Добавляет к описателю узла NodeScheme ссылку на тип узла ToNodeScheme, в который может идти дуга.
// Возвращает NULL при повторном добавлении или ссылку на добавляемый тип
memNodeDirectedTo * addDirectedToNodeScheme(memNodeSchemeRecord * NodeScheme, memNodeSchemeRecord * ToNodeScheme);

void delDirectedToFromNodeType(memNodeSchemeRecord * NodeScheme, memNodeSchemeRecord * Deleting);    // Удаляет возможность связей с узлами Deleting из определения типа узла NodeScheme


// Поиск атрибута в описателе узла NodeScheme по имени Name, порядковый номер атрибута попадает в *n,
// Возвращает найденный описатель атрибута или NULL, если не найден
memAttrRecord * findAttrByName(memNodeSchemeRecord * NodeScheme, char * Name, int * n);

memAttrRecord * addAttrToNodeScheme(memNodeSchemeRecord * NodeScheme, char * Name, unsigned char Type);  // Добавляет к описателю узла атрибут с именем Name и типом Type

void delAttrFromNodeScheme(memNodeSchemeRecord * NodeScheme, memAttrRecord * Deleting);  // Удаляет атрибут Deleting из определения типа узла NodeScheme

// Создает базу данных с описателем Scheme в файле с именем FileName
// Возвращает указатель на структуру данных, представляющую созданную базу
memDB * createNewDBbyScheme(memDBScheme * Scheme, char * FileName);

memDB * openDB(char * FileName);    // Открывает существующую базу с именем FileName

void closeDB(memDB * DB);    // Закрывает открытую базу данных

void rewindFirstNodes(memDB * DB, memNodeSchemeRecord * NodeScheme);  // Переместить внутренний указатель множества узлов на первый узел

// Перемещает внутренний указатель множества узлов к следующему узлу и возвращает не ноль.
// Если же текущий узел = последний, возвращает ноль
int nextNode(memDB * DB, memNodeSchemeRecord * NodeScheme);

int openNode(memDB * DB, memNodeSchemeRecord * NodeScheme);   // Открывает текущий узел, загружая его данные в буфер

void createNode(memDB * DB, memNodeSchemeRecord * NodeScheme);   // Добавляет новый узел, заполняя его нулями

void cancelNode(memDB * DB, memNodeSchemeRecord * NodeScheme);    // Отменяет результаты редактирования текущего узла

int deleteNode(memDB * DB, memNodeSchemeRecord * NodeScheme);     // Удаляет текущий узел. Возвращает ненулевое значение при успехе

void setNodeAttr(memDB * DB, memNodeSchemeRecord * NodeScheme, char * AttrName, float Value);  // Устанавливает значение атрибута текущего узла

float getNodeAttr(memDB * DB, memNodeSchemeRecord * NodeScheme, char * AttrName); // Загружает значение атрибута текущего узла

int createString(memDB * DB, char * S);   // Создает в базе новую строку и возвращает ее смещение от начала файла

char * getString(memDB * DB, int Offset);   // Загружает из базы новую строку по ее смещению. Строка создается в динамической памяти


// Возвращает список из *n пар вида (RootOffset; NodeOffset), представляющих узлы,
// в которые идут дуги из текущего узла
float * getDirectedToList(memDB * DB, memNodeSchemeRecord * NodeScheme, int * n);

void setNodeAttr(memDB * DB, memNodeSchemeRecord * NodeScheme, char * AttrName, float Value);  // Устанавливает значение атрибута текущего узла

float getNodeAttr(memDB * DB, memNodeSchemeRecord * NodeScheme, char * AttrName); // Загружает значение атрибута текущего узла


// Направляет дугу из текущего узла множества NodeSchemeFrom к текущему узлу множества NodeSchemeTo
// Узел From должен быть открыт для редактирования
// Возвращает ненулевое значение при успехе
int LinkCurrentNodeToCurrentNode(memDB * DB, memNodeSchemeRecord * NodeSchemeFrom, memNodeSchemeRecord * NodeSchemeTo);


void postNode(memDB * DB, memNodeSchemeRecord * NodeScheme);  // Сохраняет новый или отредактированный узел (из текущего буфера) в базе

// Создает логическое унарное или бинарное условие вида (operand1 [operation] operand2) или ([operation] operand1)
memCondition * createLogicCondition(unsigned char operation, memCondition * operand1, memCondition * operand2);
// Условия-отношения на атрибуты
memCondition * createStringAttrCondition(unsigned char operation, char * AttrName, char * Val);

memCondition * createIntOrBoolAttrCondition(unsigned char operation, char * AttrName, int Val);

memCondition * createFloatAttrCondition(unsigned char operation, char * AttrName, float Val);

void freeOperand(memConditionOperand * op);    // Удаляет из памяти операнд условия

void freeCondition(memCondition * Cond);      // Удаляет из памяти условие

int testNodeCondition(memDB * DB, memNodeSchemeRecord * NodeScheme, memCondition * Condition);  // Тестирует открытый текущий узел типа NodeScheme на условие Condition

memNodeSetItem * queryAllNodesOfType(memDB * DB, memNodeSchemeRecord * NodeScheme, memCondition * Condition);   //Выбирает все узлы типа NodeScheme, подпадающие под условие Condition

memNodeSetItem * queryNodeSet(memDB * DB, memNodeSetItem * NodeSet, memCondition * Condition);  // Выбирает все узлы из результатов запроса NodeSet, подпадающие под условие Condition

// Выбирает все узлы, соответствующие запросу в Cypher-стиле длиной nLinks элементов
// Каждый элемент запроса включает ссылку на тип узла и наложенное на него условие
memNodeSetItem * queryCypherStyle(memDB * DB, int nLinks, ...);

// Удаляет все конечные узлы, соответствующие запросу в Cypher-стиле длиной nLinks элементов
//Каждый элемент запрса включает ссылку на тип узла и наложенное на него условие
// Перемещает внутренний указатель множества узлов типа NodeSet->NodeScheme к узлу,
// специфицированному элементом NodeSet.
void navigateByNodeSetItem(memDB * DB, memNodeSetItem * NodeSet);
void deleteCypherStyle(memDB * DB, int nLinks, ...);

// Модифицирует все конечные узлы, соответствующие запросу в Cypher-стиле длиной nLinks элементов
// Каждый элемент запроса включает ссылку на тип узла и наложенное на него условие
// AttrName = имя атрибута
// AttrVal = числовое/логическое значение или смещение записи-строки в базу (возвращается ф-цией createString)
void setCypherStyle(memDB * DB, char * AttrName, float AttrVal, int nLinks, ...);


//Освобождает результаты запроса NodeSet
void freeNodeSet(memDB * DB, memNodeSetItem * NodeSet);


#endif //LLP_LAB1_GRAPH_STRUCT_H
