all: main run graph

main: main.o
	g++ main.cpp -o main.out

run:
	./main.out

graph:
	gnuplot q3_graph1
