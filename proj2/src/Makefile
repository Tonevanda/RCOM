CC = gcc
CFLAGS = -Wall
DEPS = download.c
OBJ = download.o
EXECUTABLE=download

%.o: %.c $(DEPS)
    $(CC) -c -o $@ $< $(CFLAGS)

$(EXECUTABLE): $(OBJ)
    $(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
    rm -f $(OBJ) $(EXECUTABLE)