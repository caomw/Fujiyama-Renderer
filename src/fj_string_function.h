/*
Copyright (c) 2011-2014 Hiroshi Tsubokawa
See LICENSE and README
*/

#ifndef FJ_STRINGFUNCTION_H
#define FJ_STRINGFUNCTION_H

#include <stddef.h>

namespace fj {

extern char *StrDup(const char *src);
extern char *StrFree(char *s);

extern char *StrCopyAndTerminate(char *dst, const char *src, size_t nchars);

} // namespace xxx

#endif /* FJ_XXX_H */
