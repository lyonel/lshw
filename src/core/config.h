#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifdef NONLS
#define _(String) (String)
#define N_(String) String
#define textdomain(Domain)
#define bindtextdomain(Package, Directory)
#else
#include <libintl.h>
#define _(String) gettext (String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)
#endif

#ifndef PACKAGE
#define PACKAGE "lshw"
#endif

#ifndef PREFIX
#define PREFIX "/usr"
#endif

#ifndef SBINDIR
#define SBINDIR PREFIX"/sbin"
#endif

#ifndef DATADIR
#define DATADIR PREFIX"/share/lshw"
#endif

#ifndef MANDIR
#define MANDIR PREFIX"/share/man"
#endif

#ifndef LOCALEDIR
#define LOCALEDIR PREFIX"/share/locale"
#endif
#endif
