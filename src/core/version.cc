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
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef REMOTE_VERSION_CHECK
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <sys/types.h>

#ifndef PACKETSZ
#define PACKETSZ 512
#endif
#endif

const char *getpackageversion()
{
  if (VERSION)
    return VERSION;

  return "unknown";
}

#ifdef REMOTE_VERSION_CHECK
static char *txtquery(const char *name, const char *domain, unsigned int *ttl)
{
  unsigned char answer[PACKETSZ], *pt;
  char host[128], *txt;
  int len, exp, cttl, size, txtlen, type;

  if(res_init() < 0)
    return NULL;

  memset(answer, 0, PACKETSZ);
  if((len = res_querydomain(name, domain, C_IN, T_TXT, answer, PACKETSZ)) < 0)
    return NULL;

  pt = answer + sizeof(HEADER);

  if((exp = dn_expand(answer, answer + len, pt, host, sizeof(host))) < 0)
    return NULL;

  pt += exp;

  GETSHORT(type, pt);
  if(type != T_TXT)
    return NULL;

  pt += INT16SZ; /* class */

  if((exp = dn_expand(answer, answer + len, pt, host, sizeof(host))) < 0)
    return NULL;

  pt += exp;
  GETSHORT(type, pt);
  if(type != T_TXT)
    return NULL;

  pt += INT16SZ; /* class */
  GETLONG(cttl, pt);
  if(ttl)
    *ttl = cttl;
  GETSHORT(size, pt);
  txtlen = *pt;

  if(txtlen >= size || !txtlen)
    return NULL;

  if(!(txt = (char*)malloc(txtlen + 1)))
    return NULL;

  pt++;
  strncpy(txt, (char*)pt, txtlen);
  txt[txtlen] = 0;

  return txt;
}
#endif

const char * checkupdates()
{
#ifdef REMOTE_VERSION_CHECK
  static char *latest = NULL;

  if(!latest)
    latest = txtquery(PACKAGE, "ezix.org", NULL);

  return latest;
#else
  return NULL;
#endif
}
