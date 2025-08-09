CC = g++
CFLAGS = -Wall -g -std=c++20
TARGET = main
SRC = main.cpp compiler/lexer.cpp compiler/parser.cpp compiler/compiler.cpp

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f *.o main