
#ifndef DEFINE_H
#define DEFINE_H

#if defined(_WIN32) || defined(WIN32)
# define EDP_WINDOWS
#else
# define EDP_LINUX
#endif

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>

#ifdef EDP_WINDOWS
# include <windows.h>
# include "win32_stdint.h"
#else
# include <stdint.h>
# include <stdatomic.h>
# include <errno.h>
# include <inttypes.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <dirent.h>
# include <semaphore.h>
# include <stdatomic.h>
# include <pthread.h>
#endif

#include "enum_err.h"
#include "enum_ringbuf.h"

#ifdef EDP_WINDOWS
# define EDP_API __declspec(dllexport)
#else
# define EDP_API extern
#endif

typedef uint8_t byte;
typedef int8_t bool;

#define true 1
#define false 0

#define sizefield(type, name) sizeof(((type*)0)->name)

#endif/*DEFINE_H*/
