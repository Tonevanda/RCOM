CC = gcc
CFLAGS = -Wall
SRC_DIR = src
BIN_DIR = bin
TARGET = $(BIN_DIR)/download

all: $(TARGET)

$(TARGET): $(SRC_DIR)/download.c
	$(CC) $(CFLAGS) -o $@ $<

run: $(TARGET)
	./$(TARGET) $(filter-out $@,$(MAKECMDGOALS))

clean:
	rm -f $(TARGET)
