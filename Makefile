CC=gcc
CFLAGS=-Wall -g
PROG = rw_solution_two.o
EXE = rw_solution

run: $(EXE)
	./$(EXE) test_file.txt 5 7

all: $(PROG)
	$(CC) $(CFLAGS) -o $(EXE) $(PROG) -lpthread

clean:
	rm $(PROG) $(EXE)