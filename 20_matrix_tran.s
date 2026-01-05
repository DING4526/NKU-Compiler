	.text
	.globl main
	.attribute	4, 16
	.attribute arch, "rv64i2p1_m2p0_a2p1_f2p2_d2p2_c2p0"

tran:
.tran_0:
	addi	sp, sp, -432	# prologue_sp
	addi	fp, sp, 0	# set_fp
	sd		fp, 240(sp)
	sd		s1, 248(sp)
	sd		s2, 256(sp)
	sd		s3, 264(sp)
	sd		s4, 272(sp)
	sd		s5, 280(sp)
	sd		s6, 288(sp)
	sd		s7, 296(sp)
	sd		s8, 304(sp)
	sd		s9, 312(sp)
	sd		s10, 320(sp)
	sd		s11, 328(sp)
	fsd		fs0, 336(sp)
	fsd		fs1, 344(sp)
	fsd		fs2, 352(sp)
	fsd		fs3, 360(sp)
	fsd		fs4, 368(sp)
	fsd		fs5, 376(sp)
	fsd		fs6, 384(sp)
	fsd		fs7, 392(sp)
	fsd		fs8, 400(sp)
	fsd		fs9, 408(sp)
	fsd		fs10, 416(sp)
	fsd		fs11, 424(sp)
	ld		t1, 432(sp)	# stack_arg_load_0
	sd		t1, 144(sp)
	sd		a0, 0(sp)
	sd		a1, 8(sp)
	sd		a2, 16(sp)
	sd		a3, 24(sp)
	sd		a4, 32(sp)
	sd		a5, 40(sp)
	sd		a6, 48(sp)
	sd		a7, 56(sp)
	ld		t1, 0(sp)
	sd		t1, 192(sp)
	ld		t1, 8(sp)
	sd		t1, 112(sp)
	ld		t1, 16(sp)
	sd		t1, 152(sp)
	ld		t6, 24(sp)
	ld		t6, 32(sp)
	ld		t6, 40(sp)
	ld		t1, 48(sp)
	sd		t1, 184(sp)
	ld		t1, 56(sp)
	sd		t1, 104(sp)
	li		t1, 0
	li		t2, 0
	addw	s1, t1, t2
	sw		s1, 224(sp)
	li		s1, 2
	li		a0, 0
	addw	a1, s1, a0
	zext.w	a2, a1
	li		s10, 0
	add		a3, s10, a2
	li		a4, 4
	mul		t1, a3, a4
	sd		t1, 160(sp)
	ld		t1, 104(sp)
	ld		t2, 160(sp)
	add		s6, t1, t2
	li		a5, 1
	li		a6, 0
	addw	a7, a5, a6
	zext.w	s2, a7
	add		s3, s10, s2
	li		s4, 4
	mul		t1, s3, s4
	sd		t1, 120(sp)
	ld		t1, 152(sp)
	ld		t2, 120(sp)
	add		s5, t1, t2
	flw		ft0, 0(s5)
	fsw		ft0, 0(s6)
	ld		t1, 144(sp)
	ld		t2, 120(sp)
	add		s8, t1, t2
	ld		t1, 112(sp)
	ld		t2, 160(sp)
	add		s7, t1, t2
	flw		ft1, 0(s7)
	fsw		ft1, 0(s8)
	ld		t1, 184(sp)
	ld		t2, 120(sp)
	add		t5, t1, t2
	lw		t1, 224(sp)
	zext.w	s9, t1
	add		s11, s10, s9
	li		t3, 4
	mul		t1, s11, t3
	sd		t1, 200(sp)
	ld		t1, 112(sp)
	ld		t2, 200(sp)
	add		t4, t1, t2
	flw		ft2, 0(t4)
	fsw		ft2, 0(t5)
	ld		t1, 184(sp)
	ld		t2, 160(sp)
	add		s1, t1, t2
	sd		s1, 64(sp)
	ld		t1, 152(sp)
	ld		t2, 200(sp)
	add		s1, t1, t2
	sd		s1, 232(sp)
	ld		t1, 232(sp)
	flw		ft3, 0(t1)
	ld		t1, 64(sp)
	fsw		ft3, 0(t1)
	ld		t1, 104(sp)
	ld		t2, 200(sp)
	add		s1, t1, t2
	sd		s1, 80(sp)
	ld		t1, 192(sp)
	ld		t2, 120(sp)
	add		s1, t1, t2
	sd		s1, 72(sp)
	ld		t1, 72(sp)
	flw		ft4, 0(t1)
	ld		t1, 80(sp)
	fsw		ft4, 0(t1)
	ld		t1, 144(sp)
	ld		t2, 200(sp)
	add		s1, t1, t2
	sd		s1, 96(sp)
	ld		t1, 192(sp)
	ld		t2, 160(sp)
	add		s1, t1, t2
	sd		s1, 88(sp)
	ld		t1, 88(sp)
	flw		ft5, 0(t1)
	ld		t1, 96(sp)
	fsw		ft5, 0(t1)
	ld		t1, 104(sp)
	ld		t2, 120(sp)
	add		s1, t1, t2
	sd		s1, 136(sp)
	ld		t1, 112(sp)
	ld		t2, 120(sp)
	add		s1, t1, t2
	sd		s1, 128(sp)
	ld		t1, 128(sp)
	flw		ft6, 0(t1)
	ld		t1, 136(sp)
	fsw		ft6, 0(t1)
	ld		t1, 144(sp)
	ld		t2, 160(sp)
	add		s1, t1, t2
	sd		s1, 176(sp)
	ld		t1, 152(sp)
	ld		t2, 160(sp)
	add		s1, t1, t2
	sd		s1, 168(sp)
	ld		t1, 168(sp)
	flw		ft7, 0(t1)
	ld		t1, 176(sp)
	fsw		ft7, 0(t1)
	ld		t1, 184(sp)
	ld		t2, 200(sp)
	add		s1, t1, t2
	sd		s1, 216(sp)
	ld		t1, 192(sp)
	ld		t2, 200(sp)
	add		s1, t1, t2
	sd		s1, 208(sp)
	ld		t1, 208(sp)
	flw		fs0, 0(t1)
	ld		t1, 216(sp)
	fsw		fs0, 0(t1)
	lw		t1, 224(sp)
	addi	a0, t1, 0
	ld		fp, 240(sp)
	ld		s1, 248(sp)
	ld		s2, 256(sp)
	ld		s3, 264(sp)
	ld		s4, 272(sp)
	ld		s5, 280(sp)
	ld		s6, 288(sp)
	ld		s7, 296(sp)
	ld		s8, 304(sp)
	ld		s9, 312(sp)
	ld		s10, 320(sp)
	ld		s11, 328(sp)
	fld		fs0, 336(sp)
	fld		fs1, 344(sp)
	fld		fs2, 352(sp)
	fld		fs3, 360(sp)
	fld		fs4, 368(sp)
	fld		fs5, 376(sp)
	fld		fs6, 384(sp)
	fld		fs7, 392(sp)
	fld		fs8, 400(sp)
	fld		fs9, 408(sp)
	fld		fs10, 416(sp)
	fld		fs11, 424(sp)
	addi	sp, sp, 432	# epilogue_sp
	jalr	x0, ra, 0
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
	call	tran
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
	lw		t1, 248(sp)
	slt		s1, t1, t2
	bne		s1, x0, .main_8
	jal		x0, .main_9
