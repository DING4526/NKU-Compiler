	.text
	.globl main
	.attribute	4, 16
	.attribute arch, "rv64i2p1_m2p0_a2p1_f2p2_d2p2_c2p0"

add:
.add_0:
	addi	sp, sp, -272	# prologue_sp
	addi	fp, sp, 0	# set_fp
	sd		fp, 80(sp)
	sd		s1, 88(sp)
	sd		s2, 96(sp)
	sd		s3, 104(sp)
	sd		s4, 112(sp)
	sd		s5, 120(sp)
	sd		s6, 128(sp)
	sd		s7, 136(sp)
	sd		s8, 144(sp)
	sd		s9, 152(sp)
	sd		s10, 160(sp)
	sd		s11, 168(sp)
	fsd		fs0, 176(sp)
	fsd		fs1, 184(sp)
	fsd		fs2, 192(sp)
	fsd		fs3, 200(sp)
	fsd		fs4, 208(sp)
	fsd		fs5, 216(sp)
	fsd		fs6, 224(sp)
	fsd		fs7, 232(sp)
	fsd		fs8, 240(sp)
	fsd		fs9, 248(sp)
	fsd		fs10, 256(sp)
	fsd		fs11, 264(sp)
	ld		s3, 272(sp)	# stack_arg_load_0
	sd		a0, 0(sp)
	sd		a1, 8(sp)
	sd		a2, 16(sp)
	sd		a3, 24(sp)
	sd		a4, 32(sp)
	sd		a5, 40(sp)
	sd		a6, 48(sp)
	sd		a7, 56(sp)
	ld		a1, 0(sp)
	ld		a2, 8(sp)
	ld		a3, 16(sp)
	ld		a4, 24(sp)
	ld		a5, 32(sp)
	ld		a6, 40(sp)
	ld		a7, 48(sp)
	ld		s2, 56(sp)
	li		t1, 0
	li		t2, 0
	addw	s1, t1, t2
	addiw	a0, s1, 0
	jal		x0, .add_4
.add_1:
	la		t1, M
	lw		t2, 0(t1)
	lw		t1, 72(sp)
	slt		s1, t1, t2
	bne		s1, x0, .add_2
	jal		x0, .add_3
.add_2:
	lw		t2, 72(sp)
	zext.w	t1, t2
	li		t2, 0
	add		s1, t2, t1
	li		s6, 4
	mul		t5, s1, s6
	add		s9, a7, t5
	add		s7, a1, t5
	flw		ft0, 0(s7)
	add		s8, a4, t5
	flw		ft1, 0(s8)
	fadd.s	ft2, ft0, ft1
	fsw		ft2, 0(s9)
	add		t3, s2, t5
	add		s10, a2, t5
	flw		ft3, 0(s10)
	add		s11, a5, t5
	flw		ft4, 0(s11)
	fadd.s	ft5, ft3, ft4
	fsw		ft5, 0(t3)
	add		s5, s3, t5
	add		t4, a3, t5
	flw		ft6, 0(t4)
	add		t6, a6, t5
	flw		ft7, 0(t6)
	fadd.s	fs0, ft6, ft7
	fsw		fs0, 0(s5)
	lw		t1, 72(sp)
	lw		t2, 64(sp)
	addw	s4, t1, t2
	addiw	t1, s4, 0
	sw		t1, 72(sp)
	jal		x0, .add_1
.add_3:
	li		t1, 0
	li		t2, 0
	addw	s1, t1, t2
	addi	a0, s1, 0
	ld		fp, 80(sp)
	ld		s1, 88(sp)
	ld		s2, 96(sp)
	ld		s3, 104(sp)
	ld		s4, 112(sp)
	ld		s5, 120(sp)
	ld		s6, 128(sp)
	ld		s7, 136(sp)
	ld		s8, 144(sp)
	ld		s9, 152(sp)
	ld		s10, 160(sp)
	ld		s11, 168(sp)
	fld		fs0, 176(sp)
	fld		fs1, 184(sp)
	fld		fs2, 192(sp)
	fld		fs3, 200(sp)
	fld		fs4, 208(sp)
	fld		fs5, 216(sp)
	fld		fs6, 224(sp)
	fld		fs7, 232(sp)
	fld		fs8, 240(sp)
	fld		fs9, 248(sp)
	fld		fs10, 256(sp)
	fld		fs11, 264(sp)
	addi	sp, sp, 272	# epilogue_sp
	jalr	x0, ra, 0
.add_4:
	li		t1, 1
	li		t2, 0
	addw	s1, t1, t2
	sw		s1, 64(sp)
	addiw	t1, a0, 0
	sw		t1, 72(sp)
	jal		x0, .add_1
