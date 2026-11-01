// libarchive_ios_globals.c — Thread-local global definitions for iOS exit interception
// and the session-stream replacements declared in libarchive_ios_setup.h.

#include <errno.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "archive.h"
#include "ios_error.h"

__thread jmp_buf libarchive_ios_exit_jmpbuf;
__thread int libarchive_ios_exit_status;

// The core maps a NULL/"" archive name and fd 1 to the process's fds 0 and 1.
// These use the session's streams, which ios_popen backs with fd-less FILEs.

struct ios_stdin_data {
	FILE *f;
	mode_t mode;
	int can_skip;
	off_t size;
	char buffer[64 * 1024];
};

static la_ssize_t
ios_stdin_read(struct archive *a, void *client_data, const void **buff)
{
	struct ios_stdin_data *d = client_data;
	size_t n = fread(d->buffer, 1, sizeof(d->buffer), d->f);

	if (n == 0 && ferror(d->f)) {
		archive_set_error(a, errno, "Error reading stdin");
		return (-1);
	}
	*buff = d->buffer;
	return ((la_ssize_t)n);
}

static la_int64_t
ios_stdin_skip(struct archive *a, void *client_data, la_int64_t request)
{
	struct ios_stdin_data *d = client_data;
	off_t old_offset, new_offset;

	(void)a;
	if (!d->can_skip || request == 0)
		return (0);
	old_offset = ftello(d->f);
	if (old_offset >= 0 && old_offset < d->size &&
	    request <= d->size - old_offset &&
	    fseeko(d->f, (off_t)request, SEEK_CUR) == 0 &&
	    (new_offset = ftello(d->f)) >= 0)
		return (new_offset - old_offset);
	d->can_skip = 0;
	return (0);
}

static la_int64_t
ios_stdin_seek(struct archive *a, void *client_data, la_int64_t offset,
    int whence)
{
	struct ios_stdin_data *d = client_data;

	if (S_ISREG(d->mode) && fseeko(d->f, (off_t)offset, whence) == 0)
		return (ftello(d->f));
	// FAILED, as in the core's stdin path: FATAL poisons the read filter
	// and stops zip falling back to its streaming reader on pipes.
	archive_set_error(a, S_ISREG(d->mode) ? errno : ESPIPE,
	    "Error seeking in stdin");
	return (ARCHIVE_FAILED);
}

static int
ios_stdin_close(struct archive *a, void *client_data)
{
	struct ios_stdin_data *d = client_data;

	(void)a;
	// Drain pipes like the core's stdin path so the writer doesn't get EPIPE.
	if (!S_ISREG(d->mode) && !S_ISCHR(d->mode) && !S_ISBLK(d->mode))
		while (!ios_sessionCancelRequested() &&
		    fread(d->buffer, 1, sizeof(d->buffer), d->f) > 0)
			;
	free(d);
	return (ARCHIVE_OK);
}

int
libarchive_ios_read_open_filename(struct archive *a, const char *filename,
    size_t block_size)
{
	struct ios_stdin_data *d;
	struct stat st;

	if (filename != NULL && filename[0] != '\0')
		return (archive_read_open_filename(a, filename, block_size));

	d = calloc(1, sizeof(*d));
	if (d == NULL) {
		archive_set_error(a, ENOMEM, "No memory");
		return (ARCHIVE_FATAL);
	}
	d->f = thread_stdin;
	if (fstat(fileno(d->f), &st) == 0) {
		d->mode = st.st_mode;
		if (S_ISREG(st.st_mode)) {
			archive_read_extract_set_skip_file(a, st.st_dev, st.st_ino);
			d->can_skip = 1;
			d->size = st.st_size;
		}
	}
	archive_read_set_read_callback(a, ios_stdin_read);
	archive_read_set_skip_callback(a, ios_stdin_skip);
	archive_read_set_seek_callback(a, ios_stdin_seek);
	archive_read_set_close_callback(a, ios_stdin_close);
	if (archive_read_set_callback_data(a, d) != ARCHIVE_OK) {
		free(d);
		return (ARCHIVE_FATAL);
	}
	return (archive_read_open1(a));
}

int
libarchive_ios_write_open_filename(struct archive *a, const char *filename)
{
	struct stat st;

	if (filename != NULL && filename[0] != '\0')
		return (archive_write_open_filename(a, filename));
	// Like the core's fd 1 path: pad the last block, never archive ourselves.
	if (fstat(fileno(thread_stdout), &st) == 0 && S_ISREG(st.st_mode))
		archive_write_set_skip_file(a, st.st_dev, st.st_ino);
	return (archive_write_open_FILE(a, thread_stdout));
}

int
libarchive_ios_read_data_into_fd(struct archive *a, int fd)
{
	char buff[8192];
	la_ssize_t n;
	int out_fd = fileno(thread_stdout);

	if (fd != 1 && fd != out_fd)
		return (archive_read_data_into_fd(a, fd));
	fflush(thread_stdout);
	if (out_fd >= 0)
		return (archive_read_data_into_fd(a, out_fd));
	// fd-less stream: archive_read_data() zero-fills sparse holes itself
	while ((n = archive_read_data(a, buff, sizeof(buff))) > 0)
		if (fwrite(buff, 1, (size_t)n, thread_stdout) != (size_t)n) {
			archive_set_error(a, errno, "Write error");
			return (ARCHIVE_FATAL);
		}
	return (n < 0 ? (int)n : ARCHIVE_OK);
}
