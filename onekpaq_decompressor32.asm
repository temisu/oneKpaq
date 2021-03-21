; Copyright (c) Teemu Suutari

;; ---------------------------------------------------------------------------
; Please define ONEKPAQ_DECOMPRESSOR_MODE as a compile time option
; from 1 to 4, this number matches the definition from StreamCodec:
; 1 - Single section decoder. Slow decoder for single section data
; 2 - Multi section decode. Slow decoder for multi-section data
; 3 - Single section decoder. Fast decoder for single section data
; 4 - Multi section decode. Fast decoder for multi-section data
;
; Define either shift by ONEKPAQ_DECOMPRESSOR_SHIFT or modify byte
; onekpaq_decompressor.shift later
;; ---------------------------------------------------------------------------

; clean out defines that should not be directly defined
%undef ONEKPAQ_DECOMPRESSOR_FAST
%undef ONEKPAQ_DECOMPRESSOR_MULTI_SECTION

%ifndef ONEKPAQ_DECOMPRESSOR_MODE
%error "please define ONEKPAQ_DECOMPRESSOR_MODE"
; nasm does not know how to stop preprocessor
%define ONEKPAQ_DECOMPRESSOR_MODE
%endif

%ifidn ONEKPAQ_DECOMPRESSOR_MODE,1
; Single section, normal speed
; (default mode)

%elifidn ONEKPAQ_DECOMPRESSOR_MODE,2
; Multi section, normal speed
%define ONEKPAQ_DECOMPRESSOR_MULTI_SECTION 1

%elifidn ONEKPAQ_DECOMPRESSOR_MODE,3
; Single section, fast speed
%define ONEKPAQ_DECOMPRESSOR_FAST 1

%elifidn ONEKPAQ_DECOMPRESSOR_MODE,4
; Multi section, fast speed
%define ONEKPAQ_DECOMPRESSOR_FAST 1
%define ONEKPAQ_DECOMPRESSOR_MULTI_SECTION 1

%else
%error "ONEKPAQ_DECOMPRESSOR_MODE is not valid (1 - 4)"
%endif

%ifndef ONEKPAQ_DECOMPRESSOR_SHIFT
%define ONEKPAQ_DECOMPRESSOR_SHIFT 0
%endif

;; ---------------------------------------------------------------------------

; Debugging can be enabled with DEBUG_BUILD.
; However, debug builds are really really noisy and not PIE.
%ifdef DEBUG_BUILD
%include "debug32.asm"
%else
%macro DEBUG 1+
%endm
%endif

;; ---------------------------------------------------------------------------

	bits 32
%ifdef ONEKPAQ_DECOMPRESSOR_FAST
	cpu P4				; enable SSE
%else
	cpu 386
%endif
%ifndef ONEKPAQ_NO_SECTIONS
	[section .text.onekpaq_decompressor]
%endif

;; end of preproc and setup, start of real stuff

	; embeddable code block
	;
	; inputs:
	; ebx concatenated block1+block2, pointer to start of block2
	; edi dest (must be zero filled and writable from -13 byte offset
	;     to the expected length plus one byte)
	; header+src+dest buffers must not overlap
	; d flag clear
	; fpu inited and 2 registers free
	;
	; output & side effects:
	; messed up header
	; messed up src
	; dest filled out with unpacked data
	; all registers contents destroyed
	; xmm0 & xmm1 contents destroyed when using fast variant
	; 96 bytes of stack used (64 bytes for fast variant)
onekpaq_decompressor:
	DEBUG "oneKpaq decompression started..."

	lea esi,[byte edi-(9+4)]	; esi=dest, edi=window start
	lodsd
	inc eax
	mov ecx,eax

%ifdef ONEKPAQ_DECOMPRESSOR_MULTI_SECTION
	lea edx,[byte ebx+1]		; header=src-3 (src has -4 offset)
%else
	lea edx,[byte ebx+3]		; header=src-1 (src has -4 offset)
%endif
	; ebp unitialized, will be cleaned by following loop + first decode
	; which will result into 0 bit, before actual data

.normalize_loop:
	shl byte [byte ebx+4],1
	jnz short .src_byte_has_data
	inc ebx
	rcl byte [byte ebx+4],1		; CF==1
.src_byte_has_data:
	rcl ebp,1

.block_loop:
	; loop level 1
	; eax range
	; ebx src
	; ecx dest bit shift
	; edx header
	; esi dest
	; edi window start
	; ebp value
.byte_loop:
.bit_loop:
	; loop level 2
	; eax range
	; ebx src
	; ecx dest bit shift
	; edx header
	; esi dest
	; edi window start
	; ebp value
.normalize_start:
	add eax,eax
	jns short .normalize_loop

	; for subrange calculation
	fld1
	; p = 1
	fld1

	pushad
	salc
	;jc .alff
	;mov al, 00h
	;jnc .alok
;.alff: mov al, 0xff
;.alok:
.context_loop:
	; loop level 3
	; al 0
	; eax negative
	; ebx src
	; cl dest bit shift
 	; ch model
	; edx header
	; esi dest
	; edi window start
	; ebp value
	; st0 p
	; [esp] ad

	mov ch,[edx]

	pushad
	cdq
	mov [ebx],edx			; c0 = c1 = -1

%ifdef ONEKPAQ_DECOMPRESSOR_FAST
	movq xmm0,[esi]			; SSE
