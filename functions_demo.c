//
// Created by Карина Владыкина on 08.11.2022.
//

// Программа, иллюстрирующая все функции

#include "graph_struct.h"

int main () {
    char * Titles[5] = {"Mist", "Prometeus", "Flew over the cookoo's nest", "Shawshank redemption", "AfterNoon"};
    char * Families[10] = {"Stepanov", "Hamatova", "Churikova", "Pitt", "Delon", "Williams", "Nickolson", "Boyarskaya", "Freeman", "De Vito"};
    memDBScheme *Scheme;
    memDB * DB;
    memNodeSchemeRecord * MovieNodeType;
    memNodeSchemeRecord * ActorNodeType;
    memNodeSchemeRecord * DeletedNodeType;
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


    closeDB(DB);
}