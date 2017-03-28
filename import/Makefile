BLD_BASE = $(wildcard /dev/shm)
ifeq (,$(BLD_BASE))
BLD_BASE = /tmp
endif

SYSTEMC_BUILD = $(BLD_BASE)/systemc
SYSTEMC_VER   = 2.3.1a
SYSTEMC_TAR   = systemc-$(SYSTEMC_VER).tar.gz

update_systemc: $(SYSTEMC_TAR)
	rm -rf $(SYSTEMC_BUILD); mkdir -p $(SYSTEMC_BUILD)
	cp -rp patches* $(SYSTEMC_BUILD)
	cat $(SYSTEMC_TAR) | (cd $(SYSTEMC_BUILD) > /dev/null; tar -zxvf -)
	./build_vars.sh && \
	    cd $(SYSTEMC_BUILD) > /dev/null && \
	    (cd systemc-$(SYSTEMC_VER) && \
		for p in `cat ../patches-$(SYSTEMC_VER)/order.txt`; do \
		    patch -p0 -i ../patches-$(SYSTEMC_VER)/$$p; \
		done && \
		./configure && \
		make install -j OPT_CXXFLAGS="-g -O2") && \
	    mkdir local_systemc && \
	    cp -rp systemc-$(SYSTEMC_VER)/include local_systemc && \
	    cp -rp systemc-$(SYSTEMC_VER)/lib-* local_systemc
	rm -rf local_systemc && mv $(SYSTEMC_BUILD)/local_systemc .
	rm -rf $(SYSTEMC_BUILD)

$(SYSTEMC_TAR):
	@echo "Please download $@ into the import directory"
	@exit

patch_systemc: $(SYSTEMC_TAR)
	tar -xvf $(SYSTEMC_TAR)
	cd systemc-$(SYSTEMC_VER) && \
	    for p in `cat ../patches-$(SYSTEMC_VER)/order.txt`; do \
		patch -p0 -i ../patches-$(SYSTEMC_VER)/$$p; \
	    done

clean:
	rm -rf $(SYSTEMC_BUILD) local_systemc systemc-$(SYSTEMC_VER)
