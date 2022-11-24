all: functions_demo measurings

functions_demo: functions_demo.o graphdb.o
	gcc functions_demo.o graphdb.o -o functions_demo

measurings: measurings.o graphdb.o
	gcc measurings.o graphdb.o -o measurings

functions_demo.o: functions_demo.c
	gcc -c functions_demo.c

measurings.o: measurings.c
	gcc -c measurings.c

graphdb.o: graphdb.c
	gcc -c graphdb.c

clean:
	rm -rf *.o; \
	rm functions_demo; \
	rm measurings
