/* -*- mode: c; c-file-style: "bsd" -*-
 *
 * $Id$
 *
 * private.h -- Private headers for libhsync
 *
   Copyright (C) 2000 by Martin Pool <mbp@humbug.org.au>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA
*/




#ifdef DO_HS_TRACE

void _hs_trace(char const *fmt, ...)
#ifdef __GNUC__
    __attribute__ ((format(printf, 1, 2)))
#endif
;

#else /* !DO_HS_TRACE */
#define _hs_trace(s, str...)
#endif /* !DO_HS_TRACE */


#define return_val_if_fail(expr, val) if (!(expr))	\
  { fprintf(stderr, "%s(%d): %s: assertion failed\n",	\
    __FILE__, __LINE__, __FUNCTION__); return (val); }

#define _hs_fatal(s, str...) do { fprintf (stderr,	\
  "libhsync: " __FUNCTION__ ": "			\
  s "\n" , ##str); abort(); } while(0)

#define _hs_error(s, str...) {				\
     fprintf(stderr,					\
	     "libhsync: " __FUNCTION__ ": " s "\n" , ##str);	\
     } while (0)


/* ========================================

   Nice macros */

#undef	MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

#undef	MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

#undef	ABS
#define ABS(a)	   (((a) < 0) ? -(a) : (a))

#undef	CLAMP
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))


#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif				/* ! __GNUC__ */



/* ========================================

   Net IO functions */

int _hs_read_loop(hs_read_fn_t, void *readprivate,
		  char *buf, size_t len);

int _hs_write_loop(hs_write_fn_t, void *writeprivate,
		   char const *buf, int len);

int hs_must_write(hs_write_fn_t write_fn, void *write_priv,
		  void const *buf, int len);

int _hs_must_read(hs_read_fn_t, void *, char *, ssize_t);

int _hs_read_netint(hs_read_fn_t read_fn, void *read_priv,
		    uint32_t * result);

int _hs_read_netshort(hs_read_fn_t read_fn, void *read_priv,
		      uint16_t * result);


int _hs_read_netbyte(hs_read_fn_t read_fn, void *read_priv,
		     uint8_t * result);

int _hs_write_netint(hs_write_fn_t write_fn, void *write_priv,
		     uint32_t out);

int _hs_write_netshort(hs_write_fn_t write_fn, void *write_priv,
		       uint16_t out);

int _hs_write_netbyte(hs_write_fn_t write_fn, void *write_priv,
		      uint8_t out);

int _hs_write_netvar(hs_write_fn_t write_fn, void *write_priv,
		     uint32_t value, int type);

/* ========================================

   Literal output buffer.

   Data queued for output is now held in a MEMBUF IO pipe, and copied
   from there into the real output stream when necessary.  */
ssize_t
_hs_push_literal_buf(hs_membuf_t * litbuf,
		      hs_write_fn_t write_fn, void *write_priv,
		      hs_stats_t * stats,
		      int kind);


void _hs_check_blocksize(int block_len);


/* ========================================

   Memory buffers
*/


/* An HS_MEMBUF grows dynamically.  BUF points to an array of ALLOC
   bytes, of which LENGTH contain data.  The cursor is at position
   OFS.  If BUF is null, then no memory has been allocated yet. */
struct hs_membuf {
    int dogtag;
    char *buf;
    hs_off_t ofs;
    ssize_t length;
    size_t alloc;
};

/* hs_ptrbuf_t: Memory is provided by the caller, and they retain
   responsibility for it.  BUF points to an array of LENGTH bytes.
   The cursor is currently at OFS. */
struct hs_ptrbuf {
    int dogtag;
    char *buf;
    hs_off_t ofs;
    size_t length;
};

/* ========================================

   _hs_inbuf_t: a buffer of new data waiting to be digested.

*/

/*
 * Buffer of new data waiting to be digested and encoded.  This is
 * like a map_ptr, but more suitable for reading from a socket, where
 * we can't seek, and therefore can't skip forwards or rewind.
 * Therefore we must be prepared to give up any amount of memory
 * rather than seek.
 *
 * The inbuf covers a particular part of the file with an in-memory
 * buffer.  The file is addressed by absolute position,
   
   inbuf[0..inbufamount-1] is valid, inbufamount <= inbuflen,
   cursor <= inbufamount is the next one to be processed.
   
   0 <= abspos is the absolute position in the input file of the start
   of the buffer.  We need this to generate new signatures at the
   right positions. */
