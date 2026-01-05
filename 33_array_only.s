	.text
	.globl main
	.attribute	4, 16
	.attribute arch, "rv64i2p1_m2p0_a2p1_f2p2_d2p2_c2p0"

inc_impl:
.inc_impl_0:
	addi	sp, sp, -224	# prologue_sp
	sd		ra, 16(sp)
	sd		a0, 0(sp)
	sw		a1, 8(sp)
	addi	fp, sp, 0	# set_fp
	sd		fp, 24(sp)
	sd		s1, 32(sp)
	sd		s2, 40(sp)
	sd		s3, 48(sp)
	sd		s4, 56(sp)
	sd		s5, 64(sp)
	sd		s6, 72(sp)
	sd		s7, 80(sp)
	sd		s8, 88(sp)
	sd		s9, 96(sp)
	sd		s10, 104(sp)
	sd		s11, 112(sp)
	fsd		fs0, 120(sp)
	fsd		fs1, 128(sp)
	fsd		fs2, 136(sp)
	fsd		fs3, 144(sp)
	fsd		fs4, 152(sp)
	fsd		fs5, 160(sp)
	fsd		fs6, 168(sp)
	fsd		fs7, 176(sp)
	fsd		fs8, 184(sp)
	fsd		fs9, 192(sp)
	fsd		fs10, 200(sp)
	fsd		fs11, 208(sp)
	ld		a3, 0(sp)
	lw		a2, 8(sp)
	li		t1, 0
	li		t2, 0
	addw	s1, t1, t2
	xor		a0, a2, s1
	sltiu	a1, a0, 1
	bne		a1, x0, .inc_impl_1
	jal		x0, .inc_impl_3
.inc_impl_1:
	li		t1, 0
	li		t2, 0
	addw	s1, t1, t2
	zext.w	a0, s1
	li		a1, 0
	add		a4, a1, a0
	li		a5, 4
	mul		a6, a4, a5
	add		s6, a3, a6
	addi	a7, s6, 0
	lw		s4, 0(a7)
	li		s2, 1
	li		s3, 0
	addw	s5, s2, s3
	addw	s7, s4, s5
	sw		s7, 0(s6)
	jal		x0, .inc_impl_2
.inc_impl_2:
	ld		ra, 16(sp)
	ld		fp, 24(sp)
	ld		s1, 32(sp)
	ld		s2, 40(sp)
	ld		s3, 48(sp)
	ld		s4, 56(sp)
	ld		s5, 64(sp)
	ld		s6, 72(sp)
	ld		s7, 80(sp)
	ld		s8, 88(sp)
	ld		s9, 96(sp)
	ld		s10, 104(sp)
	ld		s11, 112(sp)
	fld		fs0, 120(sp)
	fld		fs1, 128(sp)
	fld		fs2, 136(sp)
	fld		fs3, 144(sp)
	fld		fs4, 152(sp)
	fld		fs5, 160(sp)
	fld		fs6, 168(sp)
	fld		fs7, 176(sp)
	fld		fs8, 184(sp)
	fld		fs9, 192(sp)
	fld		fs10, 200(sp)
	fld		fs11, 208(sp)
	addi	sp, sp, 224	# epilogue_sp
	jalr	x0, ra, 0
.inc_impl_3:
	li		t1, 0
	li		t2, 0
	addw	s1, t1, t2
	zext.w	a0, s1
	li		a1, 0
	add		a4, a1, a0
	li		a5, 4
	mul		a6, a4, a5
	add		s6, a3, a6
	addi	a7, s6, 0
	lw		s4, 0(a7)
	li		s2, 2
	li		s3, 0
	addw	s5, s2, s3
	mulw	s7, s4, s5
	sw		s7, 0(s6)
	li		s8, 1
	li		s9, 0
	addw	s10, s8, s9
	subw	s11, a2, s10
	addi	a0, a3, 0
	addi	a1, s11, 0
	call	inc_impl
	jal		x0, .inc_impl_2
