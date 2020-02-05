all: 141044036_HW1

141044036_HW1: 141044036_main.o
	gcc -std=c11 141044036_main.o -o exe -lm	

141044036_main.o: 141044036_main.c
	gcc -std=c11 -c 141044036_main.c -Wall -lm

clean: 
	rm *.o exe
