#ifndef BUF_H_
#define BUF_H_

#include "main.h"
#include "util.h"
#include "sv.h"

typedef struct {
	u8 *data;
	u64 idx;
	u64 size;
} Buffer;

Buffer buf_fromFile(const char *filename);
Buffer buf_new(u64 initial_size);
void buf_setcap(Buffer *buf, u64 new_cap);
void buf_free(Buffer *buf);
u8  buf_peek1(Buffer buf);
u16 buf_peek2(Buffer buf);
u32 buf_peek4(Buffer buf);
u64 buf_peek8(Buffer buf);
u8  buf_read1(Buffer *buf);
u16 buf_read2(Buffer *buf);
u32 buf_read4(Buffer *buf);
u64 buf_read8(Buffer *buf);
bool buf_write1(Buffer *buf, u8  elem);
bool buf_write2(Buffer *buf, u16 elem);
bool buf_write4(Buffer *buf, u32 elem);
bool buf_write8(Buffer *buf, u64 elem);
String_View buf_peekSV(Buffer buf);
String_View buf_readSV(Buffer *buf);
bool buf_writeStr(Buffer *buf, char *data, u64 size);
Column buf_peekColumn(Buffer buf);
Column buf_readColumn(Buffer *buf);
bool buf_writeColumn(Buffer *buf, Column elem);

#endif // BUF_H_


#ifdef BUF_IMPLEMENTATION

Buffer buf_fromFile(const char *filename)
{
	Buffer buf = {0};
	buf.data = (u8*) util_readFile(filename, &buf.size);
	return buf;
}

Buffer buf_new(u64 initial_size)
{
	Buffer buf = {
		.data = malloc(initial_size),
		.size = initial_size,
		.idx = 0
	};
	return buf;
}

void buf_setcap(Buffer *buf, u64 new_cap)
{
	if (new_cap > buf->size) {
		u8 *new_data = malloc(new_cap);
		memcpy(new_data, buf->data, buf->size);
		free(buf->data);
		buf->data = new_data;
		buf->size = new_cap;
	}
}

void buf_free(Buffer *buf)
{
	free(buf->data);
	free(buf);
}

u8  buf_peek1(Buffer buf)
{
	return buf.data[buf.idx];
}

u16 buf_peek2(Buffer buf)
{
	return *((u16*)(&buf.data[buf.idx]));
}

u32 buf_peek4(Buffer buf)
{
	return *((u32*)(&buf.data[buf.idx]));
}

u64 buf_peek8(Buffer buf)
{
	return *((u64*)(&buf.data[buf.idx]));
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

bool buf_write1(Buffer *buf, u8  elem)
{
	if (buf->idx == buf->size) return false;
	buf->data[buf->idx++] = elem;
	return true;
}

bool buf_write2(Buffer *buf, u16 elem)
{
	if (buf->idx + sizeof(u16) > buf->size) return false;
	*((u16*)(&buf->data[buf->idx])) = elem;
	return true;
}

bool buf_write4(Buffer *buf, u32 elem)
{
	if (buf->idx + sizeof(u32) > buf->size) return false;
	*((u32*)(&buf->data[buf->idx])) = elem;
	return true;
}

bool buf_write8(Buffer *buf, u64 elem)
{
	if (buf->idx + sizeof(u64) > buf->size) return false;
	*((u64*)(&buf->data[buf->idx])) = elem;
	return true;
}

String_View buf_peekSV(Buffer buf)
{
	u64 size   = *((u64*)(&buf.data[buf.idx]));
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

bool buf_writeStr(Buffer *buf, char *data, u64 size)
{
	if (buf->idx + 8 + size > buf->size) return false;
	buf_write8(buf, size);
	memcpy(&buf->data[buf->idx], data, size);
	return true;
}

Column buf_internal_peekColumn(Buffer buf, u64 *size)
{
	*size = 1;
	Column col = {0};
	col.type = (Datatype) buf.data[buf.idx];

	STATIC_ASSERT(TYPE_LEN == 4);
	switch (col.type)
	{
	case TYPE_STR:
		break;
	case TYPE_SELECT:
	case TYPE_TAG:
		TODO();
	case TYPE_DATE:
		TODO();
	default:
		PANIC("Reading unexpected value '%d' for column type in buffer at index '%lld'", col.type, buf.idx);
	}
	return col;
}

Column buf_peekColumn(Buffer buf)
{
	u64 x;
	return buf_internal_peekColumn(buf, &x);
}

Column buf_readColumn(Buffer *buf)
{
	u64 size;
	Column out = buf_internal_peekColumn(*buf, &size);
	buf->idx += size;
	return out;
}

bool buf_writeColumn(Buffer *buf, Column elem)
{
	(void)buf;
	(void)elem;
	TODO();
}


#endif // BUF_IMPLEMENTATION