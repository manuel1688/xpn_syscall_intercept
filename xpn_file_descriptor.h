#ifndef XPN_FILE_DESCRIPTOR_H
#define XPN_FILE_DESCRIPTOR_H

// Declaración de la función fdsdirtable_realloc
void fdsdirtable_realloc(void);
int fdstable_put(struct generic_fd fd);
int add_xpn_file_to_fdstable(int fd);
void fdsdirtable_init(void);
void fdstable_realloc(void);
void fdstable_init ( void );
int xpn_adaptor_keepInit ( void );
int is_xpn_prefix(const char * path);
const char * skip_xpn_prefix ( const char * path );
struct generic_fd fdstable_get ( int fd );
int fdstable_remove ( int fd );

#endif // XPN_FILE_DESCRIPTOR_H