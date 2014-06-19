/*
 * Does the basic fork and wait to make sure that the
 * fork and waitpid system calls are working correctly.
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
        pid_t pid;

        open("/dev/tty0", O_RDONLY, 0);
        open("/dev/tty0", O_WRONLY, 0);

        write(1, "Ready to fork()...\n", 19);

        pid = fork();

        if (pid == 0) {
                write(1, "(Child) Hello, world!\n", 22);
                exit(0);
        } else if (pid == (pid_t)(-1)) {
                write(1, "fork() failed.\n", 15);
        } else {
                write(1, "(Parent) Calling waitpid()...\n", 30);
                waitpid(pid, 0, NULL);
                write(1, "(Parent) waitpid() returned successfully.\n", 42);
        }
        return 0;
}

