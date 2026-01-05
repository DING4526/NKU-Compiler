	.text
	.globl main
	.attribute	4, 16
	.attribute arch, "rv64i2p1_m2p0_a2p1_f2p2_d2p2_c2p0"

sub:
.sub_0:
	addi	sp, sp, -288	# prologue_sp
	addi	fp, sp, 0	# set_fp
	sd		fp, 88(sp)
	sd		s1, 96(sp)
	sd		s2, 104(sp)
	sd		s3, 112(sp)
	sd		s4, 120(sp)
	sd		s5, 128(sp)
	sd		s6, 136(sp)
	sd		s7, 144(sp)
	sd		s8, 152(sp)
	sd		s9, 160(sp)
	sd		s10, 168(sp)
	sd		s11, 176(sp)
	fsd		fs0, 184(sp)
	fsd		fs1, 192(sp)
	fsd		fs2, 200(sp)
	fsd		fs3, 208(sp)
	fsd		fs4, 216(sp)
	fsd		fs5, 224(sp)
	fsd		fs6, 232(sp)
	fsd		fs7, 240(sp)
	fsd		fs8, 248(sp)
	fsd		fs9, 256(sp)
	fsd		fs10, 264(sp)
	fsd		fs11, 272(sp)
	ld		s3, 288(sp)	# stack_arg_load_0
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
	jal		x0, .sub_4
.sub_1:
	lw		t2, 80(sp)
	lw		s1, 72(sp)
	slt		t1, t2, s1
	bne		t1, x0, .sub_2
	jal		x0, .sub_3
.sub_2:
	lw		t2, 80(sp)
	zext.w	t1, t2
	li		s5, 0
	add		s6, s5, t1
	li		s7, 4
	mul		t6, s6, s7
	add		s10, a7, t6
	add		s8, a1, t6
	flw		ft0, 0(s8)
	add		s9, a4, t6
	flw		ft1, 0(s9)
	fsub.s	ft2, ft0, ft1
	fsw		ft2, 0(s10)
	add		t4, s2, t6
	add		s11, a2, t6
	flw		ft3, 0(s11)
	add		t3, a5, t6
	flw		ft4, 0(t3)
	fsub.s	ft5, ft3, ft4
	fsw		ft5, 0(t4)
	add		s1, s3, t6
	add		t5, a3, t6
	flw		ft6, 0(t5)
	add		s4, a6, t6
	flw		ft7, 0(s4)
	fsub.s	fs0, ft6, ft7
	fsw		fs0, 0(s1)
	lw		t1, 80(sp)
	lw		s1, 64(sp)
	addw	t2, t1, s1
	addiw	t1, t2, 0
	sw		t1, 80(sp)
	jal		x0, .sub_1
.sub_3:
	li		t1, 0
	li		t2, 0
	addw	s1, t1, t2
	addi	a0, s1, 0
	ld		fp, 88(sp)
	ld		s1, 96(sp)
	ld		s2, 104(sp)
	ld		s3, 112(sp)
	ld		s4, 120(sp)
	ld		s5, 128(sp)
	ld		s6, 136(sp)
	ld		s7, 144(sp)
	ld		s8, 152(sp)
	ld		s9, 160(sp)
	ld		s10, 168(sp)
	ld		s11, 176(sp)
	fld		fs0, 184(sp)
	fld		fs1, 192(sp)
	fld		fs2, 200(sp)
	fld		fs3, 208(sp)
	fld		fs4, 216(sp)
	fld		fs5, 224(sp)
	fld		fs6, 232(sp)
	fld		fs7, 240(sp)
	fld		fs8, 248(sp)
	fld		fs9, 256(sp)
	fld		fs10, 264(sp)
	fld		fs11, 272(sp)
	addi	sp, sp, 288	# epilogue_sp
	jalr	x0, ra, 0
.sub_4:
	li		t1, 1
	li		t2, 0
	addw	s1, t1, t2
	sw		s1, 64(sp)
	li		s1, 3
	li		s4, 0
	addw	t1, s1, s4
	sw		t1, 72(sp)
	addiw	t1, a0, 0
	sw		t1, 80(sp)
	jal		x0, .sub_1
