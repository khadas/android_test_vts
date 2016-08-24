#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <asm/ioctl.h>

#define SIZE 32

void test_write(int fd)
{
  int ret;
  char buf[SIZE] = {0};

  sprintf(buf, "%x %x", 0x1111111, 0x2222222);
  ret = write(fd, buf, SIZE);
  if (ret < 0) {
    perror("write fail");
  } else {
    printf("succ write %d byte\n", ret);
  }
}

int main(int argc, char* argv[])
{
  int ret;
  char* path =
      "/sys/kernel/debug/asoc/msm8994-tomtom-snd-card/snd-soc-dummy/codec_reg";
  int fd;

  fd = open(path, O_RDWR);
  if (fd < 0) {
    perror("open fail");
    return -1;
  }
  printf("open %s succ\n", path);
  test_write(fd);
  close(fd);

  return 0;
}
