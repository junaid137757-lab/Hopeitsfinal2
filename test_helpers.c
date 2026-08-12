#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ftw.h>
#include <sys/stat.h>

#include "test_helpers.h"
#include "common.h"

/* ---- global state reset -------------------------------------- */

void th_reset_globals(void)
{
    transactionCount = 0;
    memset(transactions, 0, sizeof(transactions));

    budgetCount = 0;
    memset(budgets, 0, sizeof(budgets));

    goalCount = 0;
    memset(goals, 0, sizeof(goals));

    memset(currentUser, 0, sizeof(currentUser));
}

/* ---- stdin injection ------------------------------------------
   Redirects file descriptor 0 itself via dup2(), rather than
   reassigning the `stdin` FILE* pointer variable. The pointer-swap
   approach only works if every translation unit re-reads the
   global `stdin`/`stdout` symbol at call time, which isn't
   guaranteed across all glibc/toolchain configurations - it was
   observed to silently fail (input/output escaping to the real
   terminal instead of being captured) in at least one environment.
   Redirecting the fd itself is immune to that: the FILE* objects
   `stdin`/`stdout` never change identity, only what fd 0/1 point
   to at the OS level, so fgets()/scanf()/printf() everywhere keep
   working exactly as before, just against a different destination. */

static int th_stdin_saved_fd = -1;
static int th_stdin_has_saved = 0;

void th_feed_stdin(const char *input)
{
    char path[] = "/tmp/df_test_stdin_XXXXXX";
    int wfd = mkstemp(path);

    if(wfd == -1)
        return;

    size_t len = strlen(input);
    size_t written = 0;

    while(written < len)
    {
        ssize_t n = write(wfd, input + written, len - written);

        if(n <= 0)
            break;

        written += (size_t)n;
    }

    close(wfd);

    int rfd = open(path, O_RDONLY);
    unlink(path);

    if(rfd == -1)
        return;

    if(!th_stdin_has_saved)
    {
        th_stdin_saved_fd = dup(STDIN_FILENO);
        th_stdin_has_saved = 1;
    }

    /* Discard any bytes glibc had already buffered from the
       previous test's input before we swap the underlying fd. */
    fflush(stdin);

    dup2(rfd, STDIN_FILENO);
    close(rfd);

    /* fflush() does not clear a stream's latched EOF/error
       indicator - only clearerr() does. Without this, once any
       earlier test's read reached end-of-file, every fgets() call
       afterward (even against this freshly redirected fd) would
       short-circuit and return NULL immediately instead of
       actually reading the new data. */
    clearerr(stdin);
}

/* ---- stdout capture --------------------------------------------- */

static int th_stdout_saved_fd = -1;
static int th_stdout_capturing = 0;
static char th_stdout_capture_path[64];

void th_capture_stdout_start(void)
{
    fflush(stdout);

    char path[] = "/tmp/df_test_stdout_XXXXXX";
    int fd = mkstemp(path);

    if(fd == -1)
        return;

    strncpy(th_stdout_capture_path, path, sizeof(th_stdout_capture_path) - 1);
    th_stdout_capture_path[sizeof(th_stdout_capture_path) - 1] = '\0';

    th_stdout_saved_fd = dup(STDOUT_FILENO);
    dup2(fd, STDOUT_FILENO);
    close(fd);

    th_stdout_capturing = 1;
}

char *th_capture_stdout_end(void)
{
    if(!th_stdout_capturing)
        return strdup("");

    fflush(stdout);

    dup2(th_stdout_saved_fd, STDOUT_FILENO);
    close(th_stdout_saved_fd);
    th_stdout_saved_fd = -1;
    th_stdout_capturing = 0;

    FILE *fp = fopen(th_stdout_capture_path, "rb");
    char *result = NULL;

    if(fp != NULL)
    {
        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        if(size > 0)
        {
            result = malloc((size_t)size + 1);

            if(result != NULL)
            {
                size_t readBytes = fread(result, 1, (size_t)size, fp);
                result[readBytes] = '\0';
            }
        }

        fclose(fp);
    }

    remove(th_stdout_capture_path);

    return result != NULL ? result : strdup("");
}

char *th_call_capturing(void (*fn)(void))
{
    th_capture_stdout_start();
    fn();
    return th_capture_stdout_end();
}

/* ---- temp working directory ------------------------------------ */

static char th_prev_cwd[4096];
static char th_tmp_dir[4096];

static int th_rm_visitor(const char *path, const struct stat *sb,
                          int typeflag, struct FTW *ftwbuf)
{
    (void)sb; (void)ftwbuf;

    if(typeflag == FTW_DP || typeflag == FTW_D)
        rmdir(path);
    else
        unlink(path);

    return 0;
}

void th_enter_tmp_dir(void)
{
    if(getcwd(th_prev_cwd, sizeof(th_prev_cwd)) == NULL)
        th_prev_cwd[0] = '\0';

    strcpy(th_tmp_dir, "/tmp/df_test_XXXXXX");

    if(mkdtemp(th_tmp_dir) == NULL)
        return;

    if(chdir(th_tmp_dir) != 0)
    {
        /* leave prev_cwd recorded so leave_tmp_dir() is still safe */
    }

    /* persistence.c writes every *.dat file under data/ relative to
       the current directory - that folder needs to exist in each
       fresh scratch dir the same way the Makefile creates it for
       the real app (see the data-dir target), or every save/load
       call in here fails silently. */
    mkdir("data", 0777);
}

void th_leave_tmp_dir(void)
{
    if(th_prev_cwd[0] != '\0')
        chdir(th_prev_cwd);

    if(th_tmp_dir[0] != '\0')
        nftw(th_tmp_dir, th_rm_visitor, 16, FTW_DEPTH | FTW_PHYS);

    th_tmp_dir[0] = '\0';
}
