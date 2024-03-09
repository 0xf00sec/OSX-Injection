section .data
section .text

global _main
	_main:

start:
	jmp trick

continue:
	pop rsi            ; Pop string address into rsi
	mov rax, 0x2000004 ; System call write = 4
	mov rdi, 1         ; Write to standard out = 1
	mov rdx, 14        ; The size to write
	syscall            ; Invoke the kernel
	mov rax, 0x2000001 ; System call number for exit = 1
	mov rdi, 0         ; Exit success = 0
	syscall            ; Invoke the kernel
	
trick:
	call continue
	db "Hello World!", 0, 0