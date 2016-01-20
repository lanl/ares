	.file	"main.cpp"
	.text
	.globl	main
	.type	main, @function
main:
.LFB8:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$32, %rsp
	leaq	-32(%rbp), %rax
	movq	%rax, %rdi
	call	_ZN8NodePoolIiEC1Ev
	movl	$0, %eax
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE8:
	.size	main, .-main
	.section	.text._ZN8NodePoolIiEC2Ev,"axG",@progbits,_ZN8NodePoolIiEC5Ev,comdat
	.align 2
	.weak	_ZN8NodePoolIiEC2Ev
	.type	_ZN8NodePoolIiEC2Ev, @function
_ZN8NodePoolIiEC2Ev:
.LFB10:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	movq	$0, (%rax)
	movq	-8(%rbp), %rax
	movq	$0, 8(%rax)
	movq	-8(%rbp), %rax
	movq	$0, 16(%rax)
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE10:
	.size	_ZN8NodePoolIiEC2Ev, .-_ZN8NodePoolIiEC2Ev
	.weak	_ZN8NodePoolIiEC1Ev
	.set	_ZN8NodePoolIiEC1Ev,_ZN8NodePoolIiEC2Ev
	.ident	"GCC: (Ubuntu 4.8.4-2ubuntu1~14.04) 4.8.4"
	.section	.note.GNU-stack,"",@progbits
