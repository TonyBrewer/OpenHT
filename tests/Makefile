DIRS   = $(sort $(patsubst %/,%,$(dir $(wildcard */Makefile))))

SKIP  += model
SKIP  += $(wildcard OMP*)
SKIP  += memPSGRand
# SKIP list needs trailing <space>
SKIP  += 

VSKIP  = $(SKIP)
VSKIP += jacobi 

LOOPCNT ?= 1

.PHONY: regress
regress:
	@$(MAKE) _regress TGT=sysc

.PHONY: vregress
vregress:
	@$(MAKE) _regress TGT=vsim 

.PHONY: mregress
mregress:
	@$(MAKE) _regress TGT=model 

ifneq (,$(wildcard $(CURDIR)/../rpm/prefix))
  HT_DIR = $(CURDIR)/../rpm/prefix
else
ifneq (,$(wildcard /usr/local/ht_releases/latest))
  HT_DIR = /usr/local/ht_releases/latest
else
  HT_DIR = $(CURDIR)/..
endif
endif

RDIRS=$(DIRS)
ifdef SINGLE
  RDIRS=$(SINGLE)
endif

ifeq (sysc,$(TGT))
  RSKIP=$(SKIP)
else
  RSKIP=$(VSKIP)
endif

CNY_PDK          = /work/pdk_releases
CNY_PDK_REV      = latest
CNY_PDK_PLATFORM = wx-690

REG_ARGS  = CNY_PDK=$(CNY_PDK)
REG_ARGS += CNY_PDK_REV=$(CNY_PDK_REV)
REG_ARGS += CNY_PDK_PLATFORM=$(CNY_PDK_PLATFORM)
REG_ARGS += HT_DIR=$(HT_DIR)

.PHONY: _regress
_regress:
	@echo "Using HT_DIR=$(HT_DIR)"
	@/bin/echo "Skipping.. $(RSKIP)"
	@for l in {1..$(LOOPCNT)}; do \
		if [ $(LOOPCNT) -gt 1 ]; then \
		    echo LOOP: $$l; \
		fi; \
		for d in $(RDIRS); do \
		    s=`echo "$(RSKIP)" | grep "$$d "`; \
		    if [ "$(TGT)" != "model" -o "$$d/src_model" ] \
		         && [ -z "$$s" ]; \
		    then \
			/bin/echo -n "Regressing $$d... "; \
			(cd $$d > /dev/null; \
			    $(MAKE) $(TGT) $(REG_ARGS) > make.log 2>&1; \
			    if [ -x app_$(TGT) ]; then \
				./app_$(TGT) > regress.log 2>&1; \
				pf=`grep -e PASSED -e FAILED regress.log`; \
				if [ -z "$$pf" ]; then \
				    echo ERROR; \
				else \
				    echo $$pf; \
				fi; \
			    else \
				echo ERROR; \
			    fi \
			); \
		    fi \
		done; \
		if [ $(LOOPCNT) -gt 1 ]; then \
		    for d in $(DIRS); do \
			$(MAKE) -C$$d clean; \
			mv -f $$d/make.log $$d/make_$$l.log; \
			mv -f $$d/regress.log $$d/regress_$$l.log; \
		    done \
		fi; \
	done

_status:
	@/bin/echo "Skipping.. $(RSKIP)"
	@for d in $(RDIRS); do \
	    s=`echo "$(RSKIP)" | grep "$$d "`; \
	    if [ -z "$$s" ]; then \
		/bin/echo -n "Regressing $$d... "; \
		(cd $$d > /dev/null; \
		    pf=`grep -e PASSED -e FAILED regress.log`; \
		    if [ -z "$$pf" ]; then \
			echo ERROR; \
		    else \
			echo $$pf; \
		    fi; \
		); \
	    fi \
	done; \

sysc vsim:
	@$(foreach d, $(DIRS), $(MAKE) -C$(d) $@ > $(d)/make.log 2>&1;)

distclean clean:
	@$(foreach d, $(DIRS), $(MAKE) -s -C$(d) $@ \
			    	HT_DIR=$(HT_DIR) \
			    	CNY_PDK=$(CNY_PDK) \
			    	CNY_PDK_REV=$(CNY_PDK_REV) 2>&1 > /dev/null;)
	rm -f */make*.log */regress*.log
	rm -f InstrTrace.txt HtMon.txt
