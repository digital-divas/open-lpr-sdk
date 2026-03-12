import sys
import os
import platform

src_path = sys.argv[1]
obj_path = sys.argv[2]
sym_name = sys.argv[3]

size = os.path.getsize(src_path)
asm_src = obj_path + ".s"
is_mac = platform.system() == "Darwin"

with open(asm_src, "w") as f:
    if is_mac:
        # macOS / clang assembler
        f.write(".section __DATA,__const\n")
        f.write(f".globl _{sym_name}\n")
        f.write(f"_{sym_name}:\n")
        f.write(f'  .incbin "{src_path}"\n')
        f.write(f".globl _{sym_name}_len\n")
        f.write(f"_{sym_name}_len:\n")
        f.write(f"  .long {size}\n")
    else:
        # Linux / GNU assembler
        f.write(".section .rodata\n")
        f.write(f".globl {sym_name}\n")
        f.write(f".type {sym_name}, @object\n")
        f.write(f"{sym_name}:\n")
        f.write(f'  .incbin "{src_path}"\n')
        f.write(f".globl {sym_name}_len\n")
        f.write(f".type {sym_name}_len, @object\n")
        f.write(f"{sym_name}_len:\n")
        f.write(f"  .long {size}\n")

ret = os.system(f'as "{asm_src}" -o "{obj_path}"')

if os.path.exists(asm_src):
    os.remove(asm_src)

sys.exit(ret)
