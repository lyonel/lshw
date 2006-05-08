/*
 * display.cc
 *
 *
 */

#include "display.h"

static const char rcsid[] = "@(#) $Id: fb.cc 576 2004-05-07 07:13:31Z ezix $";

bool scan_display(hwNode & n)
{
  if((n.getClass() == hw::display) && n.isCapable("vga"))
  {
    n.claim(true);
  }

  for(unsigned int i=0; i<n.countChildren(); i++)
    scan_display(*n.getChild(i));

  return true;
}
