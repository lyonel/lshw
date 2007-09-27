#ifndef _VERSION_H_
#define _VERSION_H_

#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#define __ID(string) __asm__(".ident\t\"" string "\"")
#else
#define __ID(string) static const char rcsid[] __unused = string
#endif

#ifdef __cplusplus
extern "C" {
#endif

const char * getpackageversion();

const char * checkupdates();

#ifdef __cplusplus
}
#endif

#endif
