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

static int xpn_adaptor_initCalled = 0;
static int xpn_adaptor_initCalled_getenv = 0; 

char *xpn_adaptor_partition_prefix = "/tmp/expand/"; 
int  xpn_prefix_change_verified = 0;

struct generic_fd * fdstable = NULL;
long   fdstable_size = 0L;
long   fdstable_first_free = 0L; 

DIR ** fdsdirtable = NULL;
long   fdsdirtable_size = 0L;
long   fdsdirtable_first_free = 0L;

void fdsdirtable_realloc ( void )
{
  long          old_size = fdsdirtable_size;
  DIR ** fdsdirtable_aux = fdsdirtable;
  
  debug_info("[bypass] >> Before fdsdirtable_realloc....\n");
  
  if ( NULL == fdsdirtable )
  {
    fdsdirtable_size = (long) MAX_DIRS;
    fdsdirtable = (DIR **) malloc(MAX_DIRS * sizeof(DIR *));
  }
  else
  {
    fdsdirtable_size = fdsdirtable_size * 2;
    fdsdirtable = (DIR **) realloc((DIR **)fdsdirtable, fdsdirtable_size * sizeof(DIR *));
  }

  if ( NULL == fdsdirtable )
  {
    debug_error( "[bypass:%s:%d] Error: out of memory\n", __FILE__, __LINE__);
    if (NULL != fdsdirtable_aux) 
    {
      free(fdsdirtable_aux);
    }

    exit(-1);
  }
  
  for (int i = old_size; i < fdsdirtable_size; ++i) 
  {
    fdsdirtable[i] = NULL;
  }

  debug_info("[bypass] << After fdsdirtable_realloc....\n");
}
int fdstable_put ( struct generic_fd fd ) 
{
  debug_info("[bypass] >> Before fdstable_put....\n");

  for (int i = fdstable_first_free; i < fdstable_size; ++i)
  {
    if ( fdstable[i].type == FD_FREE ) 
    {
      fdstable[i] = fd;
      fdstable_first_free = (long)(i + 1);
      debug_info("[bypass]\t fdstable_put -> fd %d ; type: %d ; real_fd: %d\n", i + PLUSXPN, fdstable[i].type, fdstable[i].real_fd);
      debug_info("[bypass] << After fdstable_put....\n");
      return i + PLUSXPN;
    }
  }

  long old_size = fdstable_size;

  fdstable_realloc();

  if ( fdstable[old_size].type == FD_FREE ) 
  {
    fdstable[old_size] = fd;
    debug_info("[bypass]\t fdstable_put -> fd %ld ; type: %d ; real_fd: %d\n", old_size + PLUSXPN, fdstable[old_size].type, fdstable[old_size].real_fd);
    debug_info("[bypass] << After fdstable_put....\n");
    return old_size + PLUSXPN;
  }

  debug_info("[bypass]\t fdstable_put -> -1\n");
  debug_info("[bypass] << After fdstable_put....\n");

  return -1;
}

int add_xpn_file_to_fdstable ( int fd ) 
{
  struct stat st; 
  struct generic_fd virtual_fd; 
  
  debug_info("[bypass] >> Before add_xpn_file_to_fdstable....\n");
  debug_info("[bypass]    1) fd  => %d\n", fd);

  int ret = fd; 

  if (fd < 0) 
  {
    debug_info("[bypass]\t add_xpn_file_to_fdstable -> %d\n", ret);
    debug_info("[bypass] << After add_xpn_file_to_fdstable....\n");
    return ret;
  } 

  xpn_fstat(fd, &st); 

  virtual_fd.type    = FD_XPN;
  virtual_fd.real_fd = fd;
  virtual_fd.is_file = (S_ISDIR(st.st_mode)) ? 0 : 1;

  ret = fdstable_put ( virtual_fd );

  debug_info("[bypass]\t add_xpn_file_to_fdstable -> %d\n", ret);
  debug_info("[bypass] << After add_xpn_file_to_fdstable....\n");

  return ret;
}

void fdsdirtable_init ( void )
{
  debug_info("[bypass] >> Before fdsdirtable_init....\n");
  fdsdirtable_realloc();
  debug_info("[bypass] << After fdsdirtable_init....\n");
}

