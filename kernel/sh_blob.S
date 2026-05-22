section .rodata
global sh_bin_start
global sh_bin_end
align 16
sh_bin_start:
incbin "build/user/sh"
sh_bin_end:
