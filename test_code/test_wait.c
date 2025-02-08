#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

  printf("hello world (pid:%d)\n", (int)getpid());
  int rc = fork();
  if (rc < 0) {
    fprintf(stderr, "fork failed\n");
  } else if (rc == 0) {
    printf("hello, I am child (pid:%d)\n", (int)getpid());
    return -1;
  } else {
    printf("hello I am parent of %d (pid: %d)\n", rc, (int)getpid());
    int will;
    int pid = wait(&will);
    printf("child %d exist with will: %d\n", pid, WEXITSTATUS(will));
  }
  return 0;
}
