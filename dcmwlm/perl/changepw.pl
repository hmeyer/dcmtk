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
#   This perl script allows to change the password for a storage area.
#
# Last Update:      $Author: wilkens $
# Update Date:      $Date: 2002/12/03 12:16:03 $
# Source File:      $Source: /share/dicom/cvs-depot/dcmtk/dcmwlm/perl/changepw.pl,v $
# CVS/RCS Revision: $Revision: 1.1 $
# Status:           $State: Exp $
#
# CVS/RCS Log
#   $Log: changepw.pl,v $
#   Revision 1.1  2002/12/03 12:16:03  wilkens
#   Added files und functionality from the dcmtk/wlisctn folder to dcmtk/dcmwlm
#   so that dcmwlm can now completely replace wlistctn in the public domain part
#   of dcmtk. Pertaining to this replacement requirement, another optional return
#   key attribute was integrated into the wlm utilities.
#
#
#

require 'prefs.ph';
require 'layout.ph';
require 'password.ph';
require 'lock.ph';
require 'urldecod.ph';
require 'write.ph';
require 'checkvr.ph';

$path_info=$ENV{'PATH_INFO'};
&get_request;

$aetitle = '';
$passwd = '';
if ($path_info ne '')  
{
  ($dummy, $aetitle, $passwd, $rest) = split(/\//, $path_info);
}

if (($passwd eq '') || (! &checkurlcode($passwd, $aetitle)))
{
  # Password is incorrect.
  &page_title("Password invalid");
  printf("<A HREF=\"%s\">Click here</A> to return to main menu.\n", $prefs{'main.pl'});
  &page_footer;
} else {
  # Password is correct.
  &set_readlock("$prefs{'data_path'}/$aetitle");

  if (-f "$prefs{'data_path'}/$aetitle/$prefs{'publicfile'}")
  {
    &page_title("Public Storage Area");
    printf("It is not possible to change the password for a public storage area.<p>");
    printf("<A HREF=\"%s/%s/%s\">Click here</A> to return to main menu.\n", 
      $prefs{'main.pl'}, $aetitle, $passwd);
    &page_footer;
  } else {
    if ($request ne '')
    {
      # We have received a filled-in form.
      # We save/update it and return a URL to the updated form.
      if ($rqpairs{'action'} eq 'Submit')
      {
        if (&testpasswd($passwd, $rqpairs{'oldpasswd'}, $aetitle))
        {
          if ((length($rqpairs{'newpasswd1'})<6)||($rqpairs{'newpasswd1'} ne $rqpairs{'newpasswd2'}))
          {
            # new PW incorrect
            &page_title("New password invalid");
            printf("You have not entered the same password twice, or the password was shorter than six letters.<br>\n");
            printf("The password has not been changed.<p>\n");
            printf("<A HREF=\"%s/%s/%s\">Click here</A> to retry or return to <A HREF=\"%s/%s/%s\">Main Menu</A>\n", 
              $prefs{'changepw.pl'}, $aetitle, $passwd, 
              $prefs{'main.pl'}, $aetitle, $passwd);
            &page_footer;
          } else {   
            &set_writelock("$prefs{'data_path'}/$aetitle");
            &writepasswd($rqpairs{'newpasswd1'}, $aetitle);
            $passwd = &makepasswd($rqpairs{'newpasswd1'}, $aetitle);
            printf("Location: %s/%s/%s\n\n", $prefs{'main.pl'}, $aetitle, $passwd);
          }
        } else {
          # PW incorrect
          &page_title("Old Password invalid");
          printf("The password has not been changed.<p>\n");
          printf("<A HREF=\"%s%s\">Click here</A> to retry or return to <A HREF=\"%s%s\">Main Menu</A>\n", 
            $prefs{'changepw.pl'}, $path_info, $prefs{'main.pl'}, $path_info);
          &page_footer;
        } 
      } else {
        printf("Location: %s%s\n\n", $prefs{'main.pl'}, $path_info);
      }
    } else {
      # we are requested to display a form
      &page_title("Change Password");
      printf("<FORM METHOD=get ACTION=\"%s/%s/%s\">\n",$prefs{'changepw.pl'}, $aetitle, $passwd);
      printf("<INPUT TYPE=HIDDEN name=\"request\" value=\"                                                            \">\n");
      printf("Please enter the <b>old</b> password for storage area '%s':\n",$aetitle);
      printf("<CENTER><INPUT TYPE=PASSWORD name=\"oldpasswd\" size=20 value=\"\"><p>\n");
      printf("</CENTER>\n");
      printf("Please enter the <b>new</b> password <b>twice</b>:\n");
      printf("<CENTER><INPUT TYPE=PASSWORD name=\"newpasswd1\" size=20 value=\"\"><p>\n");
      printf("<INPUT TYPE=PASSWORD name=\"newpasswd2\" size=20 value=\"\"><p>\n");
      printf("<INPUT TYPE=SUBMIT name=\"action\" value=\"Submit\">\n");
      printf("<INPUT TYPE=SUBMIT name=\"action\" value=\"Cancel\"></CENTER>\n");
      &page_footer;        
    }
  }
  &release_lock;
}


