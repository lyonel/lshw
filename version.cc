#include "version.h"
#include <string.h>
#include <ctype.h>

#define RELEASE_SIZE 80

static char rcstag[] = "@(#) $Name:  $ >";

const char *
getpackageversion()
{
  static char releasename[RELEASE_SIZE];
  char *i;

  strncat(releasename, "", RELEASE_SIZE);
  i = strchr(rcstag, ':');
  if (!i)
    i = rcstag;

  for (; i && (*i) && (i < rcstag + strlen(rcstag)); i++)
  {
    if (*i == '_')
      strcat(releasename, ".");
    else if (isalnum(*i))
      strncat(releasename, i, 1);
  }

  if (strlen(releasename) == 0)
    strncat(releasename, "$unreleased$", RELEASE_SIZE);

  return releasename;
}
