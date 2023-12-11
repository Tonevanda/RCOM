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

run_test_timestamp: $(TARGET)
	./$(TARGET) ftp://ftp.up.pt/pub/kodi/timestamp.txt

run_test_pub: $(TARGET)
	./$(TARGET) ftp://netlab1.fe.up.pt/pub.txt

run_test_pipe: $(TARGET)
	./$(TARGET) ftp://rcom:rcom@netlab1.fe.up.pt/pipe.txt

run_test_arch: $(TARGET)
	./$(TARGET) ftp://rcom:rcom@netlab1.fe.up.pt/files/archlinux-2023.03.01-x86_64.iso

run_test_crab: $(TARGET)
	./$(TARGET) ftp://rcom:rcom@netlab1.fe.up.pt/files/crab.mp4

run_test_pic1: $(TARGET)
	./$(TARGET) ftp://rcom:rcom@netlab1.fe.up.pt/files/pic1.jpg

run_test_pic2: $(TARGET)
	./$(TARGET) ftp://rcom:rcom@netlab1.fe.up.pt/files/pic2.png

clean:
	rm -f $(TARGET)
	rm -f *.txt *.jpg *.png *.mp4 *.iso