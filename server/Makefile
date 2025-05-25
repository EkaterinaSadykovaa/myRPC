CC = gcc
CFLAGS = -Wall -Wextra -I../include
LDFLAGS = -L../libmysyslog -lmysyslog
TARGET = myRPC-server
SRC = myRPC-server.c
CONFIG_DIR = /etc/myRPC
SERVICE_DIR = /etc/systemd/system
INSTALL_DIR = /usr/local

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

install:
	install -D -m 755 myRPC-server $(DESTDIR)/usr/local/bin/myRPC-server
	mkdir -p $(DESTDIR)/etc/myRPC  # Создаем директорию
	install -m 644 myRPC.conf $(DESTDIR)/etc/myRPC/
	install -m 644 users.conf $(DESTDIR)/etc/myRPC/
	install -D -m 644 ../systemd/myRPC-server.service $(DESTDIR)/etc/systemd/system/myRPC-server.service
	systemctl daemon-reload

clean:
	rm -f $(TARGET)
