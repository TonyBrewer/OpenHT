DIRS   = $(sort $(patsubst %/,%,$(dir $(wildcard */Makefile))))

# SKIP list needs trailing <space>
SKIP  += bug_1366 bug_1369 callrtn g500 model
SKIP  += $(wildcard OMP*)
SKIP  += gups 

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

HT_DIR = $(wildcard $(PWD)/../rpm/prefix)
ifeq (,$(HT_DIR))
    HT_DIR = /work/ht_releases/latest
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
	@for l in {1..$(LOOPCNT)}; do \
		if [ $(LOOPCNT) -gt 1 ]; then \
		    echo LOOP: $$l; \
		fi; \
		for d in $(RDIRS); do \
		    /bin/echo -n "Regressing $$d... "; \
		    if [ "$(TGT)" = "model" -a ! -d "$$d/src_model" ]; then \
			echo NO_SRC; \
		    else \
		    s=`echo "$(RSKIP)" | grep "$$d "`; \
		    if [ -n "$$s" ]; then \
			echo SKIPPED; \
		    else \
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
	@for d in $(RDIRS); do \
	    /bin/echo -n "Regressing $$d... "; \
	    s=`echo "$(RSKIP)" | grep "$$d "`; \
	    if [ -n "$$s" ]; then \
		echo SKIPPED; \
	    else \
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
			    	CNY_PDK=$(CNY_PDK) \
			    	CNY_PDK_REV=$(CNY_PDK_REV) 2>&1 > /dev/null;)
	rm -f */make*.log */regress*.log
	rm -f InstrTrace.txt HtMon.txt
