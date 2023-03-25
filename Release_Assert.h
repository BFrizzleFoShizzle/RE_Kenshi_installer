#ifndef RELEASE_ASSERT_H
#define RELEASE_ASSERT_H

#ifdef NDEBUG
#undef NDEBUG
#include <assert.h>
#define NDEBUG
#else
#include <assert.h>
#endif

#endif // RELEASE_ASSERT_H
