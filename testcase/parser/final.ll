; Function Declarations
declare i32 @getint()
declare i32 @getch()
declare i32 @getarray(ptr)
declare float @getfloat()
declare i32 @getfarray(ptr)
declare void @putint(i32)
declare void @putch(i32)
declare void @putarray(i32, ptr)
declare void @putfloat(float)
declare void @putfarray(i32, ptr)
declare void @_sysy_starttime(i32)
declare void @_sysy_stoptime(i32)
declare void @llvm.memset.p0.i32(ptr, i8, i32, i1)

; Global Variable Declarations
@ret = global i32 0

; Function Definitions
define i32 @compare(i32 %reg_1, i32 %reg_2)
{
Block0:
	%reg_3 = alloca i32
	store i32 %reg_1, ptr %reg_3
	%reg_4 = alloca i32
	store i32 %reg_2, ptr %reg_4
	%reg_5 = icmp sle i32 %reg_4, %reg_4
	br i1 %reg_5, label %Block1, label %Block2
Block1:
	%reg_6 = add i32 1, 0
	ret i32 %reg_6
Block2:
	%reg_7 = add i32 0, 0
	ret i32 %reg_7
}

define i32 @add(i32 %reg_1, i32 %reg_2)
{
Block0:
	%reg_3 = alloca i32
	store i32 %reg_1, ptr %reg_3
	%reg_4 = alloca i32
	store i32 %reg_2, ptr %reg_4
	%reg_5 = add i32 %reg_4, %reg_4
	ret i32 %reg_5
}

define void @sav(i32 %reg_1)
{
Block0:
	%reg_2 = alloca i32
	store i32 %reg_1, ptr %reg_2
	store i32 %reg_2, ptr @ret
	%reg_3 = load i32, ptr @ret
	ret void
}

define i32 @fib(i32 %reg_1)
{
Block0:
	%reg_2 = alloca i32
	store i32 %reg_1, ptr %reg_2
	%reg_3 = add i32 1, 0
	%reg_4 = call i32 @compare(i32 %reg_2, i32 %reg_3)
	%reg_5 = icmp ne i32 %reg_4, 0
	br i1 %reg_5, label %Block1, label %Block2
Block1:
	%reg_6 = add i32 2, 0
	%reg_7 = call i32 @compare(i32 %reg_6, i32 %reg_6)
	%reg_8 = icmp ne i32 %reg_7, 0
	br i1 %reg_8, label %Block3, label %Block5
Block2:
	%reg_15 = add i32 2, 0
	%reg_16 = call i32 @compare(i32 %reg_15, i32 %reg_15)
	%reg_17 = icmp ne i32 %reg_16, 0
	br i1 %reg_17, label %Block10, label %Block11
Block3:
	%reg_9 = add i32 5466217, 0
	%reg_10 = icmp ne i32 %reg_9, 0
	br i1 %reg_10, label %Block6, label %Block7
Block4:
Block5:
	%reg_12 = add i32 2, 0
	%reg_13 = sub i32 %reg_12, %reg_12
	%reg_14 = icmp ne i32 %reg_13, 0
	br i1 %reg_14, label %Block8, label %Block9
Block6:
	%reg_11 = add i32 27497, 0
	call void @sav(i32 %reg_11)
	br label %Block7
Block7:
Block8:
	call void @sav(i32 %reg_14)
	br label %Block9
Block9:
Block10:
	%reg_18 = add i32 1, 0
	%reg_19 = sub i32 %reg_17, %reg_18
	%reg_20 = call i32 @fib(i32 %reg_19)
	%reg_21 = add i32 2, 0
	%reg_22 = sub i32 %reg_20, %reg_21
	%reg_23 = call i32 @fib(i32 %reg_22)
	%reg_24 = call i32 @add(i32 %reg_20, i32 %reg_23)
	call void @sav(i32 %reg_24)
	br label %Block11
Block11:
	ret i32 %reg_24
}

