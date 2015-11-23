CXXFLAGS  = -I/opt/convey/include

PersAsm.o: PersAsm.s
	cpp -E $(CXXFLAGS) $? | /opt/convey/bin/cnyas -o $@

clean:
	rm -f PersAsm.o
