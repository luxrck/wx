.set GD_KT,	0x08         # kernel code segment selector
.set GD_KD,	0x10        # kernel data segment selector

.globl start16
start16:
	.code16                     # Assemble for 16-bit mode
	cli                         # Disable interrupts
	cld                         # String operations increment

	# Set up the important data segment registers (DS, ES, SS).
	xorw    %ax, %ax
	movw    %ax, %ds
	movw    %ax, %es
	movw    %ax, %ss

	# Enable A20.
	movw    $0x2401, %ax
	int     $0x15
  
	# Switch from real to protected mode, using a bootstrap GDT.
	lgdt    gdtdesc
	movl    %cr0, %eax

	# Enable protect mode
	orl     $1, %eax
	movl    %eax, %cr0
  
	# Jump to next instruction, but in 32-bit code segment.
	# Switches processor into 32-bit mode.
	ljmp    $GD_KT, $start32

	.code32                     # Assemble for 32-bit mode
start32:
	# Set up the protected-mode data segment registers
	movw    $GD_KD, %ax             # Our data segment selector
	movw    %ax, %ds                # -> DS: Data Segment
	movw    %ax, %es                # -> ES: Extra Segment
	movw    %ax, %fs                # -> FS
	movw    %ax, %gs                # -> GS
	movw    %ax, %ss

	# Set up the stack pointer and call into C.
	movl    $start16, %esp
	call    bootloader

	# If bootmain returns (it shouldn't), loop.
error:
	jmp    error

# Bootstrap GDT
.p2align 2                                # force 4 byte alignment
gdt:
	.byte   0,0,0,0,0,0,0,0               # null segment
	.byte   0xFF,0xFF,0,0,0,0x9A,0xCF,0   # code segment
	.byte   0xFF,0xFF,0,0,0,0x92,0xCF,0   # data segment
gdtdesc:
	.word   23                            # sizeof(gdt) - 1
	.long   gdt                           # address of gdt

