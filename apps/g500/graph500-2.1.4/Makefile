# -*- Makefile -*-
# Copyright 2010,  Georgia Institute of Technology, USA.
# See COPYING for license.
BUILD_OPENMP = No
BUILD_XMT = No
BUILD_CONVEY = Yes
include make-incs/make.inc-convey

GRAPH500_SOURCES=graph500.c options.c rmat.c kronecker.c verify.c prng.c \
	xalloc.c timer.c 

MAKE_EDGELIST_SOURCES=make-edgelist.c options.c rmat.c kronecker.c prng.c \
	xalloc.c timer.c 

BIN=seq-list/seq-list seq-csr/seq-csr make-edgelist

ifeq ($(BUILD_OPENMP), Yes)
BIN += omp-csr/omp-csr
endif

ifeq ($(BUILD_MPI), Yes)
BIN += mpi/graph500_mpi_simple
endif

ifeq ($(BUILD_XMT), Yes)
BIN = xmt-csr/xmt-csr xmt-csr-local/xmt-csr-local
endif

ifeq ($(BUILD_CONVEY), Yes)
BIN = convey-csr/convey-csr
endif

GENERATOR_SRCS=splittable_mrg.c	\
	graph_generator.c \
	make_graph.c utils.c

.PHONY: all
all: $(BIN)

CPPFLAGS += -I./generator

make-edgelist: CFLAGS:=$(CFLAGS) $(CFLAGS_OPENMP) $(DEFINE)
make-edgelist:	make-edgelist.c options.c rmat.c kronecker.c prng.c \
	xalloc.c timer.c $(addprefix generator/,$(GENERATOR_SRCS))

seq-list/seq-list: CFLAGS:=$(CFLAGS) $(DEFINE)
seq-list/seq-list: seq-list/seq-list.c $(GRAPH500_SOURCES) \
	$(addprefix generator/,$(GENERATOR_SRCS))
seq-csr/seq-csr: CFLAGS:=$(CFLAGS) $(DEFINE)
seq-csr/seq-csr: seq-csr/seq-csr.c $(GRAPH500_SOURCES) \
	$(addprefix generator/,$(GENERATOR_SRCS))

omp-csr/omp-csr: CFLAGS:=$(CFLAGS) $(CFLAGS_OPENMP)
omp-csr/omp-csr: omp-csr/omp-csr.c $(GRAPH500_SOURCES) \
	$(addprefix generator/,$(GENERATOR_SRCS))

xmt-csr/xmt-csr: CFLAGS:=$(CFLAGS) -pl xmt-csr/xmt-csr.pl
xmt-csr/xmt-csr: xmt-csr/xmt-csr.c $(GRAPH500_SOURCES) \
	$(addprefix generator/,$(GENERATOR_SRCS))

xmt-csr-local/xmt-csr-local: CFLAGS:=$(CFLAGS) -pl xmt-csr-local/xmt-csr-local.pl
xmt-csr-local/xmt-csr-local: xmt-csr-local/xmt-csr-local.c $(GRAPH500_SOURCES) \
	$(addprefix generator/,$(GENERATOR_SRCS))

LIB_HT ?= ../lib_pers/libhtapp.a
PDK_CFG = $(wildcard $(dir $(LIB_HT))ht/config.pdk)
ifneq ($(PDK_CFG),)
PDK_PLATFORM = $(shell grep 'PLATFORM =' $(PDK_CFG) | sed 's/^.*= //')
endif

convey-csr/convey-csr: CFLAGS:=$(CFLAGS) -DCONVEY $(DEFINE) $(CFLAGS_OPENMP)
convey-csr/convey-csr: LDLIBS += $(LIB_HT)

ifeq ($(findstring sysc,$(LIB_HT)),)
 ifneq ($(findstring hc,$(PDK_PLATFORM)),)
  convey-csr/convey-csr: LDLIBS += -L/opt/convey/lib
  convey-csr/convey-csr: LDLIBS += /opt/convey/lib/cny_initruntime.o
  convey-csr/convey-csr: LDLIBS += -lcny_utils -lPersSupport -ldl
 else
  ifneq ($(findstring vsim,$(LIB_HT)),)
   convey-csr/convey-csr: LDLIBS += -L /opt/convey/lib -lwx_sim_runtime
  else
   convey-csr/convey-csr: LDLIBS += -L /opt/convey/lib -lwx_runtime
  endif
 endif
endif

convey-csr_src = $(wildcard convey-csr/*.c) \
	$(GRAPH500_SOURCES) $(addprefix generator/,$(GENERATOR_SRCS))
convey-csr_objs = $(convey-csr_src:.c=.o)
convey-csr/convey-csr: $(convey-csr_objs) $(LIB_HT)
	$(CXX) $(CFLAGS) $(convey-csr_objs) -o $@  $(LDLIBS)

generator/generator_test_seq: generator/generator_test_seq.c $(GENERATOR_SRCS)

generator/generator_test_omp: generator/generator_test_omp.c $(GENERATOR_SRCS)

mpi/graph500_mpi_simple mpi/graph500_mpi_one_sided mpi/graph500_mpi_replicated:
	$(MAKE) -C mpi

.PHONY:	clean
clean:
	rm -f generator/generator_test_seq generator/generator_test_omp \
		$(BIN) $(convey-csr_objs)
	-$(MAKE) -C mpi clean
