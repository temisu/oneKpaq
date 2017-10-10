
               .--------. .--------.   .---.
               `-----.   |`------.  `.  \   \
                     |   |        |  |.--    \
                     |   |.------'   .`---    \
                     `---'`---------'      `---'

----------------------------------------------------------------
                     oneKpaq v1.0 by TS/TDA            
----------------------------------------------------------------


Overview
--------

oneKpaq is compressor/decompressor pair for 1k / 4k intros.
It does not include any of the dynamic loader parts or hash
loading - this is pure compressor for all platfroms who want or
need a proper compressor. (x86 platform that is)

Some of its algorithms are inspired from PAQ family of packers
thus the naming. Other big inspiration was the crinkler (from
what is publicly known of it) The embeddable decompressor is
written in x86 assembly and it is 128 bytes at smallest.

Currently only 32-bit version is fully finished. if you need
64-bit, contact me. (Also 16-bit version could be doable)


Design
------

I set out to re-make the data compression for my future 4k/1k
intros with following guidelines
- Compression ratio as close to PAQ as possible with simplified
  decompressor - Same as in Crinkler
- Very small decompressor to be usable in 1K prods
- Static context models with adaptive multi section support for
  improved compression ratio
- Decompression time < 10 seconds on a modern CPU

From these ideas, I set out to experiment and ended up on
following design (copycatting following ideas from others):
- 8-byte window + decoded bits as a context w. mask bits for
  bytes resulting a total of 255 models to choose from
- model weights in the scale of 1/2^n (total being 1)
- PAQ1 count boosting
- Logistic mixing
- Hashless implementation, slow but uses very little memory
- Two modes for compression speed: small and fast (i.e. 1K/4K)
- Two modes for compression efficiency: single section or
  adaptive multi-section support for different kinds of data
  in same binary


Usage
-----

The compressor code is written using modern-ish C++ compatible
with recent clang and gcc although with gcc you have to modify
build flags a bit. It also should compile on VS2015, although
I was lazy and did not generate .vsproj for it, ABI would be
different as well. For the assmebly I used the ubiquitos nasm.
For parallelization I used dispatch library. Although it is
optional, I strongly recommended using it.

For the simplicity, the supported operating modes have been
enumerated as follows:

1. Single section decoder. Slow decoder for single section data: Decompressor is 128 bytes
1. Multi section decode. Slow decoder for multi-section data: Decompressor is 150 bytes
1. Single section decoder. Fast decoder for single section data: Decompressor is 143 bytes
1. Multi section decode. Fast decoder for multi-section data: Decompressor is 165 bytes

For the compressor, three complexity modes for compressing has
been defined: Low, Medium and High. These will translate to
minutes, hours and a day(s) of compressor running time.
Unsuprisingly, this is something you want to cache, and is
in fact cached by default so once searched models will be used
again.

Look for the oneKpaq_main.cpp how to use the compressor and
AsmDecode.cpp how to use the decompressor. Typical freestanding
usage of the decompressor would be something like this:

```
	mov ebx,source_of_compressed_code
	push ebx
	mov edi,destination_in_bss
%include "onekpaq_decompressor32.asm"
	ret
```

Please mind and define ONEKPAQ_DECOMPRESSOR_SHIFT for the
correct value produced, or include setting this value in your
build process.

Obviously that is 12 bytes of extra setup code, which can be
optimized further, depending on the chosen binary layout

There is also testbench directory, where is a simple test of the
current command line encoder.

Quirks
------

The source pointer for the input stream is not in the beginning
of the source data, but in the certain offset of it. That's
why compressor provides 2 buffers for input. It is meant that
those are concatenated and source pointer is provided between.

Source data will be completely ruined by the decompressor, it
is used as a scratch memory in the ongoing process. For this
reason the input data must reside in R/W page area.

Destination data needs to reside in BSS (or some zero filled
memory block) and this must extend from -9 offset to +1 offset
Although these extra bytes will stay as zeros, they will be read
and written to in the process of decompression.

For the small size modes, there are theoretical inputs which
produce invalid bitstream. Although I have tried to trigger it,
I have yet to stumble on it. You might not be that lucky though,
I would appreciate input in this case.

The assembly code uses the usual tricks + a little bit of
fudging to get better results when compressed itself (i.e. shell
dropping) This and the ifdefs for different modes makes it look
a bit convoluted :) Some documentation is included in the asm
source though.

Comparison
----------

Comparison is the though part. Amount of data used typically is
too short to have any meaningful statistical analysis. Other
than that I would need a lots of different representative files
to make some sort of proper comparison. Even then, because
making 1K/4K intro is a iterative process thus this process
biases the data for the current algorithm used - against any
other, it is almost impossible to say anything for certain.

Still, my feeling is that this compares well against Crinkler,
I won't pretend it would be better, but probably within 1-3%
of the compression ratio of what Crinkler can do. All the other
algorithms* do not even get close (Except PAQ8, but that is not
for 1K/4K). However, there is room for improvement, which I'll
be planning to do when I get more data points...

*) deflate, bzip2, lzw, lzma2

Thanks
------

Special thanks to Firehawk - Without his help, these projects
would have stayed at the drawing board. (Also thx for the logo)
