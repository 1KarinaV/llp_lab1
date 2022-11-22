//
// Created by Карина Владыкина on 08.11.2022.
//

// Программа, иллюстрирующая все функции
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graph_struct.h"

int main () {
    char * Titles[5] = {"Mist", "Prometeus", "Flew over the cookoo's nest", "Shawshank redemption", "AfterNoon"};
    char * Families[10] = {"Stepanov", "Hamatova", "Churikova", "Pitt", "Delon", "Williams", "Nickolson", "Boyarskaya", "Freeman", "De Vito"};
    memDBScheme *Scheme;
    memDB * DB;
    memNodeSchemeRecord * MovieNodeType;
    memNodeSchemeRecord * ActorNodeType;
    memNodeSchemeRecord * DeletedNodeType;
    memCondition * cond;
    memCondition * cond2;
    memNodeSetItem * ns;
    memNodeSetItem * ns1;
    memNodeSetItem * ns2;
    memNodeSetItem * ns12;
    int i;

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
    addAttrToNodeScheme(MovieNodeType, "ToDelete", tpBoolean);
    DeletedNodeType = addNodeTypeToScheme(Scheme, "Deleted");
    addAttrToNodeScheme(DeletedNodeType, "Signature", tpInt32);

    addDirectedToNodeScheme(MovieNodeType, DeletedNodeType);
    addDirectedToNodeScheme(MovieNodeType, ActorNodeType);
    delDirectedToFromNodeType(MovieNodeType, DeletedNodeType);
    delNodeTypeFromScheme(Scheme, DeletedNodeType);
    delAttrFromNodeScheme(MovieNodeType, findAttrByName(MovieNodeType, "ToDelete", &i));

    DB = createNewDBbyScheme(Scheme, "graphs.mydb");

    for (i = 0; i < 10; i++) {
        if (i % 2 == 0) {
            createNode(DB, MovieNodeType);
            setNodeAttr(DB, MovieNodeType, "Title", createString(DB, Titles[i/2]));
            setNodeAttr(DB, MovieNodeType, "Year", 2000 + i);
            postNode(DB, MovieNodeType);
        }
        createNode(DB, ActorNodeType);
        setNodeAttr(DB, ActorNodeType, "Family", createString(DB, Families[i]));
        setNodeAttr(DB, ActorNodeType, "Year_of_birthday", 1980 + i);
        postNode(DB, ActorNodeType);
        openNode(DB, MovieNodeType);
        if (!LinkCurrentNodeToCurrentNode(DB, MovieNodeType, ActorNodeType))
            printf("Can't connect!\n");
        postNode(DB, MovieNodeType);
    }
    rewindFirstNodes(DB, MovieNodeType);
    i = 0;
    while (openNode(DB, MovieNodeType)) {
        int Year;
        char * Title;
        Year = getNodeAttr(DB, MovieNodeType, "Year");
        Title = getString(DB, getNodeAttr(DB, MovieNodeType, "Title"));
        printf("%s [%i]\n", Title, Year);
        register_free(1 + strlen(Title));
        free(Title);
        nextNode(DB, MovieNodeType);
        i++;
    }
    printf("There is %i Movies\n", i);

    cond = createIntOrBoolAttrCondition(opLess, "Year", 2004);
    // Запрос построен по типу Cypher-запроса: MATCH (j:Movie) WHERE j.Year < 2004 RETURN j;
    ns = queryAllNodesOfType(DB, MovieNodeType, cond);
    ns1 = ns;
    i = 0;
    while (ns1 != NULL) {
        ns1 = ns1->next;
        i++;
    }
    freeNodeSet(DB, ns);
    printf("MATCH (j:Movie) WHERE j.Year < 2004 RETURN j;  =>  %i movies earlier 2004!\n", i);

    cond2 = createLogicCondition(
            opAnd,
            createStringAttrCondition(opNotEqual, "Family", "Pitt"),
            createStringAttrCondition(opNotEqual, "Family", "Hamatova")
    );
    // Запрос построен по типу Cypher-запроса:
    // MATCH (j:Movie)-[:DIRECTED]->(a:Actor) WHERE (j.Year < 2004) AND (a.Family != 'Pitt') AND (a.Family != 'Hamatova') RETURN a;
    ns2 = queryCypherStyle(DB, 2, MovieNodeType, cond, ActorNodeType, cond2);
    ns12 = ns2;
    i = 0;
    printf("MATCH (j:Movie)-[:DIRECTED]->(a:Actor) WHERE (j.Year < 2004) AND (a.Family != 'Pitt') AND (a.Family != 'Hamatova') RETURN a;  =>\n");
    while (ns12 != NULL) {
        navigateByNodeSetItem(DB, ns12);
        if (openNode(DB, ActorNodeType)) {
            char * Family = getString(DB, getNodeAttr(DB, ActorNodeType, "Family"));
            printf("%s [%i]\n", Family, (int) getNodeAttr(DB, ActorNodeType,

                                                          "Year_of_birthday"));
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



    closeDB(DB);
}