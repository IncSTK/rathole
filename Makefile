CC 		= gcc
FLAGS 		= -O3 -static
OBJS 		= rat.o hole.o blowfish.o

.c.o:
		${CC} ${FLAGS} -c $<

all:		hole rat

hole: 	${OBJS}
		${CC} ${FLAGS} -o hole hole.o blowfish.o
		strip hole

rat:	 	${OBJS}
		${CC} ${FLAGS} -o rat rat.o blowfish.o
		strip rat

clean:
		rm -f rat hole *.o *~
		@echo Much better now.