inc:
.inc_0:
	addi	sp, sp, -208	# prologue_sp
	sd		ra, 8(sp)
	sd		a0, 0(sp)
	addi	fp, sp, 0	# set_fp
	sd		fp, 16(sp)
	sd		s1, 24(sp)
	sd		s2, 32(sp)
	sd		s3, 40(sp)
	sd		s4, 48(sp)
	sd		s5, 56(sp)
	sd		s6, 64(sp)
	sd		s7, 72(sp)
	sd		s8, 80(sp)
	sd		s9, 88(sp)
	sd		s10, 96(sp)
	sd		s11, 104(sp)
	fsd		fs0, 112(sp)
	fsd		fs1, 120(sp)
	fsd		fs2, 128(sp)
	fsd		fs3, 136(sp)
	fsd		fs4, 144(sp)
	fsd		fs5, 152(sp)
	fsd		fs6, 160(sp)
	fsd		fs7, 168(sp)
	fsd		fs8, 176(sp)
	fsd		fs9, 184(sp)
	fsd		fs10, 192(sp)
	fsd		fs11, 200(sp)
	ld		s2, 0(sp)
	li		t1, 0
	li		t2, 0
	addw	s1, t1, t2
	addw	a0, s1, s1
	zext.w	a1, a0
	li		a2, 0
	add		a3, a2, a1
	li		a4, 4
	mul		a5, a3, a4
	la		a6, k
	add		a7, a6, a5
	lw		s3, 0(a7)
	addi	a0, s2, 0
	addi	a1, s3, 0
	call	inc_impl
	ld		ra, 8(sp)
	ld		fp, 16(sp)
	ld		s1, 24(sp)
	ld		s2, 32(sp)
	ld		s3, 40(sp)
	ld		s4, 48(sp)
	ld		s5, 56(sp)
	ld		s6, 64(sp)
	ld		s7, 72(sp)
	ld		s8, 80(sp)
	ld		s9, 88(sp)
	ld		s10, 96(sp)
	ld		s11, 104(sp)
	fld		fs0, 112(sp)
	fld		fs1, 120(sp)
	fld		fs2, 128(sp)
	fld		fs3, 136(sp)
	fld		fs4, 144(sp)
	fld		fs5, 152(sp)
	fld		fs6, 160(sp)
	fld		fs7, 168(sp)
	fld		fs8, 176(sp)
	fld		fs9, 184(sp)
	fld		fs10, 192(sp)
	fld		fs11, 200(sp)
	addi	sp, sp, 208	# epilogue_sp
	jalr	x0, ra, 0
add_impl:
.add_impl_0:
	addi	sp, sp, -224	# prologue_sp
	sd		ra, 24(sp)
	sd		a0, 0(sp)
	sd		a1, 8(sp)
	sw		a2, 16(sp)
	addi	fp, sp, 0	# set_fp
	sd		fp, 32(sp)
	sd		s1, 40(sp)
	sd		s2, 48(sp)
	sd		s3, 56(sp)
	sd		s4, 64(sp)
	sd		s5, 72(sp)
	sd		s6, 80(sp)
	sd		s7, 88(sp)
	sd		s8, 96(sp)
	sd		s9, 104(sp)
	sd		s10, 112(sp)
	sd		s11, 120(sp)
	fsd		fs0, 128(sp)
	fsd		fs1, 136(sp)
	fsd		fs2, 144(sp)
	fsd		fs3, 152(sp)
	fsd		fs4, 160(sp)
	fsd		fs5, 168(sp)
	fsd		fs6, 176(sp)
	fsd		fs7, 184(sp)
	fsd		fs8, 192(sp)
	fsd		fs9, 200(sp)
	fsd		fs10, 208(sp)
	fsd		fs11, 216(sp)
	ld		a3, 0(sp)
	ld		a4, 8(sp)
	lw		a2, 16(sp)
	li		t1, 0
	li		t2, 0
	addw	s1, t1, t2
	xor		a0, a2, s1
	sltiu	a1, a0, 1
	bne		a1, x0, .add_impl_1
	jal		x0, .add_impl_3
.add_impl_1:
	li		t1, 0
	li		t2, 0
	addw	s1, t1, t2
	zext.w	a0, s1
	li		a1, 0
	add		a5, a1, a0
	li		a6, 4
	mul		s2, a5, a6
	add		s6, a3, s2
	addi	a7, s6, 0
	lw		s4, 0(a7)
	add		s3, a4, s2
	lw		s5, 0(s3)
	addw	s7, s4, s5
	sw		s7, 0(s6)
	jal		x0, .add_impl_2
