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
+ functions_demo 
+ graph_struct 
+ meaurings 
 
 ##### Подключение к БД 
 
 ##### Запрос создается с помощью стуктуры Query:

+ пример запроса:
```C
    // Запрос построен по типу Cypher-запроса:
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
typedef struct memNodeSetItem { // Структура-элемент результата запроса к базе данных
   memNodeSchemeRecord * NodeScheme; // Ссылка на тип узла
   int PrevOffset; // Смещение предыдущего узла этого типа
   int ThisOffset; // Смещение текущего узла этого типа
   struct memNodeSetItem * next; // Следующий элемент
   struct memNodeSetItem * prev; // Предыдущий элемент
} memNodeSetItem;
```
