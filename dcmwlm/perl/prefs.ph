#!/usr/local/bin/perl
#
#  Copyright (C) 1996-2002, OFFIS
#
#  This software and supporting documentation were developed by
#
#    Kuratorium OFFIS e.V.
#    Forschungsbereich 2: Kommunikationssysteme
#    Escherweg 2
#    D-26121 Oldenburg, Germany
#
#  for CEN/TC251/WG4 as a contribution to the Computer Assisted Radiology
#  (CAR) 1996 DICOM Demonstration.
#
#  THIS SOFTWARE IS MADE AVAILABLE,  AS IS,  AND OFFIS MAKES NO  WARRANTY
#  REGARDING  THE  SOFTWARE,  ITS  PERFORMANCE,  ITS  MERCHANTABILITY  OR
#  FITNESS FOR ANY PARTICULAR USE, FREEDOM FROM ANY COMPUTER DISEASES  OR
#  ITS CONFORMITY TO ANY SPECIFICATION. THE ENTIRE RISK AS TO QUALITY AND
#  PERFORMANCE OF THE SOFTWARE IS WITH THE USER.
#
#  Copyright of the software  and  supporting  documentation  is,  unless
#  otherwise stated, owned by OFFIS, and free access is hereby granted as
#  a license to  use  this  software,  copy  this  software  and  prepare
#  derivative works based upon this software.  However, any  distribution
#  of this software source code or supporting documentation or derivative
#  works  (source code and  supporting documentation)  must  include  the
#  three paragraphs of this copyright notice.
#
#
# Module: dcmwlm (WWW Component)
#
# Author: Marco Eichelberg
#
# Purpose:
#   This is the configuration file for the dcmtk Basic Worklist Management
#   WWW component.
#   All supported storage areas and directories must be configured here.
#
# Last Update:      $Author: wilkens $
# Update Date:      $Date: 2002/12/03 12:16:11 $
# Source File:      $Source: /share/dicom/cvs-depot/dcmtk/dcmwlm/perl/prefs.ph,v $
# CVS/RCS Revision: $Revision: 1.1 $
# Status:           $State: Exp $
#
# CVS/RCS Log
#   $Log: prefs.ph,v $
#   Revision 1.1  2002/12/03 12:16:11  wilkens
#   Added files und functionality from the dcmtk/wlisctn folder to dcmtk/dcmwlm
#   so that dcmwlm can now completely replace wlistctn in the public domain part
#   of dcmtk. Pertaining to this replacement requirement, another optional return
#   key attribute was integrated into the wlm utilities.
#
#
#

# --------------------- WWW Server Configuration ---------------------

# RELATIVE path to the WWW script directory, as seen by httpd
$prefs{'cgi_path'}  = '/cgi-bin/worklist';

# RELATIVE path to the WWW html directory, as seen by httpd
$prefs{'html_path'}  = '/worklist';

# ABSOLUTE path to the data directory
$prefs{'data_path'} = '/home/www/wl-data';

# ------------------ Constants - Please do not change! ------------------

# Paths for HTML links.
$prefs{'offis_gif'}   = join('/',$prefs{'html_path'},'offis.gif');
$prefs{'main.pl'}     = join('/',$prefs{'cgi_path'},'main.pl');
$prefs{'patient.pl'}  = join('/',$prefs{'cgi_path'},'patient.pl');
$prefs{'patiedit.pl'} = join('/',$prefs{'cgi_path'},'patiedit.pl');
$prefs{'patidel.pl'}  = join('/',$prefs{'cgi_path'},'patidel.pl');
$prefs{'station.pl'}  = join('/',$prefs{'cgi_path'},'station.pl');
$prefs{'statedit.pl'} = join('/',$prefs{'cgi_path'},'statedit.pl');
$prefs{'statdel.pl'}  = join('/',$prefs{'cgi_path'},'statdel.pl');
$prefs{'procedur.pl'} = join('/',$prefs{'cgi_path'},'procedur.pl');
$prefs{'procedit.pl'} = join('/',$prefs{'cgi_path'},'procedit.pl');
$prefs{'procdel.pl'}  = join('/',$prefs{'cgi_path'},'procdel.pl');
$prefs{'procstep.pl'} = join('/',$prefs{'cgi_path'},'procstep.pl');
$prefs{'prstedit.pl'} = join('/',$prefs{'cgi_path'},'prstedit.pl');
$prefs{'prstdel.pl'}  = join('/',$prefs{'cgi_path'},'prstdel.pl');
$prefs{'worklist.pl'} = join('/',$prefs{'cgi_path'},'worklist.pl');
$prefs{'workedit.pl'} = join('/',$prefs{'cgi_path'},'workedit.pl');
$prefs{'workdel.pl'}  = join('/',$prefs{'cgi_path'},'workdel.pl');
$prefs{'changepw.pl'} = join('/',$prefs{'cgi_path'},'changepw.pl');

# Path to the C++ executables required by "lock.ph", "worklist.pl" etc.
$prefs{'preplock'} = './preplock';
$prefs{'readoviw'} = './readoviw';
$prefs{'readwlst'} = './readwlst';
$prefs{'writwlst'} = './writwlst';

# Name of the password file in each storage area
$prefs{'passwdfile'} = 'passwd.dat';
$prefs{'publicfile'} = 'public';

# Name of the perl data file in each storage area
$prefs{'datafile'}   = 'worklist.dat';

# Name of the (common) log file, relative to $prefs{'data_path'}.
$prefs{'logfile'}    = 'log.dat';

# Name of the file used for read/write locking.
# Attention: Must be identical to the filename used in the C++
# compontents of the dcmtk Basic Worklist Management utility.
$prefs{'lockfile'}   = 'lockfile';

# Filename extension for DICOM files (worklist entries).
$prefs{'dicom_ext'}  = 'wl';

# 
$prefs{'series_UID'}  = '1.2.276.0.7230010.3.5.3';

1;
