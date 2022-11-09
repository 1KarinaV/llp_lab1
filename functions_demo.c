//
// Created by Карина Владыкина on 08.11.2022.
//

// Программа, иллюстрирующая все функции

#include "graph_struct.h"

int main () {
    memDBScheme *Scheme;
    memDB * DB;
    memNodeSchemeRecord * MovieNodeType;
    int i;
    memNodeSchemeRecord * DeletedNodeType;

    Scheme = createDBScheme();

    MovieNodeType = addNodeTypeToScheme(Scheme, "Movie");

    DeletedNodeType = addNodeTypeToScheme(Scheme, "Deleted");

    delDirectedToFromNodeType(MovieNodeType, DeletedNodeType);
    delNodeTypeFromScheme(Scheme, DeletedNodeType);
    delAttrFromNodeScheme(MovieNodeType, findAttrByName(MovieNodeType, "ToDelete", &i));

    DB = createNewDBbyScheme(Scheme, "graphs.mydb");

    closeDB(DB);
}