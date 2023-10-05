#ifndef BUF_H_
#define BUF_H_

#include "main.h"
#include "util.h"
#include "sv.h"
#include "stb_ds.h"

typedef struct {
	u8 *data;
	u64 idx;
	u64 size;
	u64 cap;
} Buffer;

// @TODO: Add overflow checks when reading/peeking

#define buf_iter_cond(buf) ((buf).idx < (buf).size)

Buffer buf_fromFile(const char *filename);
bool buf_copyToFile(Buffer buf, const char *filename);
bool buf_toFile(Buffer *buf, const char *filename);
Buffer buf_new(u64 initial_cap);
void buf_ensure_size(Buffer *buf, u64 n);
void buf_free(Buffer buf);
u8  buf_read1(Buffer *buf);
u16 buf_read2(Buffer *buf);
u32 buf_read4(Buffer *buf);
u64 buf_read8(Buffer *buf);
i8  buf_read1i(Buffer *buf);
i16 buf_read2i(Buffer *buf);
i32 buf_read4i(Buffer *buf);
i64 buf_read8i(Buffer *buf);
void buf_write1(Buffer *buf, u8  elem);
void buf_write2(Buffer *buf, u16 elem);
void buf_write4(Buffer *buf, u32 elem);
void buf_write8(Buffer *buf, u64 elem);
void buf_write1i(Buffer *buf,i8  elem);
void buf_write2i(Buffer *buf,i16 elem);
void buf_write4i(Buffer *buf,i32 elem);
void buf_write8i(Buffer *buf,i64 elem);
String_View buf_peekSV(Buffer buf);
String_View buf_readSV(Buffer *buf);
void buf_writeStr(Buffer *buf, char *data, u64 size);
Column buf_readColumn(Buffer *buf);
void buf_writeColumn(Buffer *buf, Column elem);

#endif // BUF_H_


#ifdef BUF_IMPLEMENTATION

Buffer buf_fromFile(const char *filename)
{
	Buffer buf = {0};
	buf.data = (u8*) util_readFile(filename, &buf.cap);
	buf.size = buf.cap;
	return buf;
}

inline bool buf_copyToFile(Buffer buf, const char *filename)
{
	return util_writeFile(filename, (char*) buf.data, buf.size);
}

// Unlike copyToFile, this function frees the buffer after writing it to the file
bool buf_toFile(Buffer *buf, const char *filename)
{
	bool out = util_writeFile(filename, (char*)(buf->data), buf->size);
	free(buf->data);
	return out;
}

Buffer buf_new(u64 initial_cap)
{
	Buffer buf = {
		.data = malloc(initial_cap),
		.size = 0,
		.cap  = initial_cap,
		.idx  = 0
	};
	return buf;
}

// Ensures that there's enough capacity to write `n` more bytes into the buffer
void buf_ensure_size(Buffer *buf, u64 n)
{
	u64 min = buf->size + n;
	if (UNLIKELY(min > buf->cap)) {
		u64 new_cap = buf->cap * 2;
		if (UNLIKELY(min > new_cap)) new_cap = min;
		u8 *new_data = malloc(new_cap);
		memcpy(new_data, buf->data, buf->size);
		free(buf->data);
		buf->data = new_data;
		buf->cap = new_cap;
	}
}

inline void buf_free(Buffer buf)
{
	free(buf.data);
}

u8  buf_read1(Buffer *buf)
{
	return buf->data[buf->idx++];
}

u16 buf_read2(Buffer *buf)
{
	u16 out  =  *((u16*)(&buf->data[buf->idx]));
	buf->idx += sizeof(u16);
	return out;
}

u32 buf_read4(Buffer *buf)
{
	u32 out  =  *((u32*)(&buf->data[buf->idx]));
	buf->idx += sizeof(u32);
	return out;
}

u64 buf_read8(Buffer *buf)
{
	u64 out  =  *((u64*)(&buf->data[buf->idx]));
	buf->idx += sizeof(u64);
	return out;
}

i8  buf_read1i(Buffer *buf)
{
	return ((i8*)buf->data)[buf->idx++];
}

i16 buf_read2i(Buffer *buf)
{
	i16 out  =  *((i16*)(&buf->data[buf->idx]));
	buf->idx += sizeof(i16);
	return out;
}

i32 buf_read4i(Buffer *buf)
{
	i32 out  =  *((i32*)(&buf->data[buf->idx]));
	buf->idx += sizeof(i32);
	return out;
}

i64 buf_read8i(Buffer *buf)
{
	i64 out  =  *((i64*)(&buf->data[buf->idx]));
	buf->idx += sizeof(i64);
	return out;
}

