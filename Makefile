# Copyright (C) Teemu Suutari

BITS	:= 32
VERSION	= 1.1

-include config.mk

HAS_LIBDISPATCH ?= 0

CC	= clang
CXX	= clang++
COMMONFLAGS = -g -Os -Wall -Wsign-compare -Wshorten-64-to-32 -Wno-shift-op-parentheses -DONEKPAQ_VERSION="\"$(VERSION)\"" -mtune=i386 -fno-tree-vectorize -mfpmath=387 -ffloat-store -mno-sse2 -mno-sse -mno-mmx -m80387
CFLAGS	= $(COMMONFLAGS)
CXXFLAGS = $(COMMONFLAGS) -std=c++14
AFLAGS	= -O2 -g

ifneq ($(HAS_LIBDISPATCH),0)
ifneq ($(LIBDISPATCH_INC_DIR),)
COMMONFLAGS += -DHAS_LIBDISPATCH -I$(LIBDISPATCH_INC_DIR) -L$(LIBDISPATCH_LIB_DIR) -ldispatch -fblocks -lBlocksRuntime
else
COMMONFLAGS += -DHAS_LIBDISPATCH -ldispatch -fblocks -lBlocksRuntime
endif
endif

# debugging...
#COMMONFLAGS += -DDEBUG_BUILD -g
#AFLAGS	+= -DDEBUG_BUILD -g

ifeq ($(BITS),64)
COMMONFLAGS += -m64
AFLAGS	+= -felf64
else
COMMONFLAGS += -m32
AFLAGS	+= -felf32
endif

PROG	:= onekpaq
SLINKS	:= onekpaq_encode onekpaq_decode
OBJS	:= ArithEncoder.o ArithDecoder.o BlockCodec.o StreamCodec.o CacheFile.o \
	onekpaq_main.o log.o

ifeq ($(BITS),32)
OBJS := $(OBJS) AsmDecode.o $(foreach decompr,1 2 3 4,onekpaq_cfunc_$(decompr).o)
endif

all: $(SLINKS)

.asm.o:
	nasm $(AFLAGS) $<

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<

.c.o:
	$(CC) $(CFLAGS) -c $<

define decompressors
onekpaq_cfunc_$(1).o: onekpaq_decompressor$(BITS).asm onekpaq_cfunc$(BITS).asm
	nasm $(AFLAGS) -DONEKPAQ_DECOMPRESSOR_MODE=$(1) onekpaq_cfunc$(BITS).asm -o onekpaq_cfunc_$(1).o
endef

$(foreach decompr,1 2 3 4,$(eval $(call decompressors,$(decompr))))

$(PROG): $(OBJS)
	$(CXX) $(CFLAGS) -o $(PROG) $(OBJS) 

$(SLINKS): $(PROG)
	ln -sf $< $@

clean:
	rm -f $(OBJS) $(PROG) $(SLINKS) *~

.PHONY:

define reporting
report$(1): .PHONY
	@rm -f tmp_out
	@nasm -O2 -DONEKPAQ_DECOMPRESSOR_MODE=$(1) onekpaq_decompressor$(BITS).asm -o tmp_out
	@stat -f "onekpaq decompressor mode$(1) size: %z" tmp_out
	@rm -f tmp_out
endef

$(foreach decompr,1 2 3 4,$(eval $(call reporting,$(decompr))))

report: report1 report2 report3 report4
