//
// Created by Карина Владыкина on 24.11.2022.
//
// Программа, выполняющая измерения

#include "graph_struct.h"

#define nMovies 5

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
}
