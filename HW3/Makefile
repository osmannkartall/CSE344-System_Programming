all: 141044036_HW3

141044036_HW3: 141044036_main.o ls.o cat.o wc.o
	gcc 141044036_main.o -o exe -lm	

141044036_main.o: 141044036_main.c ls.c cat.c wc.c
	gcc -c 141044036_main.c -lm

clean: 
	rm *.o exe