.add_impl_2:
	ld		ra, 24(sp)
	ld		fp, 32(sp)
	ld		s1, 40(sp)
	ld		s2, 48(sp)
	ld		s3, 56(sp)
	ld		s4, 64(sp)
	ld		s5, 72(sp)
	ld		s6, 80(sp)
	ld		s7, 88(sp)
	ld		s8, 96(sp)
	ld		s9, 104(sp)
	ld		s10, 112(sp)
	ld		s11, 120(sp)
	fld		fs0, 128(sp)
	fld		fs1, 136(sp)
	fld		fs2, 144(sp)
	fld		fs3, 152(sp)
	fld		fs4, 160(sp)
	fld		fs5, 168(sp)
	fld		fs6, 176(sp)
	fld		fs7, 184(sp)
	fld		fs8, 192(sp)
	fld		fs9, 200(sp)
	fld		fs10, 208(sp)
	fld		fs11, 216(sp)
	addi	sp, sp, 224	# epilogue_sp
	jalr	x0, ra, 0
.add_impl_3:
	li		t1, 0
	li		t2, 0
	addw	s1, t1, t2
	zext.w	a0, s1
	li		a1, 0
	add		a5, a1, a0
	li		a6, 4
	mul		a7, a5, a6
	add		s7, a3, a7
	addi	s2, s7, 0
	lw		s5, 0(s2)
	li		s3, 2
	li		s4, 0
	addw	s6, s3, s4
	mulw	s8, s5, s6
	sw		s8, 0(s7)
	li		s9, 1
	li		s10, 0
	addw	s11, s9, s10
	subw	t3, a2, s11
	addi	a0, a3, 0
	addi	a1, a4, 0
	addi	a2, t3, 0
	call	add_impl
	jal		x0, .add_impl_2
add:
.add_0:
	addi	sp, sp, -224	# prologue_sp
	sd		ra, 16(sp)
	sd		a0, 0(sp)
	sd		a1, 8(sp)
	addi	fp, sp, 0	# set_fp
	sd		fp, 24(sp)
	sd		s1, 32(sp)
	sd		s2, 40(sp)
	sd		s3, 48(sp)
	sd		s4, 56(sp)
	sd		s5, 64(sp)
	sd		s6, 72(sp)
	sd		s7, 80(sp)
	sd		s8, 88(sp)
	sd		s9, 96(sp)
	sd		s10, 104(sp)
	sd		s11, 112(sp)
	fsd		fs0, 120(sp)
	fsd		fs1, 128(sp)
	fsd		fs2, 136(sp)
	fsd		fs3, 144(sp)
	fsd		fs4, 152(sp)
	fsd		fs5, 160(sp)
	fsd		fs6, 168(sp)
	fsd		fs7, 176(sp)
	fsd		fs8, 184(sp)
	fsd		fs9, 192(sp)
	fsd		fs10, 200(sp)
	fsd		fs11, 208(sp)
	ld		s2, 0(sp)
	ld		s3, 8(sp)
	li		t1, 0
	li		t2, 0
	addw	s1, t1, t2
	addw	a0, s1, s1
	zext.w	a1, a0
	li		a2, 0
	add		a3, a2, a1
	li		a4, 4
	mul		a5, a3, a4
	la		a6, k
	add		a7, a6, a5
	lw		s4, 0(a7)
	addi	a0, s2, 0
	addi	a1, s3, 0
	addi	a2, s4, 0
	call	add_impl
	ld		ra, 16(sp)
	ld		fp, 24(sp)
	ld		s1, 32(sp)
	ld		s2, 40(sp)
	ld		s3, 48(sp)
	ld		s4, 56(sp)
	ld		s5, 64(sp)
	ld		s6, 72(sp)
	ld		s7, 80(sp)
	ld		s8, 88(sp)
	ld		s9, 96(sp)
	ld		s10, 104(sp)
	ld		s11, 112(sp)
	fld		fs0, 120(sp)
	fld		fs1, 128(sp)
	fld		fs2, 136(sp)
	fld		fs3, 144(sp)
	fld		fs4, 152(sp)
	fld		fs5, 160(sp)
	fld		fs6, 168(sp)
	fld		fs7, 176(sp)
	fld		fs8, 184(sp)
	fld		fs9, 192(sp)
	fld		fs10, 200(sp)
	fld		fs11, 208(sp)
	addi	sp, sp, 224	# epilogue_sp
	jalr	x0, ra, 0
sub_impl:
.sub_impl_0:
	addi	sp, sp, -224	# prologue_sp
	sd		ra, 24(sp)
	sd		a0, 0(sp)
	sd		a1, 8(sp)
	sw		a2, 16(sp)
	addi	fp, sp, 0	# set_fp
	sd		fp, 32(sp)
	sd		s1, 40(sp)
	sd		s2, 48(sp)
	sd		s3, 56(sp)
	sd		s4, 64(sp)
	sd		s5, 72(sp)
	sd		s6, 80(sp)
	sd		s7, 88(sp)
	sd		s8, 96(sp)
	sd		s9, 104(sp)
	sd		s10, 112(sp)
	sd		s11, 120(sp)
	fsd		fs0, 128(sp)
	fsd		fs1, 136(sp)
	fsd		fs2, 144(sp)
	fsd		fs3, 152(sp)
	fsd		fs4, 160(sp)
	fsd		fs5, 168(sp)
	fsd		fs6, 176(sp)
	fsd		fs7, 184(sp)
	fsd		fs8, 192(sp)
	fsd		fs9, 200(sp)
	fsd		fs10, 208(sp)
	fsd		fs11, 216(sp)
	ld		a3, 0(sp)
	ld		a4, 8(sp)
	lw		a2, 16(sp)
	li		t1, 0
	li		t2, 0
	addw	s1, t1, t2
	xor		a0, a2, s1
	sltiu	a1, a0, 1
	bne		a1, x0, .sub_impl_1
	jal		x0, .sub_impl_3
.sub_impl_1:
	li		t1, 0
	li		t2, 0
	addw	s1, t1, t2
	zext.w	a0, s1
	li		a1, 0
	add		a5, a1, a0
	li		a6, 4
	mul		s2, a5, a6
	add		s6, a3, s2
	addi	a7, s6, 0
	lw		s4, 0(a7)
	add		s3, a4, s2
	lw		s5, 0(s3)
	subw	s7, s4, s5
	sw		s7, 0(s6)
	jal		x0, .sub_impl_2
.sub_impl_2:
	ld		ra, 24(sp)
	ld		fp, 32(sp)
	ld		s1, 40(sp)
	ld		s2, 48(sp)
	ld		s3, 56(sp)
	ld		s4, 64(sp)
	ld		s5, 72(sp)
	ld		s6, 80(sp)
	ld		s7, 88(sp)
	ld		s8, 96(sp)
	ld		s9, 104(sp)
	ld		s10, 112(sp)
	ld		s11, 120(sp)
	fld		fs0, 128(sp)
	fld		fs1, 136(sp)
	fld		fs2, 144(sp)
	fld		fs3, 152(sp)
	fld		fs4, 160(sp)
	fld		fs5, 168(sp)
	fld		fs6, 176(sp)
	fld		fs7, 184(sp)
	fld		fs8, 192(sp)
	fld		fs9, 200(sp)
	fld		fs10, 208(sp)
	fld		fs11, 216(sp)
	addi	sp, sp, 224	# epilogue_sp
	jalr	x0, ra, 0
.sub_impl_3:
	li		t1, 0
	li		t2, 0
	addw	s1, t1, t2
	zext.w	a0, s1
	li		a1, 0
	add		a5, a1, a0
	li		a6, 4
	mul		a7, a5, a6
	add		s7, a3, a7
	addi	s2, s7, 0
	lw		s5, 0(s2)
	li		s3, 2
	li		s4, 0
	addw	s6, s3, s4
	mulw	s8, s5, s6
	sw		s8, 0(s7)
	li		s9, 1
	li		s10, 0
	addw	s11, s9, s10
	subw	t3, a2, s11
	addi	a0, a3, 0
	addi	a1, a4, 0
	addi	a2, t3, 0
	call	sub_impl
	jal		x0, .sub_impl_2
sub:
.sub_0:
	addi	sp, sp, -224	# prologue_sp
	sd		ra, 16(sp)
	sd		a0, 0(sp)
	sd		a1, 8(sp)
	addi	fp, sp, 0	# set_fp
	sd		fp, 24(sp)
	sd		s1, 32(sp)
	sd		s2, 40(sp)
	sd		s3, 48(sp)
	sd		s4, 56(sp)
	sd		s5, 64(sp)
	sd		s6, 72(sp)
	sd		s7, 80(sp)
	sd		s8, 88(sp)
	sd		s9, 96(sp)
	sd		s10, 104(sp)
	sd		s11, 112(sp)
	fsd		fs0, 120(sp)
	fsd		fs1, 128(sp)
	fsd		fs2, 136(sp)
	fsd		fs3, 144(sp)
	fsd		fs4, 152(sp)
	fsd		fs5, 160(sp)
	fsd		fs6, 168(sp)
	fsd		fs7, 176(sp)
	fsd		fs8, 184(sp)
	fsd		fs9, 192(sp)
	fsd		fs10, 200(sp)
	fsd		fs11, 208(sp)
	ld		s2, 0(sp)
	ld		s3, 8(sp)
	li		t1, 0
	li		t2, 0
	addw	s1, t1, t2
	addw	a0, s1, s1
	zext.w	a1, a0
	li		a2, 0
	add		a3, a2, a1
	li		a4, 4
	mul		a5, a3, a4
	la		a6, k
	add		a7, a6, a5
	lw		s4, 0(a7)
	addi	a0, s2, 0
	addi	a1, s3, 0
	addi	a2, s4, 0
	call	sub_impl
	ld		ra, 16(sp)
	ld		fp, 24(sp)
	ld		s1, 32(sp)
	ld		s2, 40(sp)
	ld		s3, 48(sp)
	ld		s4, 56(sp)
	ld		s5, 64(sp)
	ld		s6, 72(sp)
	ld		s7, 80(sp)
	ld		s8, 88(sp)
	ld		s9, 96(sp)
	ld		s10, 104(sp)
	ld		s11, 112(sp)
	fld		fs0, 120(sp)
	fld		fs1, 128(sp)
	fld		fs2, 136(sp)
	fld		fs3, 144(sp)
	fld		fs4, 152(sp)
	fld		fs5, 160(sp)
	fld		fs6, 168(sp)
	fld		fs7, 176(sp)
	fld		fs8, 184(sp)
	fld		fs9, 192(sp)
	fld		fs10, 200(sp)
	fld		fs11, 208(sp)
	addi	sp, sp, 224	# epilogue_sp
	jalr	x0, ra, 0
