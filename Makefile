# Makefile

CC = gcc  # Compiler
CFLAGS = -g \
         -Isrc \
         -Ilib/libiec61850/config \
         -Ilib/libiec61850/hal/inc \
         -Ilib/libiec61850/src/common/inc \
         -Ilib/libiec61850/src/mms/inc \
         -Ilib/libiec61850/src/mms/inc_private \
         -Ilib/libiec61850/src/mms/asn1 \
         -Ilib/libiec61850/src/iec61850/inc \
         -Ilib/libiec61850/src/iec61850/inc_private \
         -Ilib/libiec61850/src/goose \
         -Ilib/libiec61850/src/sampled_values \
         -Ilib/libiec61850/src/logging \
         -Ilib/libiec61850/src/tls \
         -Ilib/libiec61850/src/r_session \
         $(shell mariadb_config --cflags)
LIBS = lib/libiec61850/build/src/libiec61850.so \
       -lpthread \
       -L/usr/local/lib/ \
       -lpaho-mqtt3c \
       $(shell mariadb_config --libs)

TARGET = dms
SOURCE = test2.c src/database.c src/list_utils.c
# MODULE_SOURCE = src/database.c
OBJECTS = $(SOURCE:.c=.o)
# MODULE_OBJECTS = $(MODULE_SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS) $(MODULE_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) -Wl,-rpath,/usr/local/lib/arm-linux-gnueabihf

%.o: %.c src/*.h # Dependency on header file
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJECTS) $(MODULE_OBJECTS)