CC = g++
CFLAGS = -shared -fPIC
TARGET = ../std.so
SRC = math.cpp io.cpp
##g++ -shared -fPIC math.cpp -o math.so
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f *.o std.so