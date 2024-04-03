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

// Original --> xpn:// 
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

// esta funcion se encarga de insertar un descriptor de fichero 
//en la tabla de descripto res de ficheros
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
// esta funcion se encarga de añadir un descriptor de fichero correspondiente 
//a un fichero de XPN en la tabla de descriptores de ficheros
int add_xpn_file_to_fdstable ( int fd ) 
{
  struct stat st; // estructura que almacena la informacion de un fichero
  struct generic_fd virtual_fd; // descriptor de fichero generico
  
  debug_info("[bypass] >> Before add_xpn_file_to_fdstable....\n");
  debug_info("[bypass]    1) fd  => %d\n", fd);

  int ret = fd; // valor de retorno

  // check arguments
  if (fd < 0) 
  {
    debug_info("[bypass]\t add_xpn_file_to_fdstable -> %d\n", ret);
    debug_info("[bypass] << After add_xpn_file_to_fdstable....\n");
    return ret;
  } 

  // fstat(fd...
  xpn_fstat(fd, &st); // obtiene la informacion del fichero correspondiente al descriptor de fichero fd

  // setup virtual_fd
  virtual_fd.type    = FD_XPN;
  virtual_fd.real_fd = fd;
  virtual_fd.is_file = (S_ISDIR(st.st_mode)) ? 0 : 1;

  // insert into fdstable
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

void fdstable_init ( void ) // esta funcion se encarga de inicializar la tabla de descriptores de ficheros
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

  // If expand has not been initialized, then initialize it.
  if (0 == xpn_adaptor_initCalled)
  {
    xpn_adaptor_initCalled = 1; //TODO: Delete
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

// valida si el path contiene el prefijo de XPN 
int is_xpn_prefix   ( const char * path ) 
{
  // Si el prefijo de XPN no ha sido verificado, entonces se verifica
  if (0 == xpn_prefix_change_verified)
  {
    xpn_prefix_change_verified = 1;

    char * env_prefix = getenv("XPN_MOUNT_POINT");
    if (env_prefix != NULL)
    {
      xpn_adaptor_partition_prefix = env_prefix;
    }
  }
  
  // el xpn_adaptor_partition_prefix se almacena en un puntero de tipo char llama prefix
  const char *prefix = (const char *)xpn_adaptor_partition_prefix;

  // el valor de prefix es /tmp/expand/
  // el valor de path es /tmp/expand/P1/demo.txt
  // se compara hasta el tamaño de prefix que es 12
  return ( !strncmp(prefix, path, strlen(prefix)) && strlen(path) > strlen(prefix) );
}

/*
  Esta funcion se encarga de saltar el prefijo de XPN con el fin de obtener 
  el path real para el sistema de archivos que se esta utilizando
*/
const char * skip_xpn_prefix ( const char * path ) 
{
  /*
    El valor de path es un puntero de tipo char que almacena /tmp/expand/P1/demo.txt
    luego se le suma el tamaño de xpn_adaptor_partition_prefix que es 12
    como resultado es un puntero de tipo char que almacena P1/demo.txt
  */
  return (const char *)(path + strlen(xpn_adaptor_partition_prefix));
}

struct generic_fd fdstable_get ( int fd ) // esta funcion se encarga de obtener el descriptor de fichero correspondiente a un descriptor de fichero dado
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

static int hook(long syscall_number,long arg0, long arg1,long arg2, long arg3,long arg4, long arg5,long *result){
  
  (void) arg2;
  (void) arg3;
  (void) arg4;
  (void) arg5;
  int fd,ret;

  if (syscall_number == SYS_creat){

    /*
      el path es /tmp/expand/P1/demo.txt
      luego se almacena en un puntero de tipo char
    */
    char *path = (char *)arg0;
    mode_t mode = (mode_t)arg1; 
    
    debug_info("[bypass] >> Before creat....\n");
    if (is_xpn_prefix(path))
    {
        // We must initialize expand if it has not been initialized yet.
        xpn_adaptor_keepInit ();
        // It is an XPN partition, so we redirect the syscall to expand syscall
        debug_info("[bypass]\t try to creat %s", skip_xpn_prefix(path));

        fd  = xpn_creat((const char *)skip_xpn_prefix(path),mode);
        ret = add_xpn_file_to_fdstable(fd);
        printf("ret: %d\n", ret);
        debug_info("[bypass]\t creat %s -> %d", skip_xpn_prefix(path), ret);
        printf("fd: %d\n", fd);
        *result = ret;
        return 0;
    }
    else
    {
      // Not an XPN partition. We must link with the standard library
      debug_info("[bypass]\t try to dlsym_creat %s\n", path);
      // ret = dlsym_creat(path, mode);
      debug_info("[bypass]\t dlsym_creat %s -> %d\n", path, ret);
      *result = syscall_no_intercept(SYS_creat, arg0, arg1);
      return 0;
    }
    debug_info("[bypass] << After creat....\n");
    return ret;
  }
  else if(syscall_number == SYS_write)
  {
    //ret = write(fd1, buffer, BUFF_SIZE);
    ssize_t ret = -1;
    int fd = (int)arg0;
    const void *buf = (const void *)arg1;
    size_t nbyte = (size_t)arg2;
    
    struct generic_fd virtual_fd = fdstable_get(fd);

    // This if checks if variable fd passed as argument is a expand fd.
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
      return 0;
    }
    // Not an XPN partition. We must link with the standard library
    else
    {
      *result = syscall_no_intercept(SYS_write, arg0, arg1, arg2);
      return 0;
    }
    return ret;
  } 
  
  return 1;
}

static __attribute__((constructor)) void
init(void)
{
  intercept_hook_point = hook;
}
