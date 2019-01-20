#ifndef __PRETTY_H
#define __PRETTY_H

void __pp_pcall(int, const char *, const char *, const char *, int);
void __pp_errno(int, const char *, const char *, const char *, int);
void __pp_warn(const char *, const char *, const char *, int);

#define pp_pcall(errnum, msg) \
    __pp_pcall(errnum, msg, __FILE__, __func__, __LINE__)

#define pp_errno(errnum, msg) \
    __pp_errno(errnum, msg, __FILE__, __func__, __LINE__)

#define pp_warn(msg) \
    __pp_warn(msg, __FILE__, __func__, __LINE__)

#endif