void fdstable_realloc ( void ) 
{
  long old_size = fdstable_size;
  struct generic_fd * fdstable_aux = fdstable;

  debug_info("[bypass] >> Before fdstable_realloc....\n");

  if ( NULL == fdstable )
  {
    fdstable_size = (long) MAX_FDS;
    fdstable = (struct generic_fd *) malloc(fdstable_size * sizeof(struct generic_fd));
  }
  else
  {
    fdstable_size = fdstable_size * 2;
    fdstable = (struct generic_fd *) realloc((struct generic_fd *)fdstable, fdstable_size * sizeof(struct generic_fd));
  }

  if ( NULL == fdstable )
  {
    debug_error( "[bypass:%s:%d] Error: out of memory\n", __FILE__, __LINE__);
    if (fdstable_aux != NULL) 
    {
      free(fdstable_aux);
    }

    exit(-1);
  }
  
  for (int i = old_size; i < fdstable_size; ++i)
  {
    fdstable[i].type = FD_FREE;
    fdstable[i].real_fd = -1;
    fdstable[i].is_file = -1;
  }

  debug_info("[bypass] << After fdstable_realloc....\n");
}

void fdstable_init ( void ) 
{
  debug_info("[bypass] >> Before fdstable_init....\n");
  fdstable_realloc();
  debug_info("[bypass] << After fdstable_init....\n");
}

int xpn_adaptor_keepInit ( void )
{
  int    ret;
  char * xpn_adaptor_initCalled_env = NULL;
  
  debug_info("[bypass] >> Before xpn_adaptor_keepInit....\n");

  if (0 == xpn_adaptor_initCalled_getenv)
  {
    xpn_adaptor_initCalled_env = getenv("INITCALLED");
    xpn_adaptor_initCalled     = 0;

    if (xpn_adaptor_initCalled_env != NULL) 
    {
      xpn_adaptor_initCalled = atoi(xpn_adaptor_initCalled_env);
    }

    xpn_adaptor_initCalled_getenv = 1;
  }
  
  ret = 0; 

  if (0 == xpn_adaptor_initCalled)
  {
    xpn_adaptor_initCalled = 1; 
    setenv("INITCALLED", "1", 1);

    debug_info("[bypass]\t Before xpn_init()\n");

    fdstable_init ();
    fdsdirtable_init ();
    ret = xpn_init();

    debug_info("[bypass]\t After xpn_init() -> %d\n", ret);

    if (ret < 0)
    {
      debug_error( "ERROR: Expand xpn_init couldn't be initialized :-(\n");
      xpn_adaptor_initCalled = 0;
      setenv("INITCALLED", "0", 1);
    }
    else
    {
      xpn_adaptor_initCalled = 1;
      setenv("INITCALLED", "1", 1);
    }
  }

  debug_info("[bypass]\t xpn_adaptor_keepInit -> %d\n", ret);
  debug_info("[bypass] << After xpn_adaptor_keepInit....\n");
  return ret;
}

int is_xpn_prefix   ( const char * path ) 
{
  if (0 == xpn_prefix_change_verified)
  {
    xpn_prefix_change_verified = 1;
    char * env_prefix = getenv("XPN_MOUNT_POINT");
    if (env_prefix != NULL)
    {
      xpn_adaptor_partition_prefix = env_prefix;
    }
  }
  
  const char *prefix = (const char *)xpn_adaptor_partition_prefix;
  return ( !strncmp(prefix, path, strlen(prefix)) && strlen(path) > strlen(prefix) );
}

const char * skip_xpn_prefix ( const char * path ) 
{
  return (const char *)(path + strlen(xpn_adaptor_partition_prefix));
}

struct generic_fd fdstable_get ( int fd ) 
{
  struct generic_fd ret;
  
  debug_info("[bypass] >> Before fdstable_get....\n");
  debug_info("[bypass]    1) fd  => %d\n", fd);

  if (fd >= PLUSXPN)
  {
    fd = fd - PLUSXPN;
    ret = fdstable[fd];
  }
  else
  {
    ret.type = FD_SYS;
    ret.real_fd = fd;
  }
  debug_info("[bypass]\t fdstable_get -> type: %d ; real_fd: %d\n", ret.type, ret.real_fd);
  debug_info("[bypass] << After fdstable_get....\n");
  return ret;
}

