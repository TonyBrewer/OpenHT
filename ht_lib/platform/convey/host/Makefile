ifeq (,$(wildcard /opt/micron/include/wdm_user.h))
  CXXFLAGS = -I/opt/convey/include
else
  CXXFLAGS = -I/opt/micron/include
endif

PersAsm.o: PersAsm.s
	cpp -E $(CXXFLAGS) $? | /opt/convey/bin/cnyas -o $@

clean:
	rm -f PersAsm.o