define i32 @main()
{
Block0:
	%reg_1 = alloca i32
	store i32 0, ptr %reg_1
	%reg_2 = alloca i32
	%reg_3 = add i32 5, 0
	%reg_4 = add i32 12, 0
	%reg_5 = sub i32 0, %reg_4
	%reg_6 = sub i32 0, %reg_5
	%reg_7 = sub i32 0, %reg_6
	%reg_8 = add i32 11, 0
	%reg_9 = mul i32 %reg_7, %reg_8
	%reg_10 = add i32 6, 0
	%reg_11 = sdiv i32 %reg_9, %reg_10
	%reg_12 = sub i32 %reg_3, %reg_11
	%reg_13 = add i32 3, 0
	%reg_14 = sub i32 0, %reg_13
	%reg_15 = add i32 %reg_12, %reg_14
	store i32 %reg_15, ptr %reg_2
	%reg_16 = alloca i32
	%reg_17 = add i32 123, 0
	%reg_18 = add i32 55, 0
	%reg_19 = add i32 %reg_17, %reg_18
	%reg_20 = add i32 9961, 0
	%reg_21 = sub i32 0, %reg_20
	%reg_22 = sub i32 %reg_19, %reg_21
	store i32 %reg_22, ptr %reg_16
	%reg_23 = alloca i32
	%reg_24 = add i32 4, 0
	%reg_25 = sdiv i32 %reg_23, %reg_24
	store i32 %reg_25, ptr %reg_23
	%reg_26 = alloca i32
	%reg_27 = call i32 @fib(i32 %reg_26)
	store i32 %reg_27, ptr %reg_26
	%reg_28 = add i32 1, 0
Block1:
	%reg_47 = add i32 0, 0
	%reg_48 = icmp ne i32 %reg_47, 0
	br i1 %reg_48, label %Block6, label %Block7
Block2:
	%reg_63 = add i32 1, 0
	%reg_64 = sub i32 0, %reg_63
	ret i32 %reg_64
Block3:
	%reg_38 = sub i32 %reg_37, %reg_37
	%reg_39 = add i32 0, 0
	%reg_40 = icmp ne i32 %reg_38, %reg_39
	br i1 %reg_40, label %Block5, label %Block2
Block4:
	%reg_29 = add i32 4, 0
	%reg_30 = sub i32 0, %reg_29
	%reg_31 = sdiv i32 %reg_28, %reg_30
	%reg_32 = add i32 2, 0
	%reg_33 = srem i32 %reg_31, %reg_32
	%reg_34 = add i32 67, 0
	%reg_35 = add i32 %reg_33, %reg_34
	%reg_36 = add i32 0, 0
	%reg_37 = icmp slt i32 %reg_35, %reg_36
	br i1 %reg_37, label %Block1, label %Block3
Block5:
	%reg_41 = add i32 2, 0
	%reg_42 = add i32 %reg_40, %reg_41
	%reg_43 = add i32 2, 0
	%reg_44 = srem i32 %reg_42, %reg_43
	%reg_45 = add i32 0, 0
	%reg_46 = icmp ne i32 %reg_44, %reg_45
	br i1 %reg_46, label %Block1, label %Block2
Block6:
	br label %Block8
Block7:
	%reg_51 = add i32 1, 0
	%reg_52 = icmp ne i32 %reg_51, 0
	br i1 %reg_52, label %Block11, label %Block12
Block8:
	%reg_49 = add i32 1, 0
	%reg_50 = icmp ne i32 %reg_49, 0
	br i1 %reg_50, label %Block9, label %Block10
Block9:
	br label %Block8
Block10:
Block11:
	br label %Block12
Block12:
	br label %Block13
Block13:
	%reg_53 = add i32 1, 0
	%reg_54 = icmp ne i32 %reg_53, 0
	br i1 %reg_54, label %Block14, label %Block15
Block14:
	%reg_55 = add i32 1, 0
	%reg_56 = sub i32 0, %reg_55
	%reg_57 = add i32 -2147483648, 0
	%reg_58 = sub i32 0, %reg_57
	%reg_59 = sub i32 %reg_56, %reg_58
	call void @putint(i32 %reg_59)
	%reg_60 = add i32 10, 0
	call void @putch(i32 %reg_60)
	br label %Block15
Block15:
	call void @putint(i32 %reg_60)
	%reg_61 = add i32 32, 0
	call void @putch(i32 %reg_61)
	call void @putint(i32 %reg_61)
	%reg_62 = add i32 10, 0
	call void @putch(i32 %reg_62)
}
