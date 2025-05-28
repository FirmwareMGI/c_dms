# Makefile for MariaDB C program

CC = gcc
CFLAGS = -Wall
LIBS = -lmariadb

TARGET = db_read
SRC = db_read.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -f $(TARGET)
