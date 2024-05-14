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

int xpn_adaptor_initCalled = 0;
int xpn_adaptor_initCalled_getenv = 0; 

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
  long old_size = fdsdirtable_size;
  DIR ** fdsdirtable_aux = fdsdirtable;
  
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

  if(NULL == fdsdirtable)
  {
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

}
int fdstable_put(struct generic_fd fd) 
{
  for(int i = fdstable_first_free; i < fdstable_size; ++i)
  {
    if(fdstable[i].type == FD_FREE) 
    {
      fdstable[i] = fd;
      fdstable_first_free = (long)(i + 1);
      return i + PLUSXPN;
    }
  }

  long old_size = fdstable_size;

  fdstable_realloc();

  if(fdstable[old_size].type == FD_FREE) 
  {
    fdstable[old_size] = fd;
    return old_size + PLUSXPN;
  }

  return -1;
}

int add_xpn_file_to_fdstable(int fd) 
{
  struct stat st; 
  struct generic_fd virtual_fd; 
  
  int ret = fd; 
  if (fd < 0) 
  {
    return ret;
  } 

  xpn_fstat(fd, &st); 

  virtual_fd.type    = FD_XPN;
  virtual_fd.real_fd = fd;
  virtual_fd.is_file = (S_ISDIR(st.st_mode)) ? 0 : 1;
  ret = fdstable_put ( virtual_fd );
  return ret;
}

void fdsdirtable_init(void)
{
  fdsdirtable_realloc();
}

void fdstable_realloc(void) 
{
  long old_size = fdstable_size;
  struct generic_fd * fdstable_aux = fdstable;

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

}

void fdstable_init ( void ) 
{
  fdstable_realloc();
}

int xpn_adaptor_keepInit ( void )
{
  int    ret;
  char * xpn_adaptor_initCalled_env = NULL;
  
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

    fdstable_init ();
    fdsdirtable_init ();
    ret = xpn_init();

    if (ret < 0)
    {
      xpn_adaptor_initCalled = 0;
      setenv("INITCALLED", "0", 1);
    }
    else
    {
      xpn_adaptor_initCalled = 1;
      setenv("INITCALLED", "1", 1);
    }
  }
  return ret;
}

int is_xpn_prefix(const char * path) 
{
  printf("xpn_prefix_change_verified: %d\n",xpn_prefix_change_verified);
  if (0 == xpn_prefix_change_verified)
  {
    xpn_prefix_change_verified = 1;
    char * env_prefix = getenv("XPN_MOUNT_POINT");
    printf("%s\n",env_prefix);
    if (env_prefix != NULL)
    {
      xpn_adaptor_partition_prefix = env_prefix;
    }
  }
  
  const char *prefix = (const char *)xpn_adaptor_partition_prefix;
  printf("prefix: %s\n",prefix);
  prinft("strncmp(prefix, path, strlen(prefix)): %d\n",strncmp(prefix, path, strlen(prefix)));
  printf("strlen(path) %ld strlen(prefix): %d\n",strlen(path),strlen(prefix));
  return ( !strncmp(prefix, path, strlen(prefix)) && strlen(path) > strlen(prefix));
}

const char * skip_xpn_prefix ( const char * path ) 
{
  return (const char *)(path + strlen(xpn_adaptor_partition_prefix));
}

struct generic_fd fdstable_get ( int fd ) 
{
  struct generic_fd ret;
  
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
  return ret;
}

int fdstable_remove ( int fd )
{
  if (fd < PLUSXPN) {
    return 0;
  }

  fd = fd - PLUSXPN;
  fdstable[fd].type    = FD_FREE;
  fdstable[fd].real_fd = -1;
  fdstable[fd].is_file = -1;

  if (fd < fdstable_first_free){
    fdstable_first_free = fd;
  }
  return 0;
}

