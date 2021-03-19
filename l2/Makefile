all: main run graph

main: main.o
	g++ main.cpp -o main.o

run:
	./main.o

graph:
	gnuplot persistentEf_graph persistentTh_graph NpersistentEf_graph NpersistentTh_graph