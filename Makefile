all: local_intf.o
	gcc -Wall -ggdb local_intf.o -o local_intf

local_intf.o: local_intf.c
	gcc -ggdb -c local_intf.c

clean:
	rm *.o local_intf
