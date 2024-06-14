SOURCES=$(wildcard *.cpp)
HEADERS=$(SOURCES:.cpp=.h)
FLAGS=-g

all: main

main: $(SOURCES) $(HEADERS) Makefile
	mpicxx $(SOURCES) $(FLAGS) -o main

debug: $(SOURCES) $(HEADERS) Makefile
	mpicxx $(SOURCES) $(FLAGS) -DDEBUG -o main

clear: clean
clean:
	rm main
	

run: main Makefile
	mpirun -oversubscribe -np 9 ./main

runDebug: debug Makefile
	mpirun -oversubscribe -np 9 ./main