CC = gcc
LIBS = -lmagic
CFLAGS = -I./include -Wall -g -O0
SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c, build/%.o, $(SRC))
TARGET = a.out

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LIBS)

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build $(TARGET)

run: $(TARGET)
	./$(TARGET)
