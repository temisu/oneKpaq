; Copyright (c) Teemu Suutari


	bits 32
	[section .text.onekpaq.cfunc32]

%ifidn __OUTPUT_FORMAT__, elf32
global onekpaq_decompressor_mode%[ONEKPAQ_DECOMPRESSOR_MODE]_shift
onekpaq_decompressor_mode%[ONEKPAQ_DECOMPRESSOR_MODE]_shift: equ onekpaq_decompressor.shift

global onekpaq_decompressor_mode%[ONEKPAQ_DECOMPRESSOR_MODE]
onekpaq_decompressor_mode%[ONEKPAQ_DECOMPRESSOR_MODE]:
%else
global _onekpaq_decompressor_mode%[ONEKPAQ_DECOMPRESSOR_MODE]_shift
_onekpaq_decompressor_mode%[ONEKPAQ_DECOMPRESSOR_MODE]_shift: equ onekpaq_decompressor.shift

global _onekpaq_decompressor_mode%[ONEKPAQ_DECOMPRESSOR_MODE]
_onekpaq_decompressor_mode%[ONEKPAQ_DECOMPRESSOR_MODE]:
%endif

	pushad
	mov ebx,[byte esp+32+4]
	mov edi,[byte esp+32+8]
	int3

;%define DEBUG_BUILD

%include "onekpaq_decompressor32.asm"

	popad
	ret

%ifidn __OUTPUT_FORMAT__, elf32
global onekpaq_decompressor_mode%[ONEKPAQ_DECOMPRESSOR_MODE]_end
onekpaq_decompressor_mode%[ONEKPAQ_DECOMPRESSOR_MODE]_end:
%else
global _onekpaq_decompressor_mode%[ONEKPAQ_DECOMPRESSOR_MODE]_end
_onekpaq_decompressor_mode%[ONEKPAQ_DECOMPRESSOR_MODE]_end:
%endif

	__SECT__

;; ---------------------------------------------------------------------------
