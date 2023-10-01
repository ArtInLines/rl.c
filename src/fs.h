// File-System related functions that work on Unix and Windows

#ifndef FS_H_
#define FS_H_

#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <stdlib.h>    // For malloc
#include <sys/types.h>

#if defined(_WIN32)
	#define mkdir(dir, mode) _mkdir(dir)
	#define open(name, ...) _open(name, __VA_ARGS__)
	#define read(fd, buf, count) _read(fd, buf, count)
	#define close(fd) _close(fd)
	#define write(fd, buf, count) _write(fd, buf, count)
	#define dup2(fd1, fd2) _dup2(fd1, fd2)
	#define unlink(file) _unlink(file)
	#define rmdir(dir) _rmdir(dir)
	#define getpid() _getpid()
	#define usleep(t) Sleep((t)/1000)
	#define sleep(t) Sleep((t)*1000)
#endif

char* readFile(const char *fpath, uint32_t *size);
bool writeFile(const char *fpath, char *buf, uint64_t size);

#endif // FS_H_


#ifdef FS_IMPLEMENTATION

char* readFile(const char *fpath, uint32_t *size)
{
    // Adapted from https://stackoverflow.com/a/68156485/13764271
    char* out = NULL;
    *size = 0;
    int fd = open(fpath, O_RDONLY, 0777);
    if (fd == -1) goto end;
    struct stat sb;
    if (stat(fpath, &sb) == -1) goto fd_end;
    if (sb.st_size == 0) goto fd_end;
    out = malloc(sb.st_size);
    if (out == NULL) goto fd_end;
    if (read(fd, out, sb.st_size) == -1) goto fd_end;
    *size = (uint32_t) sb.st_size;
fd_end:
    close(fd);
end:
    return out;
}

bool writeFile(const char *fpath, char *buf, uint64_t size)
{
    bool out = false;
    int fd = open(fpath, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (fd == -1) goto end;
    uint64_t written = 0;
    while (written < size) {
        int res = write(fd, &buf[written], size - written);
        if (res == -1) goto fd_end;
        written += res;
    }
    out = true;
fd_end:
    close(fd);
end:
    return out;
}

#endif // FS_IMPLEMENTATION