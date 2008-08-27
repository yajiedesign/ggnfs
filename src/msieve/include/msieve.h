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

#ifndef _MSIEVE_H_
#define _MSIEVE_H_

#ifdef __cplusplus
extern "C" {
#endif

	/* Lightweight factoring API */

#include <util.h>
#include <mp.h>

/* version info */

#define MSIEVE_MAJOR_VERSION 1
#define MSIEVE_MINOR_VERSION 37

/* The final output from the factorization is a linked
   list of msieve_factor structures, one for each factor
   found. */

enum msieve_factor_type {
	MSIEVE_COMPOSITE,
	MSIEVE_PRIME,
	MSIEVE_PROBABLE_PRIME
};

typedef struct msieve_factor {
	enum msieve_factor_type factor_type;
	char *number;
	struct msieve_factor *next;
} msieve_factor;

/* These flags are used to 'communicate' status and
   configuration info back and forth to a factorization
   in progress */

enum msieve_flags {
	MSIEVE_DEFAULT_FLAGS = 0,		/* just a placeholder */
	MSIEVE_FLAG_USE_LOGFILE = 0x01,	    /* append log info to a logfile */
	MSIEVE_FLAG_LOG_TO_STDOUT = 0x02,   /* print log info to the screen */
	MSIEVE_FLAG_STOP_SIEVING = 0x04,    /* tell library to stop sieving
					       when it is safe to do so */
	MSIEVE_FLAG_FACTORIZATION_DONE = 0x08,  /* set by the library if a
						   factorization completed */
	MSIEVE_FLAG_SIEVING_IN_PROGRESS = 0x10, /* set by the library when 
						   any sieving operations are 
						   in progress */
	MSIEVE_FLAG_SKIP_QS_CYCLES = 0x20,  /* do not perform exact tracking of
	                                    the number of cycles while sieving
					    is in progress; for distributed
					    sieving where exact progress info
					    is not needed, sieving clients can
					    save a lot of memory with this */
	MSIEVE_FLAG_NFS_POLY = 0x40,     /* if input is large enough, perform
	                                    polynomial selection for NFS */
	MSIEVE_FLAG_NFS_SIEVE = 0x80,    /* if input is large enough, perform
	                                    sieving for NFS */
	MSIEVE_FLAG_NFS_FILTER = 0x100,  /* if input is large enough, perform
	                                    filtering phase for NFS */
	MSIEVE_FLAG_NFS_LA = 0x200,      /* if input is large enough, perform
	                                    linear algebra phase for NFS */
	MSIEVE_FLAG_NFS_SQRT = 0x400,    /* if input is large enough, perform
	                                    square root phase for NFS */
	MSIEVE_FLAG_NFS_LA_RESTART = 0x800,/* restart the NFS linear algbra */
	MSIEVE_FLAG_DEEP_ECM = 0x1000    /* perform nontrivial-size ECM */
};
	
/* structure encapsulating the savefile used in a factorization */

typedef struct {

#if defined(WIN32) || defined(_WIN64)
	HANDLE file_handle;
	uint32 read_size;
	uint32 eof;
#else
	FILE *fp;
#endif
	char *name;
	char *buf;
	uint32 buf_off;
} savefile_t;

/* One factorization is represented by a msieve_obj
   structure. This contains all the static information
   that gets passed from one stage of the factorization
   to another. If this was C++ it would be a simple object */

typedef struct {
	char *input;		  /* pointer to string version of the 
				     integer to be factored */
	msieve_factor *factors;   /* linked list of factors found (in
				     ascending order */
	volatile uint32 flags;	  /* input/output flags */
	savefile_t savefile;      /* data for savefile */
	char *logfile_name;       /* name of the logfile that will be
				     used for this factorization */
	uint32 seed1, seed2;      /* current state of random number generator
				     (updated as random numbers are created) */
	char *nfs_fbfile_name;    /* name of factor base file */
	time_t timestamp;          /* number of seconds factorization took */
	uint32 max_relations;      /* the number of relations that the sieving
	                              stage will try to find. The default (0)
				      is to keep sieving until all necessary 
				      relations are found. */
	uint32 nfs_lower;         /* lower bound for NFS-related thing to do */
	uint32 nfs_upper;         /* upper bound for NFS-related thing to do */

	uint32 cache_size1;       /* bytes in level 1 cache */
	uint32 cache_size2;       /* bytes in level 2 cache */
	enum cpu_type cpu;
	uint32 num_threads;

	char mp_sprintf_buf[32 * MAX_MP_WORDS+1]; /* scratch space for 
						printing big integers */
} msieve_obj;

msieve_obj * msieve_obj_new(char *input_integer,
			    uint32 flags,
			    char *savefile_name,
			    char *logfile_name,
			    char *nfs_fbfile_name,
			    uint32 seed1,
			    uint32 seed2,
			    uint32 max_relations,
			    uint32 nfs_lower,
			    uint32 nfs_upper,
			    enum cpu_type cpu,
			    uint32 cache_size1,
			    uint32 cache_size2,
			    uint32 num_threads);

msieve_obj * msieve_obj_free(msieve_obj *obj);

void msieve_run(msieve_obj *obj);
				
#define MSIEVE_DEFAULT_LOGFILE "msieve.log"
#define MSIEVE_DEFAULT_SAVEFILE "msieve.dat"
#define MSIEVE_DEFAULT_NFS_FBFILE "msieve.fb"

#ifdef __cplusplus
}
#endif

#endif /* _MSIEVE_H_ */
