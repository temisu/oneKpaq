# Copyright (C) Teemu Suutari

BITS	?= $(shell getconf LONG_BIT)
VERSION	= 1.1

NASM ?= nasm

HAS_LIBDISPATCH ?= 0

CC	= clang
CXX	= clang++
COMMONFLAGS = -g -Os -Wall -Wsign-compare -Wshorten-64-to-32 -Wno-shift-op-parentheses -DONEKPAQ_VERSION="\"$(VERSION)\""
ifeq ($(BITS),32)
COMMONFLAGS += -mtune=i386 -fno-tree-vectorize -ffloat-store -mno-sse2 -mno-mmx
COMMONFLAGS += -mno-sse -mfpmath=387 -m80387
endif
CFLAGS	= $(COMMONFLAGS)
CXXFLAGS = $(COMMONFLAGS) -std=c++14
AFLAGS	= -O2 -g

-include config.mk

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
SLINKS	:= onekpaq_encode #onekpaq_decode
OBJS	:= obj/ArithEncoder.cpp.o obj/ArithDecoder.cpp.o obj/BlockCodec.cpp.o obj/StreamCodec.cpp.o obj/CacheFile.cpp.o \
	obj/onekpaq_main.cpp.o obj/log.c.o

ifeq ($(BITS),32)
OBJS := $(OBJS) obj/AsmDecode.cpp.o $(foreach decompr,1 2 3 4,obj/onekpaq_cfunc_$(decompr).asm.o)
endif

all: $(SLINKS)

%/:
	mkdir -p "$@"

#obj/%.asm.o: %.asm obj/
#	$(NASM) $(AFLAGS) "$<" -o "$@"

obj/%.cpp.o: %.cpp obj/
	$(CXX) $(CXXFLAGS) -c "$<" -o "$@"

obj/%.c.o: %.c obj/
	$(CC) $(CFLAGS) -c "$<" -o "$@"

define decompressors
obj/onekpaq_cfunc_$(1).asm.o: onekpaq_decompressor$(BITS).asm onekpaq_cfunc$(BITS).asm obj/
	$(NASM) $(AFLAGS) -DONEKPAQ_DECOMPRESSOR_MODE=$(1) onekpaq_cfunc$(BITS).asm -o "obj/onekpaq_cfunc_$(1).asm.o"
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
	@$(NASM) -O2 -DONEKPAQ_DECOMPRESSOR_MODE=$(1) onekpaq_decompressor$(BITS).asm -o tmp_out
	@stat -f "onekpaq decompressor mode$(1) size: %z" tmp_out
	@rm -f tmp_out
endef

$(foreach decompr,1 2 3 4,$(eval $(call reporting,$(decompr))))

report: report1 report2 report3 report4
