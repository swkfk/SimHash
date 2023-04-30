CC := gcc
CFLAG := -std=c11
OUT := simtool
PACK := src.zip
SRC := $(wildcard *.c)
HEADER := ${wildcard *.h}
OBJ := $(patsubst %.c,%.o,${SRC})

.PHONY: clean
.PHONY: clear
.PHONY: pack
.PHONY: show

%.o: %.c
	${CC} ${CFLAG} -c -o $@ $<

${OUT}: ${OBJ} ${HEADER}
	${CC} ${CFLAG} -o $@ ${OBJ}

clean:
	rm -f *.o ${OUT} ${PACK}

clear:
	rm -f *.o

pack:
	zip -0 ${PACK} *.c *.h makefile

show:
	@git br
	@echo "********************************"
	@wc -l *.c *.h makefile
