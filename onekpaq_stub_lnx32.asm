
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
	db 0x7F,"ELF"
	db 1 ; E_CLASS
	db 1 ; E_DATA
	db 1 ; E_VERSION
	db 0 ; E_OSABI
	times 8 db 0
	dw 2 ; e_type (EXEC)
	dw 3 ; e_machine
	dd 1 ; e_version
	dd _start ; e_entry
	dd phdr - ehdr ; e_phoff
	dd 0 ; e_shoff
	dd 0 ; e_flags
	dw ehdr.end-ehdr ; e_ehsize
	dw phdr.end-phdr ; e_phentsize
	dw 1 ; e_phnum
	dw 0 ; e_shentsize
	dw 0 ; e_shnum
	dw 0 ; e_shstrndx
.end:

phdr:
	dd 1 ; p_type
	dd 0 ; p_offset
	dd ehdr ; p_vaddr
	dd phdr ; p_paddr
	dd END-ehdr ; p_filesz
	dd 0xfc000;END-ehdr ; memsz
	dd 7 ; p_flags
	dd 0x1000 ; p_align
.end:

_start:
	mov ebx, payload+PAYLOAD_OFF
	;push ebx ; ???
	;mov edi, payload_dest
	lea edi, [ebx + payload_dest-payload-PAYLOAD_OFF]

%include "onekpaq_decompressor32.asm"

	mov eax, SYS_memfd_create
	; ebx: name: whatever
	xor ecx, ecx
	int 0x80

	xchg eax, ebx
	push SYS_write
	pop eax
	mov ecx, payload_dest
	mov edx, 0x4000-(payload_dest-ehdr)
	int 0x80
	;int3

	pop ecx
	mov edx, esp ; argv
	lea esi, [edx + ecx * 4 + 4]

	mov ax, SYS_execveat
	; ebx: filedes
	mov ecx, emptystr
	; edx: argv
	; esi: envp
	mov edi, AT_EMPTY_PATH
	int 0x80

	int3

emptystr:
	db 0

payload:
	;incbin "flag32.raw"
	incbin PAYLOAD_BIN
payload.end:

END:

;	resb 9
payload_dest equ payload.end + 9 + 4
;payload_dest:

