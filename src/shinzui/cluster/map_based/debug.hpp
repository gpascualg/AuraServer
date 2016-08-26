#pragma once

#define LOG_NOTHING     0x0000000000000000
#define LOG_ALL         0xFFFFFFFFFFFFFFFF
#define LOG_DEBUG       0x0000000000000001
#define LOG_CELLS       0x0000000000000002
#define LOG_CLUSTERS    0x0000000000000004

#define LOG_LEVEL       LOG_ALL


// Expand is a trick for MSVC __VA_ARGS__ to work :(
#define EXPAND(x)				x
#define LOG_HELPER(fmt, ...)    EXPAND(printf(fmt "\n%s", __VA_ARGS__))
#define LOG_ALWAYS(...)         EXPAND(LOG_HELPER(__VA_ARGS__, ""))

#ifndef NDEBUG
    #define LOG(lvl, ...)       ((lvl & LOG_LEVEL) && EXPAND(LOG_HELPER(__VA_ARGS__, "")))
#else
    #define LOG(lvl, ...)
#endif
