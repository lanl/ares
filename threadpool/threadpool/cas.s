	.file	"cas.c"
	.text
	.p2align 4,,15
	.globl	cas32
	.type	cas32, @function
cas32:
.LFB0:
	.cfi_startproc
	movl	%esi, %eax
	movl	%esi, -4(%rsp)
	lock cmpxchgl	%edx, (%rdi)
	sete	%al
	ret
	.cfi_endproc
.LFE0:
	.size	cas32, .-cas32
	.p2align 4,,15
	.globl	cas64
	.type	cas64, @function
cas64:
.LFB1:
	.cfi_startproc
	movq	%rsi, %rax
	movq	%rsi, -8(%rsp)
	lock cmpxchgq	%rdx, (%rdi)
	sete	%al
	ret
	.cfi_endproc
.LFE1:
	.size	cas64, .-cas64
	.p2align 4,,15
	.globl	cas128
	.type	cas128, @function
cas128:
.LFB2:
	.cfi_startproc
	pushq	%rbx
	.cfi_def_cfa_offset 16
	.cfi_offset 3, -16
	movq	%rsi, %rax
	movq	%rcx, %rbx
	movq	%r8, %rcx
	movq	%rsi, -16(%rsp)
	movq	%rdx, -8(%rsp)
	lock cmpxchg16b	(%rdi)
	sete	%al
	popq	%rbx
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE2:
	.size	cas128, .-cas128
	.ident	"GCC: (Ubuntu 4.8.4-2ubuntu1~14.04) 4.8.4"
	.section	.note.GNU-stack,"",@progbits
