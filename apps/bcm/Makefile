cgminer: import/cgminer/cgminer
	mv import/cgminer/cgminer .

import/cgminer/cgminer:
	cd import > /dev/null; $(MAKE)

pers:
	cd lib_pers > /dev/null && $(MAKE) $@

clean distclean:
	rm -f cgminer
	cd import > /dev/null && $(MAKE) $@
	cd lib_pers > /dev/null && $(MAKE) $@
