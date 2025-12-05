#!/usr/bin/env python3
"""
简单 SysY 单文件测试脚本

用法示例：
  python3 my_test.py test.sy
  python3 my_test.py testcase/functional/Basic/1_var.sy \
      --opt 0 --stdin testcase/functional/Basic/1_var.in \
      --std-out testcase/functional/Basic/1_var.out
"""

import argparse
import os
import subprocess
from contextlib import ExitStack


def run(cmd, **kwargs):
    """小工具：跑命令并在失败时抛异常，方便定位问题。"""
    print("[RUN]", " ".join(cmd))
    res = subprocess.run(cmd, **kwargs)
    if res.returncode != 0:
        raise RuntimeError(f"Command failed with code {res.returncode}: {' '.join(cmd)}")
    return res


def append_return_code(path: str, code: int):
    """把返回值追加到输出文件最后一行（和课程测试脚本保持一致）"""
    need_newline = False
    try:
        with open(path, "r", encoding="utf-8") as f:
            content = f.read()
            if content and not content.endswith("\n"):
                need_newline = True
    except FileNotFoundError:
        # 正常来说不会发生，如果发生就直接写
        pass

    with open(path, "a", encoding="utf-8") as f:
        if need_newline:
            f.write("\n")
        f.write(str(code))
        f.write("\n")


def main():
    parser = argparse.ArgumentParser(description="Simple SysY single-file tester")
    parser.add_argument("sy_file", help="要测试的 .sy 源文件")
    parser.add_argument("--opt", type=int, default=0, choices=[0, 1, 2],
                        help="优化等级，对应 -O0/-O1/-O2，默认 0")
    parser.add_argument("--stdin", help="标准输入文件（可选）")
    parser.add_argument("--std-out", help="标准输出文件，用于 diff（可选）")
    parser.add_argument("--prefix", help="生成文件前缀，默认用 sy 文件名去掉后缀")
    args = parser.parse_args()

    sy_path = args.sy_file
    if not os.path.exists(sy_path):
        raise FileNotFoundError(f"SysY file not found: {sy_path}")

    base = args.prefix or os.path.splitext(os.path.basename(sy_path))[0]
    ll_file = f"{base}.ll"
    obj_file = f"{base}.o"
    exe_file = f"{base}.bin"
    out_file = f"{base}.out"

    opt_flag = f"-O{args.opt}"

    # 1. SysY -> LLVM IR
    run(["bin/compiler", "-llvm", sy_path, "-o", ll_file, opt_flag])

    # 2. 检查 IR 语法
    run(["llvm-as", ll_file, "-o", "/dev/null"])

    # 3. LLVM IR -> object
    run(["clang", ll_file, "-c", "-o", obj_file, "-w"])

    # 4. 链接成可执行文件（这里不加 -static，尽量避免环境问题）
    run(["clang", obj_file, "-o", exe_file, "-L./lib", "-lsysy_x86"])

    # 5. 运行程序，收集输出
    print("[INFO] Running executable ...")
    with ExitStack() as stack:
        stdin_f = stack.enter_context(open(args.stdin, "r", encoding="utf-8")) if args.stdin else None
        stdout_f = stack.enter_context(open(out_file, "w", encoding="utf-8"))

        res = subprocess.run(
            [f"./{exe_file}"],
            stdin=stdin_f,
            stdout=stdout_f,
            stderr=subprocess.DEVNULL,
            text=False,
        )

    # 退出码追加到输出文件尾
    append_return_code(out_file, res.returncode)
    print(f"[INFO] Program exit code: {res.returncode}")
    print(f"[INFO] Actual output saved to: {out_file}")

    # 6. 如果提供标准输出文件，做 diff
    if args.std_out:
        if not os.path.exists(args.std_out):
            print(f"[WARN] std-out file not found: {args.std_out}, 跳过 diff")
        else:
            print("[INFO] Diff with standard output ...")
            diff_res = subprocess.run(
                ["diff", "--strip-trailing-cr", out_file, args.std_out, "-b"]
            )
            if diff_res.returncode == 0:
                print("\033[92mAccepted\033[0m")
            else:
                print("\033[91mWrong Answer\033[0m")

    # 你也可以在这里选择删掉中间文件
    # os.remove(obj_file)
    # os.remove(exe_file)


if __name__ == "__main__":
    main()
