CC = gcc

all: library exec

library: build library

build: build/as_relationship.o build/hashtable.o build/hashset.o build/hash.o build/display.o | mkbuild

exec: bin/main
	LD_PRELOAD=/usr/local/lib/libigraph.so bin/main ./dataset/test.txt

test: bin/graph_analysis
	LD_PRELOAD=/usr/local/lib/libigraph.so bin/graph_analysis ./dataset/test.txt

exec2: bin/graph_analysis
	LD_PRELOAD=/usr/local/lib/libigraph.so bin/graph_analysis ./dataset/test.txt

# Directory dove il compilatore trova gli header files
INCLUDES = -Iinclude -I/usr/local/include/igraph

# Librerie da linkare
LIB = -L. -lcga -L/usr/local/lib -ligraph -lpthread

# Flags per il compilatore
CFLAGS = $(INCLUDES) -Wall -pedantic

# Lista degli header files
HEADERS = include/*.h

# Ricompila tutti i target se cambiano gli header o questo Makefile
COMMON_DEPS = $(HEADERS) Makefile

# Regola per compilare un .c in .o
build/%.o: src/%.c $(COMMON_DEPS) | mkbuild
	$(CC) -c $< -o $@ $(CFLAGS)

# Crea build folder se non esiste
mkbuild:
	mkdir build -p

bin/graph_analysis: build/graph_analysis.o build/as_relationship.o build/hashtable.o build/hashset.o build/hash.o build/display.o $(COMMON_DEPS) | mkbin
	$(CC) -o bin/graph_analysis build/graph_analysis.o build/as_relationship.o build/hashtable.o build/hashset.o build/hash.o build/display.o -L/usr/local/lib -ligraph -lpthread

# graph_analysis.o é un esempio di file contenente il programma principale
bin/main: build/graph_analysis.o $(COMMON_DEPS) | mkbin
	$(CC) -o bin/main build/graph_analysis.o $(LIB)
# Crea bin folder se non esiste
mkbin:
	mkdir bin -p

# crea libreria statica
library:
	ar rcs libcga.a build/as_relationship.o build/hashtable.o build/hashset.o build/hash.o build/display.o

clean:
	rm -f build/* bin/*