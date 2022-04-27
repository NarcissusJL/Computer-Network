# Makefile for Project 3
#****************************************************************

COPTIONS = -g -Wall
LDOPTIONS = 
LIB = 

# ***************************************************************
# Entry to bring the package up to date
#    The "make all" entry should be the first real entry

all: proj3

l1.o: l1.c
	gcc $(COPTIONS) -c l1.c

l2.o: l2.c
	gcc $(COPTIONS) -c l2.c

l3.o: l3.c
	gcc $(COPTIONS) -c l3.c

l4.o: l4.c
	gcc $(COPTIONS) -c l4.c

proj3.o: proj3.c
	gcc $(COPTIONS) -c proj3.c

proj3: l1.o l2.o l3.o l4.o proj3.o
	gcc $(LDOPTIONS) -o proj3 l1.o l2.o l3.o l4.o proj3.o $(LIB)
	

# delete derived files in preparation for rebuild
clean:
	rm -f *.o proj3
