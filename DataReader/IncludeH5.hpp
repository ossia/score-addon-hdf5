#pragma once

#if defined(_WIN32)
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#if !defined(NOMINMAX)
#define NOMINMAX
#endif
#if !defined(UNICODE)
#define UNICODE 1
#endif
#if !defined(_UNICODE)
#define _UNICODE 1
#endif
#include <windows.h>
#endif

#undef FILE_CREATE

#define HIGHFIVE_LOG_LEVEL HIGHFIVE_LOG_LEVEL_ERROR
#include <highfive/H5Easy.hpp>