void buf_write1(Buffer *buf, u8  elem)
{
	buf_ensure_size(buf, 1);
	buf->data[buf->idx++] = elem;
	if (LIKELY(buf->idx > buf->size)) buf->size = buf->idx;
}

void buf_write2(Buffer *buf, u16 elem)
{
	buf_ensure_size(buf, 2);
	*((u16*)(&buf->data[buf->idx])) = elem;
	buf->idx += 2;
	if (LIKELY(buf->idx > buf->size)) buf->size = buf->idx;
}

void buf_write4(Buffer *buf, u32 elem)
{
	buf_ensure_size(buf, 4);
	*((u32*)(&buf->data[buf->idx])) = elem;
	buf->idx += 4;
	if (LIKELY(buf->idx > buf->size)) buf->size = buf->idx;
}

void buf_write8(Buffer *buf, u64 elem)
{
	buf_ensure_size(buf, 8);
	*((u64*)(&buf->data[buf->idx])) = elem;
	buf->idx += 8;
	if (LIKELY(buf->idx > buf->size)) buf->size = buf->idx;
}

void buf_write1i(Buffer *buf, i8  elem)
{
	buf_ensure_size(buf, 1);
	((i8*)buf->data)[buf->idx++] = elem;
	if (LIKELY(buf->idx > buf->size)) buf->size = buf->idx;
}

void buf_write2i(Buffer *buf, i16 elem)
{
	buf_ensure_size(buf, 2);
	*((i16*)(&buf->data[buf->idx])) = (u16) elem;
	buf->idx += 2;
	if (LIKELY(buf->idx > buf->size)) buf->size = buf->idx;
}

void buf_write4i(Buffer *buf, i32 elem)
{
	buf_ensure_size(buf, 4);
	*((i32*)(&buf->data[buf->idx])) = elem;
	buf->idx += 4;
	if (LIKELY(buf->idx > buf->size)) buf->size = buf->idx;
}

void buf_write8i(Buffer *buf, i64 elem)
{
	buf_ensure_size(buf, 8);
	*((i64*)(&buf->data[buf->idx])) = elem;
	buf->idx += 8;
	if (LIKELY(buf->idx > buf->size)) buf->size = buf->idx;
}

String_View buf_peekSV(Buffer buf)
{
	u64   size = *((u64*)(&buf.data[buf.idx]));
	char *data = malloc(size + 1);
	memcpy(data, &buf.data[buf.idx + sizeof(u64)], size);
	data[size] = 0;
	return sv_from_parts(data, size);
}

String_View buf_readSV(Buffer *buf)
{
	String_View out = buf_peekSV(*buf);
	buf->idx += sizeof(u64) + out.count;
	return out;
}

void buf_writeStr(Buffer *buf, char *data, u64 size)
{
	buf_ensure_size(buf, size + 8);
	buf_write8(buf, size);
	memcpy(&buf->data[buf->idx], data, size);
	buf->idx += size;
	if (LIKELY(buf->idx > buf->size)) buf->size = buf->idx;
}

Column buf_readColumn(Buffer *buf)
{
	Column col = {0};
	col.type = buf_read1(buf);
	col.name = buf_readSV(buf);

	STATIC_ASSERT(TYPE_LEN == 4);
	switch (col.type)
	{
	case TYPE_SELECT:
	case TYPE_TAG:
		{
		i32 amount = buf_read4i(buf);
		stbds_arrsetcap(col.opts.strs, amount);
		for (i32 i = 0; i < amount; i++) {
			col.opts.strs[i] = buf_readSV(buf);
		}
		}
		break;
	case TYPE_STR:
	case TYPE_DATE:
		break;
	default:
		PANIC("Reading unexpected value '%d' for column type in buffer at index '%lld'", col.type, buf->idx-1);
	}
	return col;
}

void buf_writeColumn(Buffer *buf, Column elem)
{
	buf_write1(buf, elem.type);
	buf_writeStr(buf, elem.name.data, elem.name.count);
	STATIC_ASSERT(TYPE_LEN == 4);
	switch (elem.type)
	{
	case TYPE_TAG:
	case TYPE_SELECT:
		{
		String_View *opts = elem.opts.strs;
		i32 len = stbds_arrlen(opts);
		buf_write4i(buf, len);
		for (i32 i = 0; i < len; i++) {
			String_View sv = opts[i];
			buf_writeStr(buf, sv.data, sv.count);
		}
		}
		break;
	default:
		break;
	}
	// Only necessary when writing to buffer without using write() functions, as they already update the buffer's size
	// if (LIKELY(buf->idx > buf->size)) buf->size = buf->idx;
}


#endif // BUF_IMPLEMENTATION