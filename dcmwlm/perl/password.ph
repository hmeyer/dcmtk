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
#   This module contains perl procedures for password creation and checking.
#   Passwords are one-way encrypted using crypt(), and the crypted data
#   is stored as a hexadecimal encoded text file. Therefore, it is not
#   possible to reconstruct the password from the password file.
#
# Last Update:      $Author: wilkens $
# Update Date:      $Date: 2002/12/03 12:16:08 $
# Source File:      $Source: /share/dicom/cvs-depot/dcmtk/dcmwlm/perl/password.ph,v $
# CVS/RCS Revision: $Revision: 1.1 $
# Status:           $State: Exp $
#
# CVS/RCS Log
#   $Log: password.ph,v $
#   Revision 1.1  2002/12/03 12:16:08  wilkens
#   Added files und functionality from the dcmtk/wlisctn folder to dcmtk/dcmwlm
#   so that dcmwlm can now completely replace wlistctn in the public domain part
#   of dcmtk. Pertaining to this replacement requirement, another optional return
#   key attribute was integrated into the wlm utilities.
#
#
#

require 'log.ph';

#
# string makepasswd(string passwd, string AEtitle)
#   encrypt a password string, using the AEtitle string
#   for the initialisation of the crypt() engine.
#   returns a one-way crypted string in hexadecimal coded form.
#
sub makepasswd # (passwd, AEtitle) returns encrypted, encoded string
{
# We derive the "salt" string for crypt (two letters) from the AEtitle
  local($salt1) = 0;
  local($salt2) = 0;
  local($temp, $salt, $crypt);
  local($aetitle) = @_[1];
  while (length($aetitle)>0)
  {
    $temp = $salt1 ^ ord($aetitle);
    $salt1 = $salt2;
    $salt2 = $temp;
    $aetitle = substr($aetitle,1);
  }
  $salt = pack("cc",$salt1, $salt2);
  $crypt = unpack("H*", crypt(@_[0], $salt));
  $crypt;
}

#
#  bool testpasswd(string cryptstring, string passwd, string AEtitle)
#    tests if the passwd/AEtitle combination matches the crypted password string
#    given as first parameter. Returns true if matching, false otherwise.
#
sub testpasswd # (encrypted_string, passwd, AEtitle) returns true if correct.
{
  local($crypt) = &makepasswd(@_[1], @_[2]);
  local($result) = (@_[0] =~ /^$crypt$/i);
  $result;
}

#
#  string passwdfilename(string AEtitle)
#    returns the absolute path / file name of the file where the
#    crypted password for the AEtitle (storage area) is (or should be) stored.
#
sub passwdfilename #(AEtitle)
{
  local($filename) = @_[0];
  if ($filename eq '') { &printlog("password.pl:passwdfilename: unknown AE title @_[0]"); }
  if ($filename !~ /\/.*/) { $filename = join('/',$prefs{'data_path'},$filename); }
  $filename = join('/',$filename,$prefs{'passwdfile'});
  $filename;
}

#
#  void writepasswd(string passwd, string AEtitle)
#    encrypts the password and writes it to file according to the AEtitle
#    (storage area).
#
sub writepasswd # (passwd, AEtitle)
{
  local($crypt) = &makepasswd(@_[0], @_[1]);
  local($filename) = &passwdfilename(@_[1]);
  if (open(PWDFILE,">$filename"))
  {
    printf(PWDFILE "<%s>\n",$crypt);
    close(PWDFILE);
  } else { 
    &printlog("password.pl:writepasswd: cannot create password file for AE title @_[1]");
  }
}

#
# bool checkurlcode(string cryptstring, string AEtitle)
#   checks if the cryptstring matches the one stored in file
#   for the given storage area. This allows to check if the URL path is valid.
#
sub checkurlcode # (cryptstring, AETitle)
{
  local($crypt);
  local($filename) = &passwdfilename(@_[1]);
  if (open(PWDFILE,"<$filename"))
  {
    $crypt = <PWDFILE>;
    chop($crypt);
    close(PWDFILE);
    if ($crypt =~ /<.*>/)
    {
      $crypt =~ s/^.*<//;
      $crypt =~ s/>.*$//;
    } else {
      &printlog("password.pl:checkurlcode: cannot parse password file for AE title @_[1]");
    }
  } else {
    # If no password file available, we use an empty password string as default.
    $crypt = &makepasswd('',@_[1]);
  }
  local($result) = (@_[0] =~ /^$crypt$/i);
  $result;
}

1;
