#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

int main()
{
  struct sockaddr sa;
  unsigned int len, i;
  int fd;

  fd = socket(AF_BLUETOOTH, SOCK_STREAM, 3);
  if (fd == -1) {
    printf("[-] can't create socket: %s\n", strerror(errno));
    goto out;
  }

  memset(&sa, 0, sizeof(sa));
  sa.sa_family = AF_BLUETOOTH;

  if ( bind(fd, &sa, 2) ) {
    printf("[-] can't bind socket: %s\n", strerror(errno));
    goto close_out;
  }

  len = sizeof(sa);
  if ( getsockname(fd, &sa, &len) ) {
    printf("[-] can't getsockname for socket: %s\n", strerror(errno));
    goto close_out;
  } else {
    printf("[+] getsockname return len = %d\n", len);
  }

  for (i = 0; i < len; i++)
    printf("%02x ", ((unsigned char*)&sa)[i]);

  printf("\n");

close_out:
  close(fd);
out:
  return 0;
}
