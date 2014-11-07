assignment5: assignment5.c
	gcc -std=c99 -Wall -O3 assignment5.c -o assignment5

run:
	./assignment5 $(filename)

clean:
	rm -f assignment5
