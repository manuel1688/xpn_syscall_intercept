# Clona el repositorio
#git clone https://github.com/pmem/syscall_intercept.git syscall_intercept
# Navega al directorio del repositorio clonado
#cd syscall_intercept
# Haz checkout al commit específico
#git checkout 2c8765fa292bc9c28a22624c528580d54658813d

rm -r xpn_syscall_intercept_create.so
rm -r xpn_syscall_intercept_create.o
rm -r Makefile
cp scripts/Makefile .
make