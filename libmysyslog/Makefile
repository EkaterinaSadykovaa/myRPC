# Настройки компиляции
CC = gcc
CFLAGS = -Wall -Wextra -fPIC -I../include
TARGET = libmysyslog.so
SRC = mysyslog.c
INSTALL_DIR = /usr/local

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -shared -o $@ $^

install:
	install -D -m 755 $(TARGET) $(INSTALL_DIR)/lib/$(TARGET)
	install -D -m 644 ../include/mysyslog.h $(INSTALL_DIR)/include/
	ldconfig

clean:
	rm -f $(TARGET)
