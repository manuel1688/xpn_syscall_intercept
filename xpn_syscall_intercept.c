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
  else if (syscall_number == SYS_close)
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
  else if (syscall_number == SYS_openat)
  {
    //TODO: agregar soporte para el modo
    //TODO: agregar soporte a file descriptor en el primer argumento
    char *path = (char *)arg1;
    printf("path: %s\n", path);
    int flags = (int)arg2;
    printf("flags: %d\n", flags);

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
  return 1;
}

static __attribute__((constructor)) void
init(void)
{
  intercept_hook_point = hook;
}
