# Low-level programming lab1 
### Вариант 3: Граф узлов с атрибутами (Cypher)
  
#### 1.Цели
Создать модуль, реализующий хранение в одном файле данных в виде графа узлов с атрибутами общим объёмом от 10GB.
#### 2.Задачи
+ Спроектировать структуры данных для представления информации в оперативной памяти  
+ Спроектировать представление данных с учетом схемы для файла данных и реализовать базовые операции для работы с ним:  
    + Операции над схемой данных (создание и удаление элементов)    
    + Базовые операции над элементами данными (вставка, перечисление, обновление, удаление)     
+ Реализовать публичный интерфейс для приведенных выше операций  
+ Реализовать тестовую программу для демонстрации работоспособности решения  
    
 #### 3. Описание работы  
 Программа представляет собой следующие модули:
+ graphdb  (содержит все операции: над элементами данных, над схемой, взаимодейтсвие с БД, и работа с файлом БД)
+ functions_demo (иллюстрируется все функции)
+ measurings (выполняются измерения)
 
 ##### Подключение к БД 
+ создание бд с помощью метода createNewDBbyScheme:
 ```C
 DB = createNewDBbyScheme(Scheme, "graphs.mydb");
 ```
+ закрытие с помощью метода closeDB
 ```C
 closeDB(DB);
 ```
 ##### Реализация запроса:
 Все запросы построены по типу Cypher-запроса
+ пример запроса:
```C
    // MATCH (j:Movie)-[:DIRECTED]->(a:Actor) WHERE (j.Year < 2004) AND (a.Family != 'Pitt') AND (a.Family != 'Hamatova') RETURN a;
    printf("MATCH (j:Movie)-[:DIRECTED]->(a:Actor) WHERE (j.Year < 2004) AND (a.Family != 'Pitt') AND (a.Family != 'Hamatova') RETURN a;  =>\n");
    ns2 = queryCypherStyle(DB, 2, MovieNodeType, cond, ActorNodeType, cond2);
    ns12 = ns2;
    i = 0;
    while (ns12 != NULL) {
        navigateByNodeSetItem(DB, ns12);
        if (openNode(DB, ActorNodeType)) {
            char * Family = getString(DB, getNodeAttr(DB, ActorNodeType, "Family"));
            printf("%s [%i]\n", Family, (int)getNodeAttr(DB, ActorNodeType, "Year_of_birthday"));
            register_free(strlen(Family)+1);
            free(Family);
            cancelNode(DB, ActorNodeType);
        } else
            printf("Can't open actor node!\n");
        ns12 = ns12->next;
        i++;
    }
    freeNodeSet(DB, ns2);
    printf("%i actors selected!\n", i);  
```
+ пример вывода:
```
Stepanov [1980]
Churikova [1982]
2 actors selected!
```
+ Результатом запроса может быть:
```C
typedef struct memNodeSetItem { 
   memNodeSchemeRecord * NodeScheme; // Ссылка на тип узла
   int PrevOffset; // Смещение предыдущего узла этого типа
   int ThisOffset; // Смещение текущего узла этого типа
   struct memNodeSetItem * next; // Следующий элемент
   struct memNodeSetItem * prev; // Предыдущий элемент
} memNodeSetItem;
```
##### Используемые структуры:
+ Схема:
```C
typedef struct { 
   memNodeSchemeRecord * FirstSchemeNode; // Указатель на описатель первого типа узла
   memNodeSchemeRecord * LastSchemeNode; // Указатель на описатель последнего типа узла
} memDBScheme;
  
typedef struct { 
   memDBScheme * Scheme;
   char * WriteBuffer; // Буфер записи
   int nWriteBuffer;
   char * ReadBuffer; // Буфер чтения
   int nReadBuffer;
   int iReadBuffer;
   FILE * FileDB;
} memDB; 
```
+ Атрибут:
```C
typedef struct memAttrRecord { 
   char * NameString; // Запись с именем атрибута
   unsigned char Type; // Тип атрибута
   struct memAttrRecord * next; // Указатель на следующий атрибут
} memAttrRecord;
```
+ Тип узла:
```C
typedef struct memNodeSchemeRecord { 
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
```
+ Операнд
```C
typedef struct { 
   unsigned char OperandType; // тип операнда
   union {
       struct memCondition * opCondition; // Другое условие
       char * opString; // Строка
       float opInt_Bool_Float; // Целое число или логическое значение или вещественное число
       char * opAttrName;
   };
} memConditionOperand;
```
+ Элемент условия:
```C
typedef struct memCondition { 
   unsigned char OperationType; // Операция
   memConditionOperand * Operand1; // Первый операнд
   memConditionOperand * Operand2; // Второй операнд (или NULL, если операция унарная)
} memCondition;
```
+ Возможные типы данных:
```C
enum { tpInt32 = 0, tpFloat, tpString, tpBoolean } tpDataItems; 
```
#### 4. Аспекты реализации 
#### 5. Результаты
##### Insertion
выполняется за O(1) независимо от размера данных, представленных в файле
![Alt-текст](https://github.com/1KarinaV/llp_lab1/blob/master/img/insert.jpg)
##### Select
Операция выборки без учёта отношений (но с опциональными условиями) выполняется за O(n), где n – количество строк
![Alt-текст](https://github.com/1KarinaV/llp_lab1/blob/master/img/select.jpg)  
   
##### Update and Delete   
Операции обновления и удаления элемента данных выполняются не более чем за O(n*m) > t -> O(n+m), где n – количество представленных элементов данных обрабатываемого вида, m – количество фактически затронутых элементов данных
  
  ![Alt-текст](https://github.com/1KarinaV/llp_lab1/blob/master/img/update.jpg)
  ![Alt-текст](https://github.com/1KarinaV/llp_lab1/blob/master/img/delete.jpg)
#### 6. Вывод

