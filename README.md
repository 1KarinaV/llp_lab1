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
 ##### Создание и заполнение схемы таблицы:
  ```C
 Scheme = createDBScheme();

 MovieNodeType = addNodeTypeToScheme(Scheme, "Movie");
 addAttrToNodeScheme(MovieNodeType, "Title", tpString);
 addAttrToNodeScheme(MovieNodeType, "Year", tpInt32);
 ActorNodeType = addNodeTypeToScheme(Scheme, "Actor");
 addAttrToNodeScheme(ActorNodeType, "Family", tpString);
 addAttrToNodeScheme(ActorNodeType, "Name", tpString);
 addAttrToNodeScheme(ActorNodeType, "Oscar", tpBoolean);
 addAttrToNodeScheme(ActorNodeType, "Year_of_birthday", tpInt32);
  
 addDirectedToNodeScheme(MovieNodeType, ActorNodeType);
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
   int PrevOffset; 
   int ThisOffset; 
   struct memNodeSetItem * next; // Следующий элемент
   struct memNodeSetItem * prev; // Предыдущий элемент
} memNodeSetItem;
```
##### Используемые структуры:
+ Схема:
```C
typedef struct { 
   memNodeSchemeRecord * FirstSchemeNode; 
   memNodeSchemeRecord * LastSchemeNode; 
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
   unsigned char Type; 
   struct memAttrRecord * next; // Указатель на следующий атрибут
} memAttrRecord;
```
+ Тип узла:
```C
typedef struct memNodeSchemeRecord { 
   char * TypeString; 
   int RootOffset; 
   int FirstOffset; 
   int LastOffset; 
   char * Buffer; 
   int nBuffer; 
   int added; 
   int PrevOffset; 
   int ThisOffset; 
   memNodeDirectedTo * DirectedToFirst; 
   memNodeDirectedTo * DirectedToLast; 
   struct memAttrRecord * AttrsFirst; 
   struct memAttrRecord * AttrsLast; 
   struct memNodeSchemeRecord * NextNodeScheme; 
} memNodeSchemeRecord;
```
+ Операнд
```C
typedef struct { 
   unsigned char OperandType; 
   union {
       struct memCondition * opCondition; // Другое условие
       char * opString; 
       float opInt_Bool_Float; 
       char * opAttrName;
   };
} memConditionOperand;
```
+ Элемент условия:
```C
typedef struct memCondition { 
   unsigned char OperationType; // Операция
   memConditionOperand * Operand1; 
   memConditionOperand * Operand2; 
} memCondition;
```
+ Возможные типы данных:
```C
enum { tpInt32 = 0, tpFloat, tpString, tpBoolean } tpDataItems; 
```
#### 4. Аспекты реализации 
База данных хранится в файле, объемом до 10ГБ. 
Бд построена на графах - узлах и дугах.    
Объектом БД является Node. У узлов есть разные типы. Так, например, ноды типа movie связаны с нодами типа actor. В схему новый тип узла добавляется с помощью:
```C
memNodeSchemeRecord * addNodeTypeToScheme(memDBScheme * Scheme, char * TypeName);
```
А для того, чтобы показать связь между узлами используются дуги.  
У каждого узла есть набор атрибутов (свойств), каждый из которых содержит в себе запись с именем, тип и указатель на следующий атрибут.    
2. Для создания БД сначала нужно создать схему БД, которая содержит указатели на первый и последний типы узла. 
После того, как была создана бд, функция возвращает указатель на структуру данных, представляющую созданную базу.    
3. Когда создаётся новый экземпляр узла вызовом функции createNode,он записывается в БД.   
4. Для этого создаётся строка (после будет возвращено ее смещение от начала файла), в нее записывается экземпляр узла со всеми атрибутами. И данная строка записывается в файл
```C
createNode(DB, MovieNodeType);
setNodeAttr(DB, MovieNodeType, "Title", createString(DB, Titles[i/2]));
setNodeAttr(DB, MovieNodeType, "Year", 2000 + i);
postNode(DB, MovieNodeType);
```
В случае, если не удастся провести дугу между двумя узлами, будет выведено соотвествующее сообщение:
```C
if (!LinkCurrentNodeToCurrentNode(DB, MovieNodeType, ActorNodeType))
        printf("Can't connect!\n");
        postNode(DB, MovieNodeType);
```
4. После этого внутренний указатель множества узлов перемещается на первый узел
```C
rewindFirstNodes(DB, MovieNodeType);
```
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
  ![IMG_4515](https://user-images.githubusercontent.com/84856275/204339541-e51fdeec-9218-4a1e-8f31-fb4a0acee160.jpeg)
  
#### Сборка
+ Linux  
```
git clone https://github.com/1KarinaV/llp_lab1  
cd llp_lab1  
make all  
./functions_demo  
./measurings  
```
+ Windows  
```
git clone https://github.com/1KarinaV/llp_lab1  
cd llp_lab1  
```
Необходимо оставить файл Makefile.win и удалить расширение
```
make all 
functions_demo.exe && measurings.exe 
```
#### 6. Вывод
В ходе работы была реализована графовая база данных ,поддерживающая операции Create, Insert, Delete, Update между вершинами и ребрами. Так же были разработаны тесты, в результате выполнения которых, было продемонстрировано, что реализация операция была выполнена с помощью правильных алгоритмов, потому что по графикам видно, что алгоритмическая сложность совпала с ожидаемой. 
