assignment5: assignment5.o
	gcc -Wall -O3 assignment5.o -o assignment5

assignment5.o: assignment5.c
	gcc -Wall -O3 -c assignment5.c

run:
	./assignment5

clean:
	rm -rf *.o
	rm -f assignment5
