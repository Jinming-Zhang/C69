SRCS = simpleloop.c matmul.c blocked.c addition.c
PROGS = simpleloop matmul blocked addition

all : $(PROGS)

$(PROGS) : % : %.c
	gcc -Wall -g -o $@ $<


traces: $(PROGS)
	./runit simpleloop
	./runit matmul 100
	./runit blocked 100 25
	./runit addition

.PHONY: clean
clean : 
	rm -f simpleloop matmul blocked addition tr-*.ref *.marker *~
