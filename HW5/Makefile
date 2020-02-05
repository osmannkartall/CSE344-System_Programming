all: 141044036_HW5

141044036_HW5: 141044036_main.o
	gcc 141044036_main.o -o exe -lpthread -lm	

141044036_main.o: 141044036_main.c
	gcc -c 141044036_main.c -lpthread -lm

clean: 
	rm *.o exe