main:
.main_0:
	addi	sp, sp, -816	# prologue_sp
	sd		ra, 40(sp)
	addi	fp, sp, 0	# set_fp
	sd		fp, 616(sp)
	sd		s1, 624(sp)
	sd		s2, 632(sp)
	sd		s3, 640(sp)
	sd		s4, 648(sp)
	sd		s5, 656(sp)
	sd		s6, 664(sp)
	sd		s7, 672(sp)
	sd		s8, 680(sp)
	sd		s9, 688(sp)
	sd		s10, 696(sp)
	sd		s11, 704(sp)
	fsd		fs0, 712(sp)
	fsd		fs1, 720(sp)
	fsd		fs2, 728(sp)
	fsd		fs3, 736(sp)
	fsd		fs4, 744(sp)
	fsd		fs5, 752(sp)
	fsd		fs6, 760(sp)
	fsd		fs7, 768(sp)
	fsd		fs8, 776(sp)
	fsd		fs9, 784(sp)
	fsd		fs10, 792(sp)
	fsd		fs11, 800(sp)
	addi	a0, sp, 0
	li		a1, 0
	li		a2, 4
	li		a3, 0
	call	memset
	addi	a0, sp, 16
	li		a1, 0
	li		a2, 4
	li		a3, 0
	call	memset
	addi	a0, sp, 32
	li		a1, 0
	li		a2, 8
	li		a3, 0
	call	memset
	li		s1, 1
	li		s2, 0
	addw	s3, s1, s2
	li		s4, 0
	subw	s11, s4, s3
	li		s5, 0
	addw	t1, s4, s5
	sw		t1, 152(sp)
	lw		t1, 152(sp)
	zext.w	s6, t1
	li		t1, 0
	sd		t1, 184(sp)
	ld		t1, 184(sp)
	add		s7, t1, s6
	li		s8, 4
	mul		s9, s7, s8
	addi	s10, sp, 32
	add		t1, s10, s9
	sd		t1, 48(sp)
	ld		t1, 48(sp)
	sw		s11, 0(t1)
	lw		t1, 152(sp)
	lw		t2, 152(sp)
	addw	s1, t1, t1
	sw		s1, 56(sp)
	lw		t1, 56(sp)
	zext.w	t2, t1
	sd		t2, 64(sp)
	ld		t1, 184(sp)
	ld		t2, 64(sp)
	add		s1, t1, t2
	sd		s1, 72(sp)
	li		t1, 4
	sd		t1, 80(sp)
	ld		t1, 72(sp)
	ld		t2, 80(sp)
	mul		s1, t1, t2
	sd		s1, 112(sp)
	la		t1, k
	sd		t1, 88(sp)
	ld		t1, 88(sp)
	ld		t2, 112(sp)
	add		s1, t1, t2
	sd		s1, 96(sp)
	call	getint
	addiw	t1, a0, 0
	sw		t1, 104(sp)
	lw		t1, 104(sp)
	ld		t2, 96(sp)
	sw		t1, 0(t2)
	addi	t1, sp, 16
	sd		t1, 120(sp)
	ld		t1, 120(sp)
	ld		t2, 112(sp)
	add		s1, t1, t2
	sd		s1, 128(sp)
	call	getint
	addiw	t1, a0, 0
	sw		t1, 136(sp)
	lw		t1, 136(sp)
	ld		t2, 128(sp)
	sw		t1, 0(t2)
	li		t1, 2
	sd		t1, 144(sp)
	ld		t1, 144(sp)
	lw		t2, 152(sp)
	mulw	s1, t1, t2
	sw		s1, 160(sp)
	lw		t1, 152(sp)
	lw		t2, 160(sp)
	addw	s1, t1, t2
	sw		s1, 168(sp)
	lw		t1, 168(sp)
	zext.w	t2, t1
	sd		t2, 176(sp)
	ld		t1, 184(sp)
	ld		t2, 176(sp)
	add		s1, t1, t2
	sd		s1, 192(sp)
	li		t1, 4
	sd		t1, 200(sp)
	ld		t1, 192(sp)
	ld		t2, 200(sp)
	mul		s1, t1, t2
	sd		s1, 208(sp)
	addi	t1, sp, 32
	sd		t1, 216(sp)
	ld		t1, 216(sp)
	ld		t2, 208(sp)
	add		s1, t1, t2
	sd		s1, 224(sp)
	ld		t1, 224(sp)
	addi	a0, t1, 0
	call	getarray
	addiw	t1, a0, 0
	jal		x0, .main_10
.main_1:
	ld		t2, 272(sp)
	lw		t1, 0(t2)
	li		t2, 0
	xor		s1, t1, t2
	sltu	a0, x0, s1
	bne		a0, x0, .main_2
	jal		x0, .main_3
.main_2:
	ld		t2, 264(sp)
	lw		t1, 0(t2)
	ld		t2, 256(sp)
	sw		t1, 0(t2)
	jal		x0, .main_9
.main_3:
	li		t1, 10
	li		t2, 0
	addw	s1, t1, t2
	addi	a0, s1, 0
	call	putch
	li		s8, 0
	li		s9, 0
	addw	s10, s8, s9
	addi	a0, s10, 0
	ld		ra, 40(sp)
	ld		fp, 616(sp)
	ld		s1, 624(sp)
	ld		s2, 632(sp)
	ld		s3, 640(sp)
	ld		s4, 648(sp)
	ld		s5, 656(sp)
	ld		s6, 664(sp)
	ld		s7, 672(sp)
	ld		s8, 680(sp)
	ld		s9, 688(sp)
	ld		s10, 696(sp)
	ld		s11, 704(sp)
	fld		fs0, 712(sp)
	fld		fs1, 720(sp)
	fld		fs2, 728(sp)
	fld		fs3, 736(sp)
	fld		fs4, 744(sp)
	fld		fs5, 752(sp)
	fld		fs6, 760(sp)
	fld		fs7, 768(sp)
	fld		fs8, 776(sp)
	fld		fs9, 784(sp)
	fld		fs10, 792(sp)
	fld		fs11, 800(sp)
	addi	sp, sp, 816	# epilogue_sp
	jalr	x0, ra, 0
