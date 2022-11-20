all: functions_demo

functions_demo: functions_demo.o graphdb.o
	gcc functions_demo.o graphdb.o -o demo

demo.o: functions_demo.c
	gcc -c functions_demo.c

graphdb.o: graphdb.c
	gcc -c graphdb.c