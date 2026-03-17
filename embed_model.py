import sys
import os

src_path = sys.argv[1]
out_path = sys.argv[2]
sym_name = sys.argv[3]
platform = sys.argv[4]

size = os.path.getsize(src_path)
asm_src = out_path

with open(asm_src, "w") as f:
    if platform in ("ios", "darwin"):
        # Mach-O
        f.write(".section __DATA,__const\n")
        f.write(f".globl _{sym_name}\n")
        f.write(f"_{sym_name}:\n")
        # f.write(f".globl __{sym_name}\n")
        # f.write(f"__{sym_name}:\n")
        f.write(f'  .incbin "{src_path}"\n')
        f.write(f".globl _{sym_name}_len\n")
        f.write(f"_{sym_name}_len:\n")
        # f.write(f".globl __{sym_name}_len\n")
        # f.write(f"__{sym_name}_len:\n")
        f.write(f"  .long {size}\n")
    else:
        # ELF (Android/Linux)
        f.write('.section .rodata,"a"\n')
        f.write(f".global {sym_name}\n")
        f.write(f"{sym_name}:\n")
        f.write(f'  .incbin "{src_path}"\n')
        f.write(f".global {sym_name}_len\n")
        f.write(f"{sym_name}_len:\n")
        f.write(f"  .long {size}\n")

sys.exit()