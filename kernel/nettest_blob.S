section .rodata
global nettest_bin_start
global nettest_bin_end
align 16
nettest_bin_start:
incbin "build/user/nettest"
nettest_bin_end:
