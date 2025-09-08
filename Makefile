CC = g++
CFLAGS = -Wall -O3 -std=c++20 -lffi
TARGET = main
SRC = main.cpp compiler/lexer.cpp compiler/parser.cpp compiler/compiler.cpp compiler/vm.cpp
##g++ -shared -fPIC math.cpp -o math.so
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f *.o main