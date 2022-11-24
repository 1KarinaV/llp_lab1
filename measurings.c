//
// Created by Карина Владыкина on 24.11.2022.
//
// Программа, выполняющая измерения

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "graph_struct.h"

#define nMovies 5

#define max_rand_str_length 99

char * gen_rand_str() {
    int n = rand() % (max_rand_str_length+1);
    char * result = (char *) malloc(n+1);
    int i;
    for (i = 0; i < n; i++)
        result[i] = 'A' + (rand() % ('Z'-'A'+1));
    result[n] = 0;
    return result;
}

double seconds() {
#ifdef _WIN32
    return (1.0*clock()/CLK_TCK);
#else
    return (1.0*clock()/CLOCKS_PER_SEC);
#endif
}

int main() {
    char *Titles[nMovies] = {"Mist", "Prometeus", "Flew over the cookoo's nest", "Shawshank redemption", "AfterNoon"};
    memDBScheme *Scheme;
    memDB *DB;
    memNodeSchemeRecord *MovieNodeType;
    memNodeSchemeRecord *ActorNodeType;
    memCondition *cond;
    memNodeSetItem *ns;
    double start;
    int nLinks;
    int i, j;

    initGraphsRuntime("prog.cfg");

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

    DB = createNewDBbyScheme(Scheme, "graphs.mydb");
    printf("Memory consumption (insertions+updates+deletings) [O(1)]:\n");
    for (i = 0; i < nMovies*200; i++) {
        char * Family = gen_rand_str();
        if (i % 200 == 0) {
            printf("%i items : %i bytes\n", i, getOccupiedMemory());
            createNode(DB, MovieNodeType);
            setNodeAttr(DB, MovieNodeType, "Title", createString(DB, Titles[i/200]));
            setNodeAttr(DB, MovieNodeType, "Year", 2000 + i);
            postNode(DB, MovieNodeType);
            nLinks = 0;
        }
        createNode(DB, ActorNodeType);
        setNodeAttr(DB, ActorNodeType, "Family", createString(DB, Family));
        free(Family);
        setNodeAttr(DB, ActorNodeType, "Year_of_birthday", 1980 + i);
        postNode(DB, ActorNodeType);
        openNode(DB, MovieNodeType);
        if (nLinks < getMaxLinksNum()) {
            if (!LinkCurrentNodeToCurrentNode(DB, MovieNodeType, ActorNodeType))
                printf("Can't connect!\n");
            nLinks++;
        }
        postNode(DB, MovieNodeType);
        if ((i % 200) > 150)
            deleteNode(DB, ActorNodeType);
    }

    printf("Timings (insertions) [O(1)]:\n");
    printf("File size is proportional to number of items inserted:\n");
    start = seconds();
    for (i = 0; i < nMovies*2000; i++) {
        char * Title = gen_rand_str();
        if (i % 2000 == 0) {
            printf("%i inserted : %lf seconds elapsed to insert 2000 items: fileSize = %li\n", i, (seconds() - start), getDBSize(DB));
            start = seconds();
        }
        createNode(DB, MovieNodeType);
        setNodeAttr(DB, MovieNodeType, "Title", createString(DB, Title));
        free(Title);
        setNodeAttr(DB, MovieNodeType, "Year", 2000 + i);
        postNode(DB, MovieNodeType);
    }

    // Удаляем все элементы типа MovieNodeType
    printf("Deleting... ");
    deleteCypherStyle(DB, 1, MovieNodeType, NULL);
    printf("done\n");

    cond = createIntOrBoolAttrCondition(opLess, "Year", 2004);

    printf("Timings of queries (selection, updating) [O(n)]:\n");
    for (i = 0; i < nMovies*4000; i++) {
        char * Title = gen_rand_str();
        if (i % 4000 == 0) {
            start = seconds();

            ns = queryCypherStyle(DB, 1, MovieNodeType, cond);
            freeNodeSet(DB, ns);

            printf("n = %i : Time to select = %lf [seconds]\n", i, (seconds() - start));

            start = seconds();

            setCypherStyle(DB, "Year", 1975, 1, MovieNodeType, cond);

            printf("n = %i : Time to update = %lf [seconds]\n", i, (seconds() - start));
        }
        createNode(DB, MovieNodeType);
        setNodeAttr(DB, MovieNodeType, "Title", createString(DB, Title));
        free(Title);
        setNodeAttr(DB, MovieNodeType, "Year", 2000 + i);
        postNode(DB, MovieNodeType);
    }

    // Удаляем все элементы типа MovieNodeType
    printf("Deleting... ");
    deleteCypherStyle(DB, 1, MovieNodeType, NULL);
    printf("done\n");

    printf("Timings of queries (deletion) [O(n)]:\n");
    for (j = 0; j < nMovies; j++) {
        for (i = 0; i < j*4000; i++) {
            char * Title = gen_rand_str();
            createNode(DB, MovieNodeType);
            setNodeAttr(DB, MovieNodeType, "Title", createString(DB, Title));
            free(Title);
            setNodeAttr(DB, MovieNodeType, "Year", 2000 + i);
            postNode(DB, MovieNodeType);
        }
        start = seconds();

        deleteCypherStyle(DB, 1, MovieNodeType, cond);

        printf("n = %i : Time to delete = %lf [seconds]\n", j*4000, (seconds() - start));
    }

    closeDB(DB);
    freeCondition(cond);
    // Проверяем, корректно ли освобождена память
    if (getOccupiedMemory() == 0)
        printf("Memory is freed correctly!\n");
    else
        printf("Not freed: %i bytes!\n", getOccupiedMemory());

    return 0;
}