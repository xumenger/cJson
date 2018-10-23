test : cJson.o test.o
	gcc -Wall -g test.o cJson.o -o test

cJson.o : ../src/cJson.c ../src/cJson.h ../src/cJsonStruct.h
	gcc -Wall -g -c ../src/cJson.c -o cJson.o

test.o : test.c
	gcc -Wall -g -c test.c -o test.o

.PHONY : clean
clean: 
	rm -f *.o