%endif

.model_loop:
	; loop level 4
	; al 0
	; [ebx] c1
	; cl dest bit shift
	; ch model
	; edx c0
	; esi dest
	; edi window start
	; st0 p
	; [esp] ad
	; [esp+32] ad

%ifdef ONEKPAQ_DECOMPRESSOR_FAST
	movq xmm1,[edi]			; SSE
	pcmpeqb xmm1,xmm0		; SSE
	pmovmskb eax,xmm1		; SSE
	or al,ch
	inc ax
	jnz short .match_no_hit

	mov al,[byte esi+8]
	rol al,cl
	xor al,[byte edi+8]
	shr eax,cl
	jnz short .match_no_hit
%else
	; deepest stack usage 32+32+32 bytes = 96 bytes
	pushad

.match_byte_loop:
	; loop level 5
	cmpsb
	rcr ch,1			; ror would work as well
	ja short .match_mask_miss	; CF==0 && ZF==0
	add al,0x60			; any odd multiplier of 0x20 works
	jnz short .match_byte_loop

	lodsb
	rol al,cl
	xor al,[edi]
	shr al,cl			; undefined CF when cl=8, still works though
					; To make this conform to Intel spec
					; add 'xor eax,eax' after 'pushad'
					; and replace 'shr al,cl' with 'shr eax,cl'
					; -> +2 bytes
.match_mask_miss:
	popad
	jnz short .match_no_hit
%endif
	; modify c1 and c0
	dec edx
	dec dword [ebx]

	jc short .match_bit_set
	sar edx,1
%ifndef ONEKPAQ_DECOMPRESSOR_FAST
.match_no_hit:
%endif
	db 0xc0				; rcl cl,0x3b -> nop (0x3b&31=3*9)
.match_bit_set:
	sar dword [ebx],1

;	DEBUG "Model+bit: %hx, new weights %d/%d",ecx,dword [ebx],edx
%ifdef ONEKPAQ_DECOMPRESSOR_FAST
.match_no_hit:
%endif
	inc edi

	; matching done
	cmp edi,esi
%ifdef ONEKPAQ_DECOMPRESSOR_MULTI_SECTION
	ja short .model_early_start
	jnz short .model_loop
%else
	; will do underflow matching with zeros...
	; not ideal if data starts with lots of ones.
	; Usally impact is 1 or 2 bytes, thus mildly
	; better than +2 bytes of code
	jc short .model_loop
%endif
	; scale the probabilities before loading them to FPU
	; p *= c1/c0 =>  p = c1/(c0/p)
.weight_upload_loop:
.shift:	equ $+2
	rol dword [ebx],byte ONEKPAQ_DECOMPRESSOR_SHIFT
	fidivr dword [ebx]
	mov [ebx],edx

%ifdef ONEKPAQ_DECOMPRESSOR_FAST
	neg ecx
	js short .weight_upload_loop
%else
	dec eax
	jp short .weight_upload_loop
%endif

.model_early_start:
	popad

.context_reload:
	dec edx
	cmp ch,[edx]
	jc short .context_next
	fsqrt
	jbe short .context_reload

.context_next:
	cmp al,[edx]
	jnz short .context_loop

	popad

	; restore range
	shr eax,1

	; subrange = range/(p+1)
	faddp st1
	mov [ebx],eax
	fidivr dword [ebx]
	fistp dword [ebx]

	; Arith decode
	DEBUG "value %x, range %x, sr %x",ebp,eax,dword [ebx]
	sub eax,[ebx]
	cmp ebp,eax
%ifdef ONEKPAQ_DECOMPRESSOR_MULTI_SECTION
	jc .dest_bit_is_set;short .dest_bit_is_set
%else
	jbe .dest_bit_is_set;short .dest_bit_is_set
	inc eax
%endif
	sub ebp,eax
	mov eax,[ebx]
;	uncommenting the next command would make the single-section decompressor "correct"
;	i.e. under %ifndef ONEKPAQ_DECOMPRESSOR_MULTI_SECTION
;	does not seem to be a practical problem though
	;dec eax
.dest_bit_is_set:
	rcl byte [byte esi+8],1

%ifndef ONEKPAQ_DECOMPRESSOR_MULTI_SECTION
	; preserves ZF when it matters i.e. on a non-byte boundary ...
	loop .no_full_byte
	inc esi
	mov cl,8
.no_full_byte:
	jnz .bit_loop;short .bit_loop

%else
.block_loop_trampoline:
;	dec cl
;	jnz short .bit_loop
	loop .bit_loop
	inc esi

	dec word [byte edx+1]
	jnz .new_byte;short .new_byte

	DEBUG "Block done"
	; next header
.skip_header_loop:
	dec edx
	cmp ch,[edx]
	jnz .skip_header_loop;short .skip_header_loop
	lea edx,[byte edx-3]
	cmp cx,[byte edx+1]
	lea edi,[byte esi+8]
.new_byte:
	mov cl,9
	jnz .block_loop_trampoline;short .block_loop_trampoline
%endif
	; all done!
	; happy happy joy joy
	DEBUG "oneKpaq decompression done"
onekpaq_decompressor_end:

%ifndef ONEKPAQ_NO_SECTIONS
	__SECT__
%endif

;; ---------------------------------------------------------------------------
