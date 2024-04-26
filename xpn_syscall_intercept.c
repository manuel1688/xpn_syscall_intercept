#include <libsyscall_intercept_hook_point.h>
#include <syscall.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <sys/uio.h> 
#include "xpn_syscall_intercept.h"
#include "debug_msg.h"
#include <stdlib.h>
#include <linux/kernel.h>
#include "xpn_file_descriptor.h"
#include <fcntl.h>

static int hook(long syscall_number,long arg0, long arg1,long arg2, long arg3,long arg4, long arg5,long *result){
  
  (void) arg2;
  (void) arg3;
  (void) arg4;
  (void) arg5;
  int fd,ret;

  if(syscall_number == SYS_creat){
    char *path = (char *)arg0;
    mode_t mode = (mode_t)arg1; 

    if(is_xpn_prefix(path))
    {
      xpn_adaptor_keepInit();
      fd  = xpn_creat((const char *)skip_xpn_prefix(path),mode);
      ret = add_xpn_file_to_fdstable(fd);
      *result = ret;
    }
    else
    {
      *result = syscall_no_intercept(SYS_creat, arg0, arg1);
    }
    return 0;
  }
  else if(syscall_number == SYS_write)
  {
    ssize_t ret = -1;
    int fd = (int)arg0;
    const void *buf = (const void *)arg1;
    size_t nbyte = (size_t)arg2;
    struct generic_fd virtual_fd = fdstable_get(fd);
    
    if(virtual_fd.type == FD_XPN)
    {
      xpn_adaptor_keepInit();

      if (virtual_fd.is_file == 0)
      {
        errno = EISDIR;
        return -1;
      }
      ret = xpn_write(virtual_fd.real_fd, (void *)buf, nbyte);
      *result = ret;
    }
    else
    {
      *result = syscall_no_intercept(SYS_write, arg0, arg1, arg2);
    }
    return 0;
  }
  else if(syscall_number == SYS_close)
  {
    int ret = -1;
    int fd = (int)arg0;
    struct generic_fd virtual_fd = fdstable_get(fd);

    if(virtual_fd.type == FD_XPN)
    {
      xpn_adaptor_keepInit ();
      ret = xpn_close(virtual_fd.real_fd);
      fdstable_remove(fd);
      *result = ret;
    }
    else
    {
      *result = syscall_no_intercept(SYS_close, arg0);
    }
    return 0;
  }
  else if(syscall_number == SYS_openat)
  {
    //TODO: agregar soporte para el modo
    //TODO: agregar soporte a file descriptor en el primer argumento
    char *path = (char *)arg1;
    int flags = (int)arg2;

    if (is_xpn_prefix(path))
    {
      xpn_adaptor_keepInit();
      fd = xpn_open(skip_xpn_prefix(path), flags);
      ret = add_xpn_file_to_fdstable(fd);
      *result = ret; 
    }
    else 
    {
      *result = syscall_no_intercept(SYS_openat, arg0, arg1, arg2);
    }
    return 0;
  }
  else if(syscall_number == SYS_read)
  {
    int fd = (int)arg0;
    void *buf = (void *)arg1;
    size_t nbyte = (size_t)arg2;
    ssize_t ret = -1;
    struct generic_fd virtual_fd = fdstable_get(fd);
    if(virtual_fd.type == FD_XPN)
    {
      xpn_adaptor_keepInit ();
      if (virtual_fd.is_file == 0)
      {
        errno = EISDIR;
        return -1;
      }
      ret = xpn_read(virtual_fd.real_fd, buf, nbyte);
      *result = ret;
    }
    else
    {
      *result = syscall_no_intercept(SYS_read, arg0, arg1, arg2);
    }
    return 0;
  }
  else if(syscall_number == SYS_pwrite64)
  {
    ssize_t ret = -1;
    int fd = (int)arg0;
    const void *buf = (const void *)arg1;
    size_t count = (size_t)arg2;
    off_t offset = (off_t)arg3;
    struct generic_fd virtual_fd = fdstable_get (fd);
    if(virtual_fd.type == FD_XPN)
    {
      xpn_adaptor_keepInit ();
      if (virtual_fd.is_file == 0)
      {
        errno = EISDIR;
        return -1;
      }

      ret = xpn_lseek(virtual_fd.real_fd, offset, SEEK_SET);
      if (ret != -1) {
        ret = xpn_write(virtual_fd.real_fd, buf, count);
      }
      if (ret != -1) {
        xpn_lseek(virtual_fd.real_fd, -ret, SEEK_CUR);
      }
      *result = ret;
    }
    else
    {
      *result = syscall_no_intercept(SYS_pwrite64, arg0, arg1, arg2, arg3);
    }
    return 0;
  }
  else if(syscall_number == SYS_lseek)
  {
    off_t ret = (off_t) -1;
    int fd = (int)arg0;
    off_t offset = (off_t)arg1;
    int whence = (int)arg2;
    struct generic_fd virtual_fd = fdstable_get(fd);
    // printf("SYS_lseek");
    if(virtual_fd.type == FD_XPN)
    {
      xpn_adaptor_keepInit ();
      // printf("offset: %ld\n", offset);
      ret = xpn_lseek(virtual_fd.real_fd, offset, whence);
      *result = ret;
    }
    else
    {
      *result = syscall_no_intercept(SYS_lseek, arg0, arg1, arg2);
    }
    return 0;
  }
  else if(syscall_number == SYS_pread64){
    int fd = (int)arg0;
    void *buf = (void *)arg1;
    size_t count = (size_t)arg2;
    off_t offset = (off_t)arg3;

    ssize_t ret = -1;
    struct generic_fd virtual_fd = fdstable_get(fd);
    // printf("SYS_pread64\n");
    if(virtual_fd.type == FD_XPN)
    {
      xpn_adaptor_keepInit ();
      if (virtual_fd.is_file == 0)
      {
        errno = EISDIR;
        return -1;
      }
      ret = xpn_lseek(virtual_fd.real_fd, offset, SEEK_SET);
      if (ret != -1) {
        ret = xpn_read(virtual_fd.real_fd, buf, count);
      }
      if (ret != -1) {
        xpn_lseek(virtual_fd.real_fd, -ret, SEEK_CUR);
      }
      *result = ret;
    }
    else
    {
      *result = syscall_no_intercept(SYS_pread64, arg0, arg1, arg2, arg3);
    }
    return 0;
  }
  else if(syscall_number == SYS_ftruncate){
    int fd = (int)arg0;
    off_t length = (off_t)arg1;

    int ret = -1;
    struct generic_fd virtual_fd = fdstable_get ( fd );
    // printf("SYS_ftruncate\n");
    if(virtual_fd.type == FD_XPN)
    {
      xpn_adaptor_keepInit ();
      ret = xpn_ftruncate(virtual_fd.real_fd, length);
      *result = ret;
    }
    else
    {
      *result = syscall_no_intercept(SYS_ftruncate, arg0, arg1);
    }
    return 0;
  }
  else if(syscall_number == SYS_newfstatat)
  // TODO: Investigar donde es que el stat se pasa a la syscall newfstatat
  { 
    // (AT_FDCWD, "/tmp/expand/P1/demo.txt", 0x7ffd6c137000, 0x0)
    int fd = (int)arg0;
    char *path = (char *)arg1;
    struct stat *buf = (struct stat *)arg2;
    int ret = -1;

    if (fd != -100){
      
      struct generic_fd virtual_fd = fdstable_get(fd);
      if (virtual_fd.type == FD_XPN)
      {
        xpn_adaptor_keepInit ();
        ret = xpn_fstat(virtual_fd.real_fd, buf);
        *result = ret;
      }
      else
      {
        *result = syscall_no_intercept(SYS_newfstatat, arg0, arg1, arg2, arg3);
      }
    
    } else if (is_xpn_prefix(path))
    {
      xpn_adaptor_keepInit();
      ret = xpn_stat(skip_xpn_prefix(path), buf);
      *result = ret;
    }
    else
    {
      *result = syscall_no_intercept(SYS_newfstatat, arg0, arg1, arg2, arg3);
    }
    return 0;
  }
  // else if(syscall_number == SYS_lstat) //TODO: Rectificar si en efecto se esta intersentado lstat por apartde de la syscall newfstatat
  // {
  //   char *path = (char *)arg0;
  //   struct stat *buf = (struct stat *)arg1;
  //   int ret;
  //   if (is_xpn_prefix(path))
  //   {
  //     xpn_adaptor_keepInit ();
  //     ret = xpn_stat(skip_xpn_prefix(path), buf);
  //     *result = ret;
  //   }
  //   else
  //   {
  //     *result = syscall_no_intercept(SYS_lstat, arg0, arg1);
  //   }
  //   return 0;
  // }
  return 1;
}

static __attribute__((constructor)) void
init(void)
{
  intercept_hook_point = hook;
}