.main_4:
	ld		t2, 336(sp)
	lw		t1, 0(t2)
	lw		s1, 328(sp)
	slt		t2, t1, s1
	bne		t2, x0, .main_5
	jal		x0, .main_6
.main_5:
	ld		t2, 320(sp)
	lw		t1, 0(t2)
	addi	a0, t1, 0
	call	putint
	ld		t1, 312(sp)
	lw		s4, 0(t1)
	addi	a0, s4, 0
	call	putint
	ld		t1, 304(sp)
	lw		s3, 0(t1)
	addi	a0, s3, 0
	call	putint
	ld		t1, 296(sp)
	lw		s2, 0(t1)
	addi	a0, s2, 0
	call	putint
	ld		t1, 288(sp)
	addi	a0, t1, 0
	addi	a1, sp, 16
	call	add
	addi	a0, sp, 0
	addi	a1, sp, 16
	call	add
	ld		t1, 280(sp)
	addi	a0, t1, 0
	addi	a1, sp, 16
	call	sub
	jal		x0, .main_4
.main_6:
	la		t1, i
	addi	a0, t1, 0
	call	inc
	la		s2, i
	addi	a0, s2, 0
	ld		t1, 248(sp)
	addi	a1, t1, 0
	call	add
	ld		t1, 240(sp)
	lw		s3, 0(t1)
	ld		t1, 232(sp)
	lw		s4, 0(t1)
	xor		s5, s3, s4
	sltiu	s6, s5, 1
	bne		s6, x0, .main_7
	jal		x0, .main_8
.main_7:
	jal		x0, .main_3
.main_8:
	jal		x0, .main_1
.main_9:
	li		t1, 0
	li		t2, 0
	addw	a2, t1, t2
	li		a0, 5
	li		a1, 0
	addw	t1, a0, a1
	sw		t1, 328(sp)
	addiw	t1, a2, 0
	sw		t1, 352(sp)
	addw	a4, a2, a2
	lw		t1, 352(sp)
	lw		t2, 352(sp)
	addw	s4, t1, t1
	li		a3, 2
	lw		t1, 352(sp)
	mulw	s11, a3, t1
	zext.w	a5, a4
	li		t1, 0
	sd		t1, 384(sp)
	ld		t1, 384(sp)
	add		a6, t1, a5
	li		a7, 4
	mul		s2, a6, a7
	addi	s3, sp, 0
	add		t1, s3, s2
	sd		t1, 336(sp)
	zext.w	s5, s4
	ld		t1, 384(sp)
	add		s6, t1, s5
	li		t3, 4
	mul		t6, s6, t3
	addi	t4, sp, 0
	add		t1, t4, t6
	sd		t1, 312(sp)
	la		t5, i
	add		t1, t5, t6
	sd		t1, 320(sp)
	addi	s7, sp, 16
	add		t1, s7, t6
	sd		t1, 304(sp)
	lw		t1, 352(sp)
	addw	t2, t1, s11
	sw		t2, 360(sp)
	lw		t1, 360(sp)
	zext.w	s10, t1
	ld		t1, 384(sp)
	add		s9, t1, s10
	li		s8, 4
	mul		s1, s9, s8
	addi	t1, sp, 32
	sd		t1, 344(sp)
	ld		t1, 344(sp)
	add		t2, t1, s1
	sd		t2, 280(sp)
	ld		t1, 280(sp)
	addi	t2, t1, 0
	sd		t2, 288(sp)
	lw		t1, 360(sp)
	lw		t2, 352(sp)
	addw	s1, t1, t2
	sw		s1, 368(sp)
	lw		t1, 368(sp)
	zext.w	t2, t1
	sd		t2, 376(sp)
	ld		t1, 384(sp)
	ld		t2, 376(sp)
	add		s1, t1, t2
	sd		s1, 392(sp)
	li		t1, 4
	sd		t1, 400(sp)
	ld		t1, 392(sp)
	ld		t2, 400(sp)
	mul		s1, t1, t2
	sd		s1, 408(sp)
	addi	t1, sp, 32
	sd		t1, 416(sp)
	ld		t1, 416(sp)
	ld		t2, 408(sp)
	add		s1, t1, t2
	sd		s1, 296(sp)
	jal		x0, .main_4
