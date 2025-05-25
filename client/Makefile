CC = gcc
CFLAGS = -Wall -Wextra -I../include
LDFLAGS = -L../libmysyslog -lmysyslog
TARGET = myRPC-client
SRC = myRPC-client.c
INSTALL_DIR = /usr/local

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

install:
	install -D -m 755 myRPC-client $(DESTDIR)/usr/bin/myRPC-client

clean:
	rm -f $(TARGET)
