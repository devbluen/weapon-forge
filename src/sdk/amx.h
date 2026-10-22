// Minimal, ABI-compatible subset of the Pawn AMX definitions used by the
// SA-MP plugin interface (also implemented by open.mp's legacy plugin loader).
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

typedef int32_t cell;
typedef uint32_t ucell;

struct tagAMX;
typedef tagAMX AMX;

typedef cell (*AMX_NATIVE)(AMX *amx, cell *params);
typedef int (*AMX_CALLBACK)(AMX *amx, cell index, cell *result, cell *params);
typedef int (*AMX_DEBUG)(AMX *amx);

#define AMX_USERNUM 4

#pragma pack(push, 1)

struct AMX_NATIVE_INFO
{
	const char *name;
	AMX_NATIVE func;
};

struct AMX_FUNCSTUBNT
{
	ucell address;
	uint32_t nameofs;
};

struct AMX_HEADER
{
	int32_t size;
	uint16_t magic;
	char file_version;
	char amx_version;
	int16_t flags;
	int16_t defsize;
	int32_t cod;
	int32_t dat;
	int32_t hea;
	int32_t stp;
	int32_t cip;
	int32_t publics;
	int32_t natives;
	int32_t libraries;
	int32_t pubvars;
	int32_t tags;
	int32_t nametable;
};

struct tagAMX
{
	unsigned char *base;
	unsigned char *data;
	AMX_CALLBACK callback;
	AMX_DEBUG debug;
	cell cip;
	cell frm;
	cell hea;
	cell hlw;
	cell stk;
	cell stp;
	int flags;
	long usertags[AMX_USERNUM];
	void *userdata[AMX_USERNUM];
	int error;
	int paramcount;
	cell pri;
	cell alt;
	cell reset_stk;
	cell reset_hea;
	cell sysreq_d;
};

#pragma pack(pop)

enum
{
	AMX_ERR_NONE = 0,
	AMX_ERR_NATIVE = 10,
	AMX_ERR_NOTFOUND = 19,
};

#define AMX_EXEC_MAIN (-1)

inline cell amx_ftoc(float f)
{
	cell c;
	std::memcpy(&c, &f, sizeof(c));
	return c;
}

inline float amx_ctof(cell c)
{
	float f;
	std::memcpy(&f, &c, sizeof(f));
	return f;
}
