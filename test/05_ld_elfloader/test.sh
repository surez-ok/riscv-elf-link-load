qemu-riscv64 -cpu rv64,g=true,c=true,v=true -L /study/riscv-gnu-toolchain/build_glibc/sysroot ./elfloader hello_dyn main
qemu-riscv64 -cpu rv64,g=true,c=true,v=true -L /study/riscv-gnu-toolchain/build_glibc/sysroot ./elfloader hello_static main
