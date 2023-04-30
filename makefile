CC := clang
CFLAG := -std=c11
OUT := simtool
PACK := src.zip
SRC := $(wildcard *.c)
OBJ := $(patsubst %.c,%.o,${SRC})

.PHONY: clean
.PHONY: clear
.PHONY: pack

%.o: %.c
	${CC} ${CFLAG} -c -o $@ $<

${OUT}: ${OBJ}
	${CC} ${CFLAG} -o $@ $^

clean:
	rm -f *.o ${OUT} ${PACK}

clear:
	rm -f *.o

pack:
	zip -0 ${PACK} *.c *.h makefile
