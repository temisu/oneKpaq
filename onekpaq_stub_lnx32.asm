
; vim: set ft=nasm noet ts=4:

; offset = 2, mode=3, dind=2, shift=3

%ifndef ONEKPAQ_DECOMPRESSOR_MODE
%error "please define ONEKPAQ_DECOMPRESSOR_MODE"
;%define ONEKPAQ_DECOMPRESSOR_MODE 3
%endif
%ifndef ONEKPAQ_DECOMPRESSOR_SHIFT
%error "please define ONEKPAQ_DECOMPRESSOR_SHIFT"
;%define ONEKPAQ_DECOMPRESSOR_SHIFT 3
%endif
%ifndef PAYLOAD_OFF
%error "please define PAYLOAD_OFF"
;%define PAYLOAD_OFF 2
%endif

%define ONEKPAQ_NO_SECTIONS 1

%define STDIN_FILENO	0
%define STDOUT_FILENO	1
%define STDERR_FILENO	2

%define AT_EMPTY_PATH	0x1000
%define P_ALL			4

%define SYS_exit			  1
%define SYS_fork			  2
%define SYS_write			  4
%define SYS_open			  5
%define SYS_waitpid			  7
%define SYS_creat			  8
%define SYS_execve			 11
%define SYS_lseek			 19
%define SYS_dup2			 63
%define SYS_vfork			190
%define SYS_memfd_create	356
%define SYS_execveat		358

bits 32

org 0x08048000

ehdr:
	db 0x7F,"ELF" ;!EI_MAGIC
	db 1 ;~E_CLASS
	db 1 ;~E_DATA

_endstub1: ; 10 bytes
	mov eax, SYS_memfd_create ; 5 bytes
	; ebx: name: don't care
	xor ecx, ecx ; 2 bytes
	int 0x80     ; 2 bytes

	db 0x3d ; cmp eax, imm32 -> jump over e_type and e_machine

	;db 1 ; E_VERSION
	;db 0 ; E_OSABI
	;times 8 db 0
	db 2 ;!e_type.0 (EXEC)
emptystr:
	db 0 ;!e_type.1 (EXEC)
	dw 3 ;!e_machine

	;dd 1 ; e_version
_endstub2: ; 4 bytes
	push SYS_write ; 2 bytes
	jmp short _endstub3 ; 2 bytes

	dd _start ;!e_entry
	dd phdr - ehdr ;!e_phoff

	;dd 0 ; e_shoff
	;dd 0 ; e_flags
_endstub3: ; 8 bytes
	xchg eax, ebx ; 1 byte
	;pop eax ; 1 byte
	mov ecx, payload_dest ; 5 bytes
	jmp short _endstub4 ; 2 bytes

	dw ehdr.end-ehdr ;!e_ehsize
	dw phdr.end-phdr ;!e_phentsize
	dw 1 ;!e_phnum

_endstub4: ; >=6 bytes
	pop eax ; 1 byte
	mov edx, 0x4000-(payload_dest-ehdr) ; 5 bytes
ehdr.end:
	int 0x80 ; 2 bytes
	jmp short _endstub5

	;dw 0 ; e_shentsize
	;dw 0 ; e_shnum
	;dw 0 ; e_shstrndx

phdr:
	dd 1 ;!p_type
	dd 0 ;!p_offset
	dd ehdr ;!p_vaddr
	dd phdr ;!p_paddr
	dd END-ehdr ;!p_filesz
	dd 0xfc000;END-ehdr ;!memsz

	;dd 7 ;~p_flags
	;dd 0x1000 ; p_align
	db 7
_endstub5: ; 7 bytes
	pop ecx ; 1 byte
	mov edx, esp ; 2 bytes
	lea esi, [edx + ecx * 4 + 4] ; 4 bytes
phdr.end:

	mov ax, SYS_execveat
	mov ecx, emptystr
	mov edi, AT_EMPTY_PATH
	int 0x80

_start:
	mov ebx, payload+PAYLOAD_OFF
	lea edi, [ebx + payload_dest-payload-PAYLOAD_OFF]

%include "onekpaq_decompressor32.asm"

	jmp _endstub1

;	mov eax, SYS_memfd_create
;	; ebx: name: whatever
;	xor ecx, ecx
;	int 0x80
;
;	push SYS_write
;	xchg eax, ebx
;	mov ecx, payload_dest
;	pop eax
;	mov edx, 0x4000-(payload_dest-ehdr)
;	int 0x80
;
;	pop ecx ; argc
;	mov edx, esp ; argv
;	lea esi, [edx + ecx * 4 + 4] ; envp
;
;	mov ax, SYS_execveat ; eax was SYS_write result
;	; ebx: filedes
;	mov ecx, emptystr
;	; edx: argv
;	; esi: envp
;	mov edi, AT_EMPTY_PATH
;	int 0x80
;
;	int3

payload:
	;incbin "flag32.raw"
	incbin PAYLOAD_BIN
payload.end:

END:

;	resb 9
payload_dest equ payload.end + 9 + 4
;payload_dest:

