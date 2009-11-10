/*
 *
 *  Copyright (C) 1996-2005, OFFIS
 *
 *  This software and supporting documentation were developed by
 *
 *    Kuratorium OFFIS e.V.
 *    Healthcare Information and Communication Systems
 *    Escherweg 2
 *    D-26121 Oldenburg, Germany
 *
 *  THIS SOFTWARE IS MADE AVAILABLE,  AS IS,  AND OFFIS MAKES NO  WARRANTY
 *  REGARDING  THE  SOFTWARE,  ITS  PERFORMANCE,  ITS  MERCHANTABILITY  OR
 *  FITNESS FOR ANY PARTICULAR USE, FREEDOM FROM ANY COMPUTER DISEASES  OR
 *  ITS CONFORMITY TO ANY SPECIFICATION. THE ENTIRE RISK AS TO QUALITY AND
 *  PERFORMANCE OF THE SOFTWARE IS WITH THE USER.
 *
 *  Module:  dcmwlm
 *
 *  Author:  Marco Eichelberg
 *
 *  Purpose:
 *   This application creates binary coded "struct flock"  fields for  use
 *   within the "fcntl" function of a perl script. The output of this file
 *   is system dependent, therefore it should be called dynamically  (e.g.
 *   upon startup of the perl script). This  will  prevent  problems  with
 *   systems supporting fat binaries (e.g. NeXTStep).
 *
 *   Usage example (from perl):
 *     $fcntldata=`./preplock`;
 *     eval $fcntldata;
 *   This will create four global perl variables:
 *     $const_F_SETLKW      (2nd fnctl parameter for locking files)
 *     $const_RDLCK_STRUCT  (3rd   "      "       "  read lock)
 *     $const_WRLCK_STRUCT  ( "    "      "       "  write lock)
 *     $const_UNLCK_STRUCT  ( "    "      "       "  unlock)
 *
 *   A perl fcntl call using these (packed binary) values will lock/unlock a
 *   whole file, which must be a successfully opened perl  file  descriptor.
 *   See perl man page for details. 
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:48:37 $
 *  Source File:      $Source: /share/dicom/cvs-depot/dcmtk/dcmwlm/wwwapps/preplock.cc,v $
 *  CVS/RCS Revision: $Revision: 1.3 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#include "dcmtk/config/osconfig.h"

#define INCLUDE_CSTDIO
#include "dcmtk/ofstd/ofstdinc.h"

BEGIN_EXTERN_C
#include <fcntl.h>
END_EXTERN_C

int main()
{
#ifdef __MINGW32__
  /* MinGW does not support flock, there is no way to make this work. */
  printf("die \"Sorry, this application is not supported on the MinGW platform\";\n");
#else
  struct flock lockdata;
  unsigned int i;
  unsigned char *c;

  /* first, we initialize the data */  
  c = (unsigned char *)(&lockdata);
  for (i=0; i<sizeof(struct flock); i++) c[i]=0;

  lockdata.l_whence=0;
  lockdata.l_start=0;
  lockdata.l_len=0;

  /* now we create the output */
  printf("$const_F_SETLKW = %ld;\n",(long)F_SETLKW);

  lockdata.l_type = F_RDLCK;
  printf("$const_RDLCK_STRUCT = pack(\"");
  for (i=0; i<sizeof(struct flock); i++) printf("c");
  printf("\"");
  for (i=0; i<sizeof(struct flock); i++) printf(",%d",(int)(c[i]));
  printf(");\n");
  
  lockdata.l_type = F_WRLCK;
  printf("$const_WRLCK_STRUCT = pack(\"");
  for (i=0; i<sizeof(struct flock); i++) printf("c");
  printf("\"");
  for (i=0; i<sizeof(struct flock); i++) printf(",%d",(int)(c[i]));
  printf(");\n");
  
  lockdata.l_type = F_UNLCK;
  printf("$const_UNLCK_STRUCT = pack(\"");
  for (i=0; i<sizeof(struct flock); i++) printf("c");
  printf("\"");
  for (i=0; i<sizeof(struct flock); i++) printf(",%d",(int)(c[i]));
  printf(");\n");

#endif
  return 0;  
}

/*
 * CVS/RCS Log
 *   $Log: preplock.cc,v $
 *   Revision 1.3  2005/12/08 15:48:37  meichel
 *   Changed include path schema for all DCMTK header files
 *
 *   Revision 1.2  2003/07/03 14:26:57  meichel
 *   When compiling on MinGW, only issues "die" command since the flock
 *     family of functions is not available on this platform and, therefore,
 *     preplock cannot be made to work.
 *
 *   Revision 1.1  2002/12/03 12:17:35  wilkens
 *   Added files und functionality from the dcmtk/wlisctn folder to dcmtk/dcmwlm
 *   so that dcmwlm can now completely replace wlistctn in the public domain part
 *   of dcmtk. Pertaining to this replacement requirement, another optional return
 *   key attribute was integrated into the wlm utilities.
 *
 *
 */
