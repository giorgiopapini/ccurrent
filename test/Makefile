# Compiler and flags
CC = gcc  # clang is also supported

CFLAGS = -g -O2
#CFLAGS += -Wpedantic -pedantic-errors
CFLAGS += -Werror
CFLAGS += -Wall
CFLAGS += -Wextra

# Only for debugging!
LDFLAGS += -fsanitize=address,undefined
CFLAGS += -fsanitize=address,undefined
CFLAGS += -O0

SRC != find . -name '*.c'
OBJ = ${SRC:.c=.o}

TARGET = tests

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)