.main_8:
	lw		t2, 240(sp)
	lw		s1, 248(sp)
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
	lw		t1, 248(sp)
	lw		t2, 200(sp)
	addw	s11, t1, t2
	addiw	t1, s11, 0
	sw		t1, 248(sp)
	jal		x0, .main_7
.main_9:
	li		t1, 10
	li		t2, 0
	addw	a1, t1, t2
	li		s1, 0
	li		a0, 0
	addw	s11, s1, a0
	addi	a0, a1, 0
	call	putch
	addiw	t1, s11, 0
	sw		t1, 208(sp)
	jal		x0, .main_16
.main_10:
	la		t1, N
	lw		t2, 0(t1)
	lw		t1, 232(sp)
	slt		s1, t1, t2
	bne		s1, x0, .main_11
	jal		x0, .main_12
.main_11:
	lw		t2, 216(sp)
	lw		s1, 232(sp)
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
	lw		t1, 232(sp)
	lw		t2, 224(sp)
	addw	s11, t1, t2
	addiw	t1, s11, 0
	sw		t1, 232(sp)
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
	sw		t1, 240(sp)
	lw		t1, 192(sp)
	addiw	t2, t1, 0
	sw		t2, 248(sp)
	jal		x0, .main_7
.main_16:
	li		t1, 1
	li		t2, 0
	addw	s1, t1, t2
	sw		s1, 224(sp)
	li		s1, 0
	li		a0, 0
	addw	t1, s1, a0
	sw		t1, 216(sp)
	lw		t1, 208(sp)
	addiw	t2, t1, 0
	sw		t2, 232(sp)
	jal		x0, .main_10
	.data
M:
	.word	0
L:
	.word	0
N:
	.word	0
