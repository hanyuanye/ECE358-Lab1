all: main run graph

main: main.o
	g++ main.cpp -o main.out

run:
	./main.out 0
	./main.out 1

graph:
	gnuplot q3_graph1 q3_graph2 q6_graph1 q6_graph2
