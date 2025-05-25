# Главный Makefile
.PHONY: all client server install deb clean

all: client server

client:
	$(MAKE) -C client

server:
	$(MAKE) -C server

install:
	$(MAKE) -C libmysyslog install
	$(MAKE) -C server install
	$(MAKE) -C client install

deb: prepare-deb
	# Сборка клиентского пакета
	dpkg-deb --build packaging/deb/client
	mv packaging/deb/client.deb myRPC-client.deb
	
	# Сборка серверного пакета
	dpkg-deb --build packaging/deb/server
	mv packaging/deb/server.deb myRPC-server.deb

prepare-deb:
	@echo "Подготовка структуры пакетов..."
	# Клиентский пакет
	mkdir -p packaging/deb/client/DEBIAN
	cp packaging/client.control packaging/deb/client/DEBIAN/control
	mkdir -p packaging/deb/client/usr/bin
	test -f client/myRPC-client && cp client/myRPC-client packaging/deb/client/usr/bin/ || (echo "Ошибка: файл client/myRPC-client не найден"; exit 1)
	
	# Серверный пакет
	mkdir -p packaging/deb/server/DEBIAN
	cp packaging/server.control packaging/deb/server/DEBIAN/control
	mkdir -p packaging/deb/server/usr/bin
	mkdir -p packaging/deb/server/etc/myRPC
	mkdir -p packaging/deb/server/lib/systemd/system
	test -f server/myRPC-server && cp server/myRPC-server packaging/deb/server/usr/bin/ || (echo "Ошибка: файл server/myRPC-server не найден"; exit 1)
	test -f server/myRPC.conf && cp server/myRPC.conf packaging/deb/server/etc/myRPC/ || (echo "Ошибка: файл server/myRPC.conf не найден"; exit 1)
	test -f server/users.conf && cp server/users.conf packaging/deb/server/etc/myRPC/ || (echo "Ошибка: файл server/users.conf не найден"; exit 1)
	test -f systemd/myRPC-server.service && cp systemd/myRPC-server.service packaging/deb/server/lib/systemd/system/ || (echo "Ошибка: файл systemd/myRPC-server.service не найден"; exit 1)

clean:
	$(MAKE) -C client clean
	$(MAKE) -C server clean
	rm -rf packaging/deb *.deb
