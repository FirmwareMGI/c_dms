CC = gcc
CFLAGS = $(shell mariadb_config --cflags)
LDFLAGS = $(shell mariadb_config --libs)
SRC = test2.c
OBJ = $(SRC:.c=.o)
OUT = connector

$(OUT): $(OBJ)
	$(CC) $(OBJ) -o $(OUT) $(LDFLAGS)

$(OBJ): $(SRC)
	$(CC) -c $(SRC) $(CFLAGS)

clean:
	rm -f $(OBJ) $(OUT)

# gcc main.c -o my_program -lmariadbclient -lMQTTClient -liec61850