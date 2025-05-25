DEB_DIR = deb
BUILD_DIR = ..

.PHONY: all client server

all: client server

client:
	mkdir -p $(DEB_DIR)/client/usr/bin
	cp $(BUILD_DIR)/client/myRPC-client $(DEB_DIR)/client/usr/bin/
	dpkg-deb --build $(DEB_DIR)/client

server:
	mkdir -p $(DEB_DIR)/server/{usr/bin,etc/myRPC,lib/systemd/system}
	cp $(BUILD_DIR)/server/myRPC-server $(DEB_DIR)/server/usr/bin/
	cp $(BUILD_DIR)/server/myRPC.conf $(DEB_DIR)/server/etc/myRPC/
	cp $(BUILD_DIR)/server/users.conf $(DEB_DIR)/server/etc/myRPC/
	cp $(BUILD_DIR)/systemd/myRPC-server.service $(DEB_DIR)/server/lib/systemd/system/
	dpkg-deb --build $(DEB_DIR)/server

clean:
	rm -rf $(DEB_DIR)
