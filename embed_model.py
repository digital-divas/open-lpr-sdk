import sys
import os

src_path = sys.argv[1]
obj_path = sys.argv[2]
sym_name = sys.argv[3]

size = os.path.getsize(src_path)
asm_src = obj_path + ".s"

with open(asm_src, "w") as f:
    f.write(".section __DATA,__const\n")
    f.write(f".globl _{sym_name}\n")
    f.write(f"_{sym_name}:\n")
    f.write(f'  .incbin "{src_path}"\n')
    f.write(f".globl _{sym_name}_len\n")
    f.write(f"_{sym_name}_len:\n")
    f.write(f"  .long {size}\n")

ret = os.system(f'as "{asm_src}" -o "{obj_path}"')
os.remove(asm_src)
sys.exit(ret)