int fdstable_remove ( int fd )
{
  debug_info("[bypass] >> Before fdstable_remove....\n");
  debug_info("[bypass]    1) fd  => %d\n", fd);

  if (fd < PLUSXPN) {
    debug_info("[bypass] << After fdstable_remove....\n");
    return 0;
  }

  fd = fd - PLUSXPN;
  fdstable[fd].type    = FD_FREE;
  fdstable[fd].real_fd = -1;
  fdstable[fd].is_file = -1;

  if (fd < fdstable_first_free){
    fdstable_first_free = fd;
  }
  debug_info("[bypass] << After fdstable_remove....\n");
  return 0;
}

static int hook(long syscall_number,long arg0, long arg1,long arg2, long arg3,long arg4, long arg5,long *result){
  
  (void) arg2;
  (void) arg3;
  (void) arg4;
  (void) arg5;
  int fd,ret;

  if (syscall_number == SYS_creat){
    char *path = (char *)arg0;
    mode_t mode = (mode_t)arg1; 
    // printf("CREAT");
    if (is_xpn_prefix(path))
    {
        xpn_adaptor_keepInit ();
        fd  = xpn_creat((const char *)skip_xpn_prefix(path),mode);
        ret = add_xpn_file_to_fdstable(fd);
        printf("xpn_creat(%s, %o) -> %d\n", skip_xpn_prefix(path), mode, ret);
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
    printf("fd: %d\n", fd); 

    if(virtual_fd.type == FD_XPN)
    {
      xpn_adaptor_keepInit();

      if (virtual_fd.is_file == 0)
      {
        errno = EISDIR;
        return -1;
      }
      ret = xpn_write(virtual_fd.real_fd, (void *)buf, nbyte);
      printf("xpn_write(%d, %p, %lu) -> %ld\n", virtual_fd.real_fd, buf, nbyte, ret);
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
    printf("CLOSE\n");
    printf("fd: %d\n", fd);

    if(virtual_fd.type == FD_XPN)
    {
      xpn_adaptor_keepInit ();
      ret = xpn_close(virtual_fd.real_fd);
      fdstable_remove(fd);
      printf("fdstable_remove(%d)\n", fd);
      *result = ret;
    }
    else
    {
      *result = syscall_no_intercept(SYS_close, arg0);
    }
    return 0;
  }
  // else if(syscall_number  == SYS_open)
  // {
  //   printf("SYS_open\n");
  //   //TODO: agregar soporte para el modo
  //   char *path = (char *)arg0;
  //   printf("path: %s\n", path);
  //   int flags = (int)arg1;
  //   ret = -1;
  //   fd = -1;

  //   printf("flags: %d\n", flags);

  //   if (is_xpn_prefix(path))
  //   {
  //     printf("is_xpn_prefix\n");
  //     xpn_adaptor_keepInit();
  //     fd = xpn_open(skip_xpn_prefix(path), flags);
  //     ret = add_xpn_file_to_fdstable(fd);
  //     *result = ret;
  //   }
  //   else 
  //   {
  //     *result = syscall_no_intercept(SYS_open, arg0, arg1);
  //   }
  //   return 0;
  // }
  // else if(syscall_number == SYS_read)
  // {
  //   printf("SYS_read\n");
  //   int fd = (int)arg0;
  //   void *buf = (void *)arg1;
  //   size_t nbyte = (size_t)arg2;
  //   ssize_t ret = -1;
  //   struct generic_fd virtual_fd = fdstable_get(fd);
  //   if(virtual_fd.type == FD_XPN)
  //   {
  //     xpn_adaptor_keepInit ();
  //     if (virtual_fd.is_file == 0)
  //     {
  //       errno = EISDIR;
  //       return -1;
  //     }
  //     ret = xpn_read(virtual_fd.real_fd, buf, nbyte);
  //     *result = ret;
  //   }
  //   else
  //   {
  //     *result = syscall_no_intercept(SYS_read, arg0, arg1, arg2);
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
