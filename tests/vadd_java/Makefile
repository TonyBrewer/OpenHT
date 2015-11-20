export CNY_PDK          ?= /opt/convey/pdk2
export CNY_PDK_REV      ?= latest
export CNY_PDK_PLATFORM ?= hc-2

PLAT_wx-2000_AEUCNT = 8
PLAT_${CNY_PDK_PLATFORM}_AEUCNT ?= 16
pers: export CNY_HT_AEUCNT = $(PLAT_${CNY_PDK_PLATFORM}_AEUCNT)

export HT_LIB_PIC=true

javamodel: TGT=model
javasysc: TGT=sysc
javaapp: TGT=app
LINK_OPTS   = -o libVaddCoproc.so -shared -z noexecstack 
javaapp: LINK_OPTS += -L/opt/convey/lib
ifneq (,$(findstring wx,$(CNY_PDK_PLATFORM)))
javaapp: LINK_OPTS += -lwx_runtime
else
javaapp: LINK_OPTS += -lcny_sys -lcny_init -lconvey_utils
endif
LINK_OPTS += -Wl,-whole-archive libht$(TGT).pa -Wl,-no-whole-archive 

export CFLAGS += -I/usr/lib/jvm/java-openjdk/include
export CFLAGS += -I/usr/lib/jvm/java-openjdk/include/linux

export CNY_HT_LIBEXCLUDE = Main

javamodel: libmodel
	g++ $(LINK_OPTS)

javasysc: libsysc
	g++ $(LINK_OPTS)

javaapp: libapp
	g++ $(LINK_OPTS)

javaclean:
	rm -f libVaddCoproc.so

clean cleaner distclean: javaclean

# DO NOT MODIFY ITEMS BELOW THIS LINE
export HT_DIR ?= /usr/local/ht_releases/latest
include $(HT_DIR)/ht_lib/MakefileInclude.cnyht
