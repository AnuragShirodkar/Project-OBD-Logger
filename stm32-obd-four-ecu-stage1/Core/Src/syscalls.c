// No host filesystem or serial stdio is provided by this bench application.
// Explicit stubs avoid newlib's warning sections being reported as IDE errors.
#include <errno.h>
int _close(int fd) { (void)fd; errno=ENOSYS; return -1; }
int _lseek(int fd,int offset,int whence) { (void)fd;(void)offset;(void)whence;errno=ENOSYS;return -1; }
int _read(int fd,char *buffer,int length) { (void)fd;(void)buffer;(void)length;errno=ENOSYS;return -1; }
int _write(int fd,char *buffer,int length) { (void)fd;(void)buffer;(void)length;errno=ENOSYS;return -1; }
