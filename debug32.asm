; Copyright (C) Teemu Suutari
%ifdef DEBUG_BUILD

; short loops wont be short anymore
%define short

; loop wont fit into byte
%macro loop 1
	loop %%do_loop
	jmp %%done
%%do_loop:
	jmp %1
%%done:
%endm

%macro EXTCALL 1
; edi first param
; esi second param
; edx third param
; ecx fourth param
; return value in eax

%ifndef ONEKPAQ_NO_SECTIONS
	[section .data.onekpaq.func_r_%1]
%endif
%%func_r_%1:
extern %1
	dd %1
	__SECT__

	pushfd
	push ebp			; osX mandates 16 byte stack alignment, does not harm others
	mov ebp,esp			; .
	and esp,byte ~15		; .

	push ecx
	push edx
	push esi
	push edi

	call [dword %%func_r_%1]
;	call %1

	pop edi
	pop esi
	pop edx
	pop ecx

	leave				; .
	popfd
%endm

%macro DEBUG_PLAIN 1
%ifndef ONEKPAQ_NO_SECTIONS
	[section .rodata.onekpaq.dbg_text]
%endif
%%dbg_text:
	db %1
	db 0
	__SECT__

	mov edi,%%dbg_text
%ifidn __OUTPUT_FORMAT__, elf32
	EXTCALL DebugPrint
%else
	EXTCALL _DebugPrint
%endif
%endm

%macro DEBUG 1
	pushad
	DEBUG_PLAIN {"D ",%1,10}
	popad
%endm

%macro DEBUG 2
	pushad
	push dword %2
	pop esi
	DEBUG_PLAIN {"D ",%1,10}
	popad
%endm

%macro DEBUG 3
	pushad
	push dword %2
	push dword %3
	pop edx
	pop esi
	DEBUG_PLAIN {"D ",%1,10}
	popad
%endm

%macro DEBUG 4
	pushad
	push dword %2
	push dword %3
	push dword %4
	pop ecx
	pop edx
	pop esi
	DEBUG_PLAIN {"D ",%1,10}
	popad
%endm

%else

%macro DEBUG 1+  
%endm

%endif
