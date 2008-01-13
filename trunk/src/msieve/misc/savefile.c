/*----------------------------------------------------------------------
Copyright 2007, Jason Papadopoulos

This file is part of GGNFS.

GGNFS is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GGNFS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GGNFS; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
----------------------------------------------------------------------*/

#include <common.h>

/* we need a generic interface for reading and writing lines
   of data to the savefile while a factorization is in progress.
   This is necessary for two reasons: first, early msieve 
   versions would sometimes clobber their savefiles, and some
   users have several machines all write to the same savefile
   in a network directory. When output is manually buffered and 
   then explicitly flushed after writing to disk, most of the
   relations in the savefile will survive under these circumstances.

   The other reason is Windows-specific. Microsoft's C runtime 
   library has a bug that causes writes to files more than 4GB in
   size to fail. Thus, to deal with really large savefiles we have
   to call Win32 API functions directly, and these do not have
   any stdio-like stream functionality. Hence we need a homebrew
   implementation of some of stdio.h for the rest of the library
   to use */

#define SAVEFILE_BUF_SIZE 65536

/*--------------------------------------------------------------------*/
void savefile_init(savefile_t *s, char *savefile_name) {
	
	memset(s, 0, sizeof(savefile_t));

	s->name = MSIEVE_DEFAULT_SAVEFILE;
	if (savefile_name)
		s->name = savefile_name;
	
	s->buf = (char *)xmalloc((size_t)SAVEFILE_BUF_SIZE);
}

/*--------------------------------------------------------------------*/
void savefile_free(savefile_t *s) {
	
	free(s->buf);
	memset(s, 0, sizeof(savefile_t));
}

/*--------------------------------------------------------------------*/
void savefile_open(savefile_t *s, uint32 flags) {
	
#if defined(WIN32) || defined(_WIN64)
	DWORD access_arg, open_arg;

	if (flags & SAVEFILE_READ)
		access_arg = GENERIC_READ;
	else
		access_arg = GENERIC_WRITE;

	if (flags & SAVEFILE_READ)
		open_arg = OPEN_EXISTING;
	else if (flags & SAVEFILE_APPEND)
		open_arg = OPEN_ALWAYS;
	else
		open_arg = CREATE_ALWAYS;

	s->file_handle = CreateFile(s->name, 
					access_arg,
					FILE_SHARE_READ |
					FILE_SHARE_WRITE, NULL,
					open_arg,
					FILE_FLAG_SEQUENTIAL_SCAN,
					NULL);

	if (s->file_handle == INVALID_HANDLE_VALUE) {
		printf("error: cannot open '%s'", s->name);
		exit(-1);
	}
	if (flags & SAVEFILE_APPEND) {
		LARGE_INTEGER fileptr;
		fileptr.QuadPart = 0;
		SetFilePointerEx(s->file_handle, 
				fileptr, NULL, FILE_END);
	}
	s->read_size = 0;
	s->eof = 0;

#else
	char *open_string;

	if (flags & SAVEFILE_APPEND)
		open_string = "a+";
	else if ((flags & SAVEFILE_READ) && (flags & SAVEFILE_WRITE))
		open_string = "r+w";
	else if (flags & SAVEFILE_READ)
		open_string = "r";
	else
		open_string = "w";

	s->fp = fopen(s->name, open_string);
	if (s->fp == NULL) {
		printf("error: cannot open '%s'", s->name);
		exit(-1);
	}
#endif

	s->buf_off = 0;
}

/*--------------------------------------------------------------------*/
void savefile_close(savefile_t *s) {
	
#if defined(WIN32) || defined(_WIN64)
	CloseHandle(s->file_handle);
	s->file_handle = INVALID_HANDLE_VALUE;
#else
	fclose(s->fp);
	s->fp = NULL;
#endif
}

/*--------------------------------------------------------------------*/
uint32 savefile_eof(savefile_t *s) {
	
#if defined(WIN32) || defined(_WIN64)
	return (s->buf_off == s->read_size && s->eof);
#else
	return feof(s->fp);
#endif
}

/*--------------------------------------------------------------------*/
uint32 savefile_exists(savefile_t *s) {
	
#if defined(WIN32) || defined(_WIN64)
	struct _stat dummy;
	return (_stat(s->name, &dummy) == 0);
#else
	struct stat dummy;
	return (stat(s->name, &dummy) == 0);
#endif
}

/*--------------------------------------------------------------------*/
void savefile_read_line(char *buf, size_t max_len, savefile_t *s) {

#if defined(WIN32) || defined(_WIN64)
	size_t i, j;
	char *sbuf = s->buf;

	for (i = s->buf_off, j = 0; i < s->read_size && 
				j < max_len - 1; i++, j++) { /* read bytes */
		buf[j] = sbuf[i];
		if (buf[j] == '\n' || buf[j] == '\r') {
			buf[j+1] = 0;
			s->buf_off = i + 1;
			return;
		}
	}
	s->buf_off = i;
	if (i == s->read_size && !s->eof) {	/* sbuf ran out? */
		DWORD num_read;
		ReadFile(s->file_handle, sbuf, 
				SAVEFILE_BUF_SIZE, 
				&num_read, NULL);
		s->read_size = num_read;
		s->buf_off = 0;
		if (num_read < SAVEFILE_BUF_SIZE)
			s->eof = 1;
	}
	for (i = s->buf_off; i < s->read_size && 
				j < max_len - 1; i++, j++) { /* read more */
		buf[j] = sbuf[i];
		if (buf[j] == '\n' || buf[j] == '\r') {
			i++; j++;
			break;
		}
	}
	buf[j] = 0;
	s->buf_off = i;
#else
	fgets(buf, (int)max_len, s->fp);
#endif
}

/*--------------------------------------------------------------------*/
void savefile_write_line(savefile_t *s, char *buf) {

	if (s->buf_off + strlen(buf) + 1 >= SAVEFILE_BUF_SIZE)
		savefile_flush(s);

	s->buf_off += sprintf(s->buf + s->buf_off, "%s", buf);
}

/*--------------------------------------------------------------------*/
void savefile_flush(savefile_t *s) {

#if defined(WIN32) || defined(_WIN64)
	if (s->buf_off) {
		DWORD num_write; /* required because of NULL arg below */
		WriteFile(s->file_handle, s->buf, 
				s->buf_off, &num_write, NULL);
	}
	FlushFileBuffers(s->file_handle);
#else
	fprintf(s->fp, "%s", s->buf);
	fflush(s->fp);
#endif

	s->buf_off = 0;
	s->buf[0] = 0;
}

/*--------------------------------------------------------------------*/
void savefile_rewind(savefile_t *s) {

#if defined(WIN32) || defined(_WIN64)
	LARGE_INTEGER fileptr;
	fileptr.QuadPart = 0;
	SetFilePointerEx(s->file_handle, fileptr, NULL, FILE_BEGIN);
	s->read_size = 0;   /* invalidate buffered data */
	s->buf_off = 0;
	s->eof = 0;
#else
	rewind(s->fp);
#endif
}

