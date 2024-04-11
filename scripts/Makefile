#
# This Makefile is for expand bypass (syscalls interceptions) library
#

# xpn_file_descriptor.h se incluye en xpn_syscall_intercept.c, ajustar este codigo para que al compilar xpn_syscall_intercept.c considere a xpn_file_descriptor.c y xpn_file_descriptor.h
prefix = /home/lab/bin/xpn
CC=/home/lab/bin/mpich/bin/mpicc

#-I../../../mxml
#-L../../src/xpn_client -L../../../mxml
# cc xpn_syscall_intercept.c -lsyscall_intercept -fpic -shared -o xpn_syscall_intercept.so
FLAGS=-Wall -Wextra -fpic -std=c11
INCLUDE=-I../../include -I../../include/base -I../../include/xpn_client -I../../include/bypass -I../../../mxml -DNOT_TO_USE_FCNTL_H -DNOT_TO_USE_STDLIB_H
LIBS=-L../xpn_client -lxpn -lmxml -lpthread -ldl -lsyscall_intercept -fpic

all:
	$(CC) $(FLAGS) $(INCLUDE) -c xpn_file_descriptor.c xpn_syscall_intercept.c
	$(CC) -shared -o xpn_syscall_intercept.so xpn_file_descriptor.o xpn_syscall_intercept.o $(LIBS)
	
install:
	cp -f xpn_syscall_intercept.so ${prefix}/lib

clean:
	rm -f *.o
	rm -f *.so
