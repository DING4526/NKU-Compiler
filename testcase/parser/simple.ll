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
@INT_MAX = global i32 2147483647
@INT_MIN = global i32 -2147483648

; Function Definitions
define i32 @add(i32 %reg_1, i32 %reg_2)
{
Block0:
	%reg_3 = alloca i32
	store i32 %reg_1, ptr %reg_3
	%reg_4 = alloca i32
	store i32 %reg_2, ptr %reg_4
	%reg_5 = alloca i32
	%reg_6 = add i32 %reg_5, %reg_5
	store i32 %reg_6, ptr %reg_5
	ret i32 %reg_6
}

define i32 @fib(i32 %reg_1)
{
Block0:
	%reg_2 = alloca i32
	store i32 %reg_1, ptr %reg_2
	%reg_3 = add i32 1, 0
	%reg_4 = icmp sle i32 %reg_2, %reg_3
	br i1 %reg_4, label %Block1, label %Block2
Block1:
	ret i32 %reg_4
Block2:
	%reg_5 = alloca i32
	%reg_6 = add i32 0, 0
	store i32 %reg_6, ptr %reg_5
	%reg_7 = alloca i32
	%reg_8 = add i32 1, 0
	store i32 %reg_8, ptr %reg_7
	%reg_9 = alloca i32
	%reg_10 = add i32 2, 0
	store i32 %reg_10, ptr %reg_9
	br label %Block3
Block3:
	%reg_11 = icmp sle i32 %reg_10, %reg_10
	br i1 %reg_11, label %Block4, label %Block5
Block4:
	%reg_12 = alloca i32
	%reg_13 = call i32 @add(i32 %reg_12, i32 %reg_12)
	store i32 %reg_13, ptr %reg_12
	store i32 %reg_13, ptr %reg_5
	%reg_14 = load i32, ptr %reg_5
	store i32 %reg_14, ptr %reg_7
	%reg_15 = load i32, ptr %reg_7
	%reg_16 = add i32 1, 0
	%reg_17 = call i32 @add(i32 %reg_15, i32 %reg_16)
	store i32 %reg_17, ptr %reg_9
	%reg_18 = load i32, ptr %reg_9
	br label %Block3
Block5:
	ret i32 %reg_18
}

define i32 @main()
{
Block0:
	%reg_1 = alloca i32
	%reg_2 = call i32 @add(i32 %reg_1, i32 %reg_1)
	store i32 %reg_2, ptr %reg_1
	%reg_3 = add i32 10, 0
	%reg_4 = mul i32 %reg_2, %reg_3
	%reg_5 = call i32 @fib(i32 %reg_4)
	call void @putint(i32 %reg_5)
	%reg_6 = add i32 0, 0
	ret i32 %reg_6
}
