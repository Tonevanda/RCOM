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

run_test_1: $(TARGET)
	./$(TARGET) ftp://host.name/url-path

run_test_2: $(TARGET)
	./$(TARGET) ftp://joao:123@host.name/url-path

run_test_3: $(TARGET)
	./$(TARGET) ftp://ftp.up.pt/pub/kodi/timestamp.txt

run_test_4: $(TARGET)
	./$(TARGET) ftp://joao:123@ftp.up.pt/pub/CPAN/

clean:
	rm -f $(TARGET)