main:
.main_0:
	addi	sp, sp, -480	# prologue_sp
	sd		ra, 176(sp)
	addi	fp, sp, 0	# set_fp
	sd		fp, 280(sp)
	sd		s1, 288(sp)
	sd		s2, 296(sp)
	sd		s3, 304(sp)
	sd		s4, 312(sp)
	sd		s5, 320(sp)
	sd		s6, 328(sp)
	sd		s7, 336(sp)
	sd		s8, 344(sp)
	sd		s9, 352(sp)
	sd		s10, 360(sp)
	sd		s11, 368(sp)
	fsd		fs0, 376(sp)
	fsd		fs1, 384(sp)
	fsd		fs2, 392(sp)
	fsd		fs3, 400(sp)
	fsd		fs4, 408(sp)
	fsd		fs5, 416(sp)
	fsd		fs6, 424(sp)
	fsd		fs7, 432(sp)
	fsd		fs8, 440(sp)
	fsd		fs9, 448(sp)
	fsd		fs10, 456(sp)
	fsd		fs11, 464(sp)
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
	slt		t1, s1, s2
	bne		t1, x0, .main_2
	jal		x0, .main_3
.main_2:
	addw	t1, s3, s1
	zext.w	t2, t1
	li		a0, 0
	add		a1, a0, t2
	li		a2, 4
	mul		s11, a1, a2
	addi	a3, sp, 16
	add		a4, a3, s11
	fcvt.s.w	ft0, s1
	fsw		ft0, 0(a4)
	addi	a5, sp, 32
	add		a6, a5, s11
	fsw		ft0, 0(a6)
	addi	a7, sp, 48
	add		s6, a7, s11
	fsw		ft0, 0(s6)
	addi	s7, sp, 64
	add		s8, s7, s11
	fsw		ft0, 0(s8)
	addi	s9, sp, 80
	add		s10, s9, s11
	fsw		ft0, 0(s10)
	addi	t3, sp, 96
	add		t4, t3, s11
	fsw		ft0, 0(t4)
	addw	t5, s1, s4
	addiw	s1, t5, 0
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
	call	sub
	addiw	s6, a0, 0
	addiw	s7, s6, 0
	jal		x0, .main_14
.main_4:
	slt		t1, s6, s8
	bne		t1, x0, .main_5
	jal		x0, .main_6
.main_5:
	lw		t2, 272(sp)
	addw	t1, t2, s6
	zext.w	t2, t1
	li		a0, 0
	add		a1, a0, t2
	li		a2, 4
	mul		a3, a1, a2
	addi	a4, sp, 112
	add		a5, a4, a3
	flw		ft0, 0(a5)
	fcvt.w.s	a6, ft0, rtz
	addi	a0, a6, 0
	call	putint
	lw		t1, 264(sp)
	addw	s11, s6, t1
	addiw	s6, s11, 0
	jal		x0, .main_4
.main_6:
	li		t1, 10
	li		t2, 0
	addw	a2, t1, t2
	li		a0, 0
	li		a1, 0
	addw	s11, a0, a1
	addi	a0, a2, 0
	call	putch
	addiw	t1, s11, 0
	sw		t1, 184(sp)
	jal		x0, .main_15
.main_7:
	lw		t2, 216(sp)
	lw		s1, 192(sp)
	slt		t1, t2, s1
	bne		t1, x0, .main_8
	jal		x0, .main_9
.main_8:
	lw		t2, 200(sp)
	lw		s1, 216(sp)
	addw	t1, t2, s1
	zext.w	t2, t1
	li		a0, 0
	add		a1, a0, t2
	li		a2, 4
	mul		a3, a1, a2
	addi	a4, sp, 144
	add		a5, a4, a3
	flw		ft0, 0(a5)
	fcvt.w.s	a6, ft0, rtz
	addi	a0, a6, 0
	call	putint
	lw		t1, 216(sp)
	lw		t2, 208(sp)
	addw	s11, t1, t2
	addiw	t1, s11, 0
	sw		t1, 216(sp)
	jal		x0, .main_7