struct _hs_inbuf {
    int tag;
    int len;
    char *buf;
    int amount;
    int cursor;
    int abspos;
};

typedef struct _hs_inbuf _hs_inbuf_t;

int _hs_fill_inbuf(_hs_inbuf_t *, hs_read_fn_t read_fn, void *readprivate);

_hs_inbuf_t * _hs_new_inbuf(void);
void _hs_free_inbuf(_hs_inbuf_t *);
int _hs_slide_inbuf(_hs_inbuf_t *);

/* ========================================

  Checksums
*/

#define MD4_LENGTH 16
#define SUM_LENGTH 8

/* We should make this something other than zero to improve the
   checksum algorithm: tridge suggests a prime number. */
#define CHAR_OFFSET 31

typedef unsigned short tag;

struct target {
    tag t;
    int i;
};

/* FIXME: Better names for the members of this structure. */
typedef struct sum_struct {
    hs_off_t flength;		/* total file length */
    int count;			/* how many chunks */
    int remainder;		/* flength % block_length */
    int n;			/* block_length */
    struct sum_buf *sums;	/* points to info for each chunk */
    int *tag_table;
    struct target *targets;
} hs_sum_struct_t;


/* All blocks are the same length in the current algorithm except for
   the last block which may be short. */
typedef struct sum_buf {
    int i;			/* index of this chunk */
    uint32_t sum1;		/* simple checksum */
    char strong_sum[SUM_LENGTH];	/* checksum  */
} sum_buf_t;

/* ROLLSUM_T contains the checksums that roll through the new version
   of the file as we see it.  We use this for two different things:
   searching for matches in the old version of the file, and also
   generating new-signature information to send down to the client.  */
typedef struct rollsum {
    int havesum;		/* false if we've skipped & need to
				   recalculate */
    uint32_t weak_sum, s1, s2;	/* weak checksum */
} rollsum_t;

uint32_t _hs_calc_weak_sum(char const *buf1, int len);
uint32_t _hs_calc_strong_sum(char const *buf, int len, char *sum);


/* ========================================

   Things to do with searching through the hashtable of blocks from
   downstream.  */

int _hs_find_in_hash(rollsum_t * rollsum,
		     char const *inbuf, int block_len,
		     struct sum_struct const *sigs,
		     hs_stats_t *);

int _hs_build_hash_table(struct sum_struct *sums);

hs_sum_struct_t * _hs_make_sum_struct(hs_read_fn_t, void *, int block_len);
void _hs_free_sum_struct(hs_sum_struct_t *);


/* ========================================

   queue of outgoing copy commands
*/

typedef struct _hs_copyq {
     size_t start, len;
} _hs_copyq_t;

int _hs_queue_copy(hs_write_fn_t write_fn, void *write_priv,
		    _hs_copyq_t *copyq, size_t start, size_t len,
		    hs_stats_t *stats);
int _hs_copyq_push(hs_write_fn_t write_fn, void *write_priv,
		    _hs_copyq_t *copyq,
		    hs_stats_t *stats);


/* ========================================

   emit/inhale commands
*/

struct hs_op_kind_name {
     char *name;
     int code;
};

extern struct hs_op_kind_name const _hs_op_kind_names[];

int _hs_emit_signature_cmd(hs_write_fn_t write_fn, void *write_priv,
		       uint32_t size);

int _hs_emit_filesum(hs_write_fn_t write_fn, void *write_priv,
		     char const *buf, uint32_t size);

int _hs_emit_literal_cmd(hs_write_fn_t write_fn, void *write_priv,
		       uint32_t size);

int _hs_emit_checksum_cmd(hs_write_fn_t, void *, uint32_t size);

int _hs_emit_copy(hs_write_fn_t write_fn, void *write_priv,
		  uint32_t offset, uint32_t length, hs_stats_t * stats);


int _hs_emit_eof(hs_write_fn_t write_fn, void *write_priv,
		 hs_stats_t *stats);

int _hs_append_literal(hs_membuf_t * litbuf, char value);


int _hs_inhale_command(hs_read_fn_t read_fn, void * read_priv,
		       int *kind, uint32_t *len, uint32_t *off);



/* ========================================

   map_ptr IO
*/

typedef struct hs_map hs_map_t;

hs_map_t *_hs_map_file(int fd);
char const * _hs_map_ptr(hs_map_t *, hs_off_t, int *len, int *reached_eof);
void _hs_unmap_file(hs_map_t *map);

int hs_file_open(char const *filename, int mode);
