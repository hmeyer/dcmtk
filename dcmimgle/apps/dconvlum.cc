/*
 *
 *  Copyright (C) 1999-2005, OFFIS
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
 *  Module:  dcmimgle
 *
 *  Authors: Joerg Riesmeier
 *
 *  Purpose: convert VeriLUM CCx_xx.dat files to DCMTK display files
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:42:41 $
 *  CVS/RCS Revision: $Revision: 1.20 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */


#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#define INCLUDE_CSTDLIB
#define INCLUDE_CCTYPE
#include "dcmtk/ofstd/ofstdinc.h"

#include "dcmtk/ofstd/ofstream.h"
#include "dcmtk/ofstd/ofconsol.h"
#include "dcmtk/ofstd/ofstd.h"
#include "dcmtk/dcmdata/dcuid.h"    /* for dcmtk version name */


#define OFFIS_CONSOLE_APPLICATION "dconvlum"

static char rcsid[] = "$dcmtk: " OFFIS_CONSOLE_APPLICATION " v"
  OFFIS_DCMTK_VERSION " " OFFIS_DCMTK_RELEASEDATE " $";


int main(int argc, char *argv[])
{
    /* check number of arguments */
    if ((argc >= 3) && (argc <= 4))
    {
#ifdef HAVE_IOS_NOCREATE
        ifstream input(argv[1], ios::in|ios::nocreate);
#else
        ifstream input(argv[1], ios::in);
#endif
        if (input)
        {
            ofstream output(argv[2]);
            if (output)
            {
                const unsigned int maxddl = 255;                                    // maximum DDL value (8 bits)
                output << "# Monitor characteristic file (converted from VeriLUM to DCMTK)" << endl << "# ";
                char c;
                while (input.get(c) && (c != '\n') && (c != '\r'))                  // read description
                    output << c;                                                    // ... and write it to output file
                output << endl << endl;
                output << "# maximum DDL value" << endl << endl;
                output << "  max   " << maxddl << endl << endl;
                if (argc == 4)
                {
                    double ambient = OFStandard::atof(argv[3]);
                    output << "# ambient light value" << endl << endl;
                    output << "  amb   " << ambient << endl << endl;
                }
                output << "# DDL   LumVal" << endl << endl;
                double lum;
                unsigned int ddl;
                unsigned int i = 0;
                while ((i <= maxddl) && input.good() && output.good())
                {
                    while (input.get(c) && !isdigit(c));                            // skip non-numeric chars
                    input.putback(c);
                    input >> ddl;                                                   // read DDL value
                    while (input.get(c) && !isdigit(c));                            // skip non-numeric chars
                    input.putback(c);
                    input >> lum;                                                   // read luminance value
                    output.width(5);
                    output << ddl;                                                  // write DDL value
                    output.width(12);
                    output.setf(ios::fixed, ios::floatfield);
                    output << lum << endl;                                          // write luminance value
                    ++i;
                }
                if (i <= maxddl)
                    CERR << "ERROR: can't convert input file ... error in line #" << i << " !" << endl;
                else {
                    output << endl << "# eof of file";
                    return 0;                                                       // everything is OK
                }
             } else
                CERR << "ERROR: can't create output file !" << endl;
        } else
            CERR << "ERROR: can't open input file !" << endl;
    } else {
        /* print usage */
        CERR << rcsid << endl << endl;
        CERR << OFFIS_CONSOLE_APPLICATION << ": Convert VeriLUM \"CCx_xx.dat\" files to DCMTK display files" << endl;
        CERR << "usage: " << OFFIS_CONSOLE_APPLICATION << " in-file out-file [ambient]" << endl << endl;
        CERR << "parameters:" << endl;
        CERR << "  in-file   VeriLUM characteristic curve file to be converted" << endl;
        CERR << "  out-file  DCMTK display file to be written" << endl;
        CERR << "  ambient   ambient light (cd/m^2, floating point value)" << endl;
    }
    return 1;                                                                       // an error has happened
}


/*
 * CVS/RCS Log:
 * $Log: dconvlum.cc,v $
 * Revision 1.20  2005/12/08 15:42:41  meichel
 * Changed include path schema for all DCMTK header files
 *
 * Revision 1.19  2004/01/05 14:49:53  joergr
 * Removed acknowledgements with e-mail addresses from CVS log.
 *
 * Revision 1.18  2003/12/23 15:40:13  joergr
 * Replaced post-increment/decrement operators by pre-increment/decrement
 * operators where appropriate (e.g. 'i++' by '++i').
 *
 * Revision 1.17  2003/12/08 19:24:24  joergr
 * Updated copyright header.
 *
 * Revision 1.16  2002/11/27 14:08:00  meichel
 * Adapted module dcmimgle to use of new header file ofstdinc.h
 *
 * Revision 1.15  2002/07/18 12:22:37  joergr
 * Adapted "usage" text to the dcmtk standard format.
 *
 * Revision 1.14  2002/06/20 12:07:57  meichel
 * Changed toolkit to use OFStandard::atof instead of atof, strtod or
 *   sscanf for all string to double conversions that are supposed to
 *   be locale independent
 *
 * Revision 1.13  2002/04/16 13:52:55  joergr
 * Added configurable support for C++ ANSI standard includes (e.g. streams).
 *
 * Revision 1.12  2001/09/28 12:55:53  joergr
 * Added check whether ios::nocreate exists.
 *
 * Revision 1.11  2001/06/01 15:49:38  meichel
 * Updated copyright header
 *
 * Revision 1.10  2000/03/08 16:24:10  meichel
 * Updated copyright header.
 *
 * Revision 1.9  2000/03/03 14:09:08  meichel
 * Implemented library support for redirecting error messages into memory
 *   instead of printing them to stdout/stderr for GUI applications.
 *
 * Revision 1.8  2000/02/23 15:12:13  meichel
 * Corrected macro for Borland C++ Builder 4 workaround.
 *
 * Revision 1.7  2000/02/01 10:52:34  meichel
 * Avoiding to include <stdlib.h> as extern "C" on Borland C++ Builder 4,
 *   workaround for bug in compiler header files.
 *
 * Revision 1.6  1999/04/28 14:44:11  joergr
 * Added newline to error output.
 *
 * Revision 1.5  1999/03/03 11:41:53  joergr
 * Added support to specify ambient light value (re: Barten transformation).
 *
 * Revision 1.4  1999/02/23 16:54:10  joergr
 * Corrected some typos and formatting.
 *
 * Revision 1.3  1999/02/11 16:55:23  joergr
 * Added mode ios::nocreate when opening file streams for reading to avoid
 * implicit creation of non-existing files.
 *
 * Revision 1.2  1999/02/08 12:35:23  joergr
 * Corrected some typos and formatting.
 *
 * Revision 1.1  1999/02/05 15:16:16  joergr
 * Added console application to convert VeriLUM LUT files to dcmtk DISPLAY
 * file used for Barten transformation.
 *
 *
 */