main:
.main_0:
	addi	sp, sp, -448	# prologue_sp
	sd		ra, 176(sp)
	addi	fp, sp, 0	# set_fp
	sd		fp, 256(sp)
	sd		s1, 264(sp)
	sd		s2, 272(sp)
	sd		s3, 280(sp)
	sd		s4, 288(sp)
	sd		s5, 296(sp)
	sd		s6, 304(sp)
	sd		s7, 312(sp)
	sd		s8, 320(sp)
	sd		s9, 328(sp)
	sd		s10, 336(sp)
	sd		s11, 344(sp)
	fsd		fs0, 352(sp)
	fsd		fs1, 360(sp)
	fsd		fs2, 368(sp)
	fsd		fs3, 376(sp)
	fsd		fs4, 384(sp)
	fsd		fs5, 392(sp)
	fsd		fs6, 400(sp)
	fsd		fs7, 408(sp)
	fsd		fs8, 416(sp)
	fsd		fs9, 424(sp)
	fsd		fs10, 432(sp)
	fsd		fs11, 440(sp)
	li		t1, 3
	li		t2, 0
	addw	a1, t1, t2
	la		s1, N
	sw		a1, 0(s1)
	la		a0, M
	sw		a1, 0(a0)
	la		a2, L
	sw		a1, 0(a2)
	addi	a0, sp, 16
	li		a1, 0
	li		a2, 12
	li		a3, 0
	call	memset
	addi	a0, sp, 32
	li		a1, 0
	li		a2, 12
	li		a3, 0
	call	memset
	addi	a0, sp, 48
	li		a1, 0
	li		a2, 12
	li		a3, 0
	call	memset
	addi	a0, sp, 64
	li		a1, 0
	li		a2, 12
	li		a3, 0
	call	memset
	addi	a0, sp, 80
	li		a1, 0
	li		a2, 12
	li		a3, 0
	call	memset
	addi	a0, sp, 96
	li		a1, 0
	li		a2, 12
	li		a3, 0
	call	memset
	addi	a0, sp, 112
	li		a1, 0
	li		a2, 24
	li		a3, 0
	call	memset
	addi	a0, sp, 144
	li		a1, 0
	li		a2, 12
	li		a3, 0
	call	memset
	addi	a0, sp, 160
	li		a1, 0
	li		a2, 12
	li		a3, 0
	call	memset
	li		s2, 0
	li		s3, 0
	addw	s4, s2, s3
	addiw	s5, s4, 0
	jal		x0, .main_13
.main_1:
	la		t1, M
	lw		t2, 0(t1)
	slt		s1, s2, t2
	bne		s1, x0, .main_2
	jal		x0, .main_3
.main_2:
	addw	t1, s3, s2
	zext.w	t2, t1
	li		s1, 0
	add		a0, s1, t2
	li		a1, 4
	mul		s10, a0, a1
	addi	a2, sp, 16
	add		a3, a2, s10
	fcvt.s.w	ft0, s2
	fsw		ft0, 0(a3)
	addi	a4, sp, 32
	add		a5, a4, s10
	fsw		ft0, 0(a5)
	addi	a6, sp, 48
	add		a7, a6, s10
	fsw		ft0, 0(a7)
	addi	s6, sp, 64
	add		s7, s6, s10
	fsw		ft0, 0(s7)
	addi	s8, sp, 80
	add		s9, s8, s10
	fsw		ft0, 0(s9)
	addi	s11, sp, 96
	add		t3, s11, s10
	fsw		ft0, 0(t3)
	addw	t4, s2, s4
	addiw	s2, t4, 0
	jal		x0, .main_1
.main_3:
	addi	a0, sp, 16
	addi	a1, sp, 32
	addi	a2, sp, 48
	addi	a3, sp, 64
	addi	a4, sp, 80
	addi	a5, sp, 96
	addi	a6, sp, 112
	addi	a7, sp, 144
	addi	t1, sp, 160
	sd		t1, 0(sp)
	call	add
	addiw	s1, a0, 0
	addiw	s6, s1, 0
	jal		x0, .main_14
.main_4:
	la		t1, N
	lw		t2, 0(t1)
	slt		s1, s7, t2
	bne		s1, x0, .main_5
	jal		x0, .main_6
.main_5:
	addw	t1, s8, s7
	zext.w	t2, t1
	li		s1, 0
	add		a0, s1, t2
	li		a1, 4
	mul		a2, a0, a1
	addi	a3, sp, 112
	add		a4, a3, a2
	flw		ft0, 0(a4)
	fcvt.w.s	a5, ft0, rtz
	addi	a0, a5, 0
	call	putint
	lw		t1, 184(sp)
	addw	s10, s7, t1
	addiw	s7, s10, 0
	jal		x0, .main_4
.main_6:
	li		t1, 10
	li		t2, 0
	addw	s1, t1, t2
	addi	a0, s1, 0
	call	putch
	li		s10, 0
	li		s11, 0
	addw	s9, s10, s11
	addiw	t1, s9, 0
	sw		t1, 192(sp)
	jal		x0, .main_15
