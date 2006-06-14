/*
 * version.cc
 *
 * The sole purpose of this module is to report package version information:
 * keyword expansion should be allowed for the 'URL' tag (svn ps svn:keywords URL)
 * Reported version is computed using this file's location in the source archive;
 * development versions return 'development' whereas released versions return
 * version numbers like A.01.04
 *
 */
#include "version.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static char versiontag[] = "@(#) $URL$ >";

const char *getpackageversion()
{
  char * releasename = NULL;
  char * lastslash = NULL;

  releasename = strdup(versiontag);

  lastslash = strrchr(releasename, '/');
  if(lastslash)
  {
    *lastslash = '\0';	// cut the basename off

    lastslash = strrchr(releasename, '/');
  }

  lastslash = strrchr(releasename, '/');
  if(lastslash)
  {
    *lastslash = '\0';  // cut the basename off
                                                                                
    lastslash = strrchr(releasename, '/');
  }

  lastslash = strrchr(releasename, '/');
  if(lastslash)
  {
    *lastslash = '\0';  // cut the basename off
                                                                                
    lastslash = strrchr(releasename, '/');
  }

  if(lastslash)
  {
    free(releasename);
    return lastslash+1;
  }
  else
  {
    free(releasename);
    return "unknown";
  }
}
