; Copyright (c) Teemu Suutari


	bits 32
	[section .text]

global _onekpaq_decompressor_mode%[ONEKPAQ_DECOMPRESSOR_MODE]_shift
_onekpaq_decompressor_mode%[ONEKPAQ_DECOMPRESSOR_MODE]_shift: equ onekpaq_decompressor.shift

global _onekpaq_decompressor_mode%[ONEKPAQ_DECOMPRESSOR_MODE]
_onekpaq_decompressor_mode%[ONEKPAQ_DECOMPRESSOR_MODE]:

	pushad
	mov ebx,[byte esp+32+4]
	mov edi,[byte esp+32+8]

%include "onekpaq_decompressor32.asm"

	popad
	ret

global _onekpaq_decompressor_mode%[ONEKPAQ_DECOMPRESSOR_MODE]_end
_onekpaq_decompressor_mode%[ONEKPAQ_DECOMPRESSOR_MODE]_end:

	__SECT__

;; ---------------------------------------------------------------------------