.main_7:
	la		t1, N
	lw		t2, 0(t1)
	lw		t1, 216(sp)
	slt		s1, t1, t2
	bne		s1, x0, .main_8
	jal		x0, .main_9
.main_8:
	lw		t2, 208(sp)
	lw		s1, 216(sp)
	addw	t1, t2, s1
	zext.w	t2, t1
	li		s1, 0
	add		a0, s1, t2
	li		a1, 4
	mul		a2, a0, a1
	addi	a3, sp, 144
	add		a4, a3, a2
	flw		ft0, 0(a4)
	fcvt.w.s	a5, ft0, rtz
	addi	a0, a5, 0
	call	putint
	lw		t1, 216(sp)
	lw		t2, 200(sp)
	addw	s11, t1, t2
	addiw	t1, s11, 0
	sw		t1, 216(sp)
	jal		x0, .main_7
.main_9:
	li		t1, 10
	li		t2, 0
	addw	s1, t1, t2
	addi	a0, s1, 0
	call	putch
	li		s11, 0
	li		s10, 0
	addw	s9, s11, s10
	addiw	t1, s9, 0
	sw		t1, 224(sp)
	jal		x0, .main_16
.main_10:
	la		t1, N
	lw		t2, 0(t1)
	lw		t1, 248(sp)
	slt		s1, t1, t2
	bne		s1, x0, .main_11
	jal		x0, .main_12
.main_11:
	lw		t2, 240(sp)
	lw		s1, 248(sp)
	addw	t1, t2, s1
	zext.w	t2, t1
	li		s1, 0
	add		a0, s1, t2
	li		a1, 4
	mul		a2, a0, a1
	addi	a3, sp, 160
	add		a4, a3, a2
	flw		ft0, 0(a4)
	fcvt.w.s	a5, ft0, rtz
	addi	a0, a5, 0
	call	putint
	lw		t1, 248(sp)
	lw		t2, 232(sp)
	addw	s11, t1, t2
	addiw	t1, s11, 0
	sw		t1, 248(sp)
	jal		x0, .main_10
.main_12:
	li		t1, 10
	li		t2, 0
	addw	s1, t1, t2
	addi	a0, s1, 0
	call	putch
	li		s11, 0
	li		s10, 0
	addw	s9, s11, s10
	addi	a0, s9, 0
	ld		ra, 176(sp)
	ld		fp, 256(sp)
	ld		s1, 264(sp)
	ld		s2, 272(sp)
	ld		s3, 280(sp)
	ld		s4, 288(sp)
	ld		s5, 296(sp)
	ld		s6, 304(sp)
	ld		s7, 312(sp)
	ld		s8, 320(sp)
	ld		s9, 328(sp)
	ld		s10, 336(sp)
	ld		s11, 344(sp)
	fld		fs0, 352(sp)
	fld		fs1, 360(sp)
	fld		fs2, 368(sp)
	fld		fs3, 376(sp)
	fld		fs4, 384(sp)
	fld		fs5, 392(sp)
	fld		fs6, 400(sp)
	fld		fs7, 408(sp)
	fld		fs8, 416(sp)
	fld		fs9, 424(sp)
	fld		fs10, 432(sp)
	fld		fs11, 440(sp)
	addi	sp, sp, 448	# epilogue_sp
	jalr	x0, ra, 0
.main_13:
	li		t1, 1
	li		t2, 0
	addw	s4, t1, t2
	li		s1, 0
	li		a0, 0
	addw	s3, s1, a0
	addiw	s2, s5, 0
	jal		x0, .main_1
.main_14:
	li		t1, 1
	li		t2, 0
	addw	s1, t1, t2
	sw		s1, 184(sp)
	li		s1, 0
	li		a0, 0
	addw	s8, s1, a0
	addiw	s7, s6, 0
	jal		x0, .main_4
.main_15:
	li		t1, 1
	li		t2, 0
	addw	s1, t1, t2
	sw		s1, 200(sp)
	li		s1, 0
	li		a0, 0
	addw	t1, s1, a0
	sw		t1, 208(sp)
	lw		t1, 192(sp)
	addiw	t2, t1, 0
	sw		t2, 216(sp)
	jal		x0, .main_7
.main_16:
	li		t1, 1
	li		t2, 0
	addw	s1, t1, t2
	sw		s1, 232(sp)
	li		s1, 0
	li		a0, 0
	addw	t1, s1, a0
	sw		t1, 240(sp)
	lw		t1, 224(sp)
	addiw	t2, t1, 0
	sw		t2, 248(sp)
	jal		x0, .main_10
	.data
M:
	.word	0
L:
	.word	0
N:
	.word	0