.main_9:
	li		t1, 10
	li		t2, 0
	addw	a2, t1, t2
	li		a0, 0
	li		a1, 0
	addw	s11, a0, a1
	addi	a0, a2, 0
	call	putch
	addiw	t1, s11, 0
	sw		t1, 224(sp)
	jal		x0, .main_16
.main_10:
	lw		t2, 256(sp)
	lw		s1, 232(sp)
	slt		t1, t2, s1
	bne		t1, x0, .main_11
	jal		x0, .main_12
.main_11:
	lw		t2, 240(sp)
	lw		s1, 256(sp)
	addw	t1, t2, s1
	zext.w	t2, t1
	li		a0, 0
	add		a1, a0, t2
	li		a2, 4
	mul		a3, a1, a2
	addi	a4, sp, 160
	add		a5, a4, a3
	flw		ft0, 0(a5)
	fcvt.w.s	a6, ft0, rtz
	addi	a0, a6, 0
	call	putint
	lw		t1, 256(sp)
	lw		t2, 248(sp)
	addw	s11, t1, t2
	addiw	t1, s11, 0
	sw		t1, 256(sp)
	jal		x0, .main_10
.main_12:
	li		t1, 10
	li		t2, 0
	addw	a0, t1, t2
	addi	a0, a0, 0
	call	putch
	li		s11, 0
	li		s10, 0
	addw	s9, s11, s10
	addi	a0, s9, 0
	ld		ra, 176(sp)
	ld		fp, 280(sp)
	ld		s1, 288(sp)
	ld		s2, 296(sp)
	ld		s3, 304(sp)
	ld		s4, 312(sp)
	ld		s5, 320(sp)
	ld		s6, 328(sp)
	ld		s7, 336(sp)
	ld		s8, 344(sp)
	ld		s9, 352(sp)
	ld		s10, 360(sp)
	ld		s11, 368(sp)
	fld		fs0, 376(sp)
	fld		fs1, 384(sp)
	fld		fs2, 392(sp)
	fld		fs3, 400(sp)
	fld		fs4, 408(sp)
	fld		fs5, 416(sp)
	fld		fs6, 424(sp)
	fld		fs7, 432(sp)
	fld		fs8, 440(sp)
	fld		fs9, 448(sp)
	fld		fs10, 456(sp)
	fld		fs11, 464(sp)
	addi	sp, sp, 480	# epilogue_sp
	jalr	x0, ra, 0
.main_13:
	li		t1, 1
	li		t2, 0
	addw	s4, t1, t2
	li		a0, 0
	li		a1, 0
	addw	s3, a0, a1
	li		a2, 3
	li		a3, 0
	addw	s2, a2, a3
	addiw	s1, s5, 0
	jal		x0, .main_1
.main_14:
	li		t1, 1
	li		t2, 0
	addw	s1, t1, t2
	sw		s1, 264(sp)
	li		s1, 0
	li		a0, 0
	addw	t1, s1, a0
	sw		t1, 272(sp)
	li		a1, 3
	li		a2, 0
	addw	s8, a1, a2
	addiw	s6, s7, 0
	jal		x0, .main_4
.main_15:
	li		t1, 1
	li		t2, 0
	addw	s1, t1, t2
	sw		s1, 208(sp)
	li		s1, 0
	li		a0, 0
	addw	t1, s1, a0
	sw		t1, 200(sp)
	li		a1, 3
	li		a2, 0
	addw	t1, a1, a2
	sw		t1, 192(sp)
	lw		t1, 184(sp)
	addiw	t2, t1, 0
	sw		t2, 216(sp)
	jal		x0, .main_7
.main_16:
	li		t1, 1
	li		t2, 0
	addw	s1, t1, t2
	sw		s1, 248(sp)
	li		s1, 0
	li		a0, 0
	addw	t1, s1, a0
	sw		t1, 240(sp)
	li		a1, 3
	li		a2, 0
	addw	t1, a1, a2
	sw		t1, 232(sp)
	lw		t1, 224(sp)
	addiw	t2, t1, 0
	sw		t2, 256(sp)
	jal		x0, .main_10
	.data
N:
	.word	0
M:
	.word	0
L:
	.word	0
