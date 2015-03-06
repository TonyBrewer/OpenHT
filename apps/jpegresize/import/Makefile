PREFIX=$(abspath $(CURDIR)/local)

$(PREFIX): mpio/config.status msgpack-c/config.status msgpack-rpc-cpp/config.status

.NOTPARALLEL: mpio

mpio/config.status:
	$(MAKE) mpio
	cd mpio && ./configure --prefix=$(PREFIX) && $(MAKE) install

mpio:
	wget https://github.com/downloads/frsyuki/mpio/mpio-0.3.7.tar.gz
	tar -xf mpio*.tar.gz && rm mpio*.tar.gz
	mv mpio-* mpio

.NOTPARALLEL: msgpack-c

msgpack-c/config.status:
	$(MAKE) msgpack-c
	cd msgpack-c && ./configure --prefix=$(PREFIX) && $(MAKE) install

msgpack-c:
	wget https://github.com/msgpack/msgpack-c/releases/download/cpp-0.5.9/msgpack-0.5.9.tar.gz
	tar -xf msgpack*.tar.gz && rm msgpack*.tar.gz
	mv msgpack* $@

.NOTPARALLEL: msgpack-rpc

msgpack-rpc-cpp/config.status:
	$(MAKE) msgpack-rpc-cpp
	cd msgpack-rpc-cpp && ./bootstrap && \
	    ./configure --prefix=$(PREFIX) \
		--with-mpio=$(PREFIX) \
		--with-msgpack=$(PREFIX) && \
	    $(MAKE) install

msgpack-rpc-cpp:
	wget https://github.com/msgpack-rpc/msgpack-rpc/archive/cpp-0.3.1.tar.gz
	tar -xf cpp*.tar.gz && rm cpp*.tar.gz
	mv msgpack-rpc-*/cpp $@ && rm -rf msgpack-rpc-cpp-*
##

clean:
	rm -rf mpio msgpack-c msgpack-rpc-cpp $(PREFIX)