.main_10:
	li		t1, 0
	li		t2, 0
	addw	s3, t1, t2
	addiw	a1, s3, 0
	addiw	t1, s3, 0
	sw		t1, 496(sp)
	li		s1, 1
	li		a0, 0
	addw	t1, s1, a0
	sw		t1, 480(sp)
	addw	a3, s3, s3
	li		a2, 2
	mulw	s4, a2, s3
	addw	s5, a1, a1
	lw		t1, 496(sp)
	lw		t2, 496(sp)
	addw	s11, t1, t1
	lw		t1, 496(sp)
	mulw	t2, a2, t1
	sw		t2, 432(sp)
	zext.w	a4, a3
	li		t1, 0
	sd		t1, 576(sp)
	ld		t1, 576(sp)
	add		a5, t1, a4
	li		a6, 4
	mul		a7, a5, a6
	la		s2, i
	add		t1, s2, a7
	sd		t1, 240(sp)
	addw	t1, s3, s4
	sw		t1, 488(sp)
	zext.w	s6, s5
	ld		t1, 576(sp)
	add		s7, t1, s6
	li		s8, 4
	mul		s9, s7, s8
	addi	s10, sp, 16
	add		t1, s10, s9
	sd		t1, 272(sp)
	zext.w	t3, s11
	ld		t1, 576(sp)
	add		t4, t1, t3
	li		t5, 4
	mul		t6, t4, t5
	addi	t1, sp, 0
	sd		t1, 424(sp)
	ld		t1, 424(sp)
	add		t2, t1, t6
	sd		t2, 256(sp)
	lw		t1, 496(sp)
	lw		t2, 432(sp)
	addw	s1, t1, t2
	sw		s1, 504(sp)
	lw		t1, 488(sp)
	zext.w	t2, t1
	sd		t2, 440(sp)
	ld		t1, 576(sp)
	ld		t2, 440(sp)
	add		s1, t1, t2
	sd		s1, 448(sp)
	li		t1, 4
	sd		t1, 456(sp)
	ld		t1, 448(sp)
	ld		t2, 456(sp)
	mul		s1, t1, t2
	sd		s1, 464(sp)
	addi	t1, sp, 32
	sd		t1, 472(sp)
	ld		t1, 472(sp)
	ld		t2, 464(sp)
	add		s1, t1, t2
	sd		s1, 248(sp)
	lw		t1, 488(sp)
	lw		t2, 480(sp)
	addw	s1, t1, t2
	sw		s1, 512(sp)
	lw		t1, 504(sp)
	lw		t2, 496(sp)
	addw	s1, t1, t2
	sw		s1, 560(sp)
	lw		t1, 512(sp)
	zext.w	t2, t1
	sd		t2, 520(sp)
	ld		t1, 576(sp)
	ld		t2, 520(sp)
	add		s1, t1, t2
	sd		s1, 528(sp)
	li		t1, 4
	sd		t1, 536(sp)
	ld		t1, 528(sp)
	ld		t2, 536(sp)
	mul		s1, t1, t2
	sd		s1, 544(sp)
	addi	t1, sp, 32
	sd		t1, 552(sp)
	ld		t1, 552(sp)
	ld		t2, 544(sp)
	add		s1, t1, t2
	sd		s1, 232(sp)
	lw		t1, 560(sp)
	zext.w	t2, t1
	sd		t2, 568(sp)
	ld		t1, 576(sp)
	ld		t2, 568(sp)
	add		s1, t1, t2
	sd		s1, 584(sp)
	li		t1, 4
	sd		t1, 592(sp)
	ld		t1, 584(sp)
	ld		t2, 592(sp)
	mul		s1, t1, t2
	sd		s1, 600(sp)
	addi	t1, sp, 32
	sd		t1, 608(sp)
	ld		t1, 608(sp)
	ld		t2, 600(sp)
	add		s1, t1, t2
	sd		s1, 264(sp)
	jal		x0, .main_1
	.data
i:
	.zero	4
k:
	.zero	4
