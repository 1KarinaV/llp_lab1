all: functions_demo

functions_demo: functions_demo.o graphdb.o
	gcc functions_demo.o graphdb.o -o functions_demo

demo.o: functions_demo.c
	gcc -c functions_demo.c

graphdb.o: graphdb.c
	gcc -c graphdb.c

clean:
	rm -rf *.o; \
	rm functions_demo