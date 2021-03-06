//-*-C++-*-
/***************************************************************************
 *
 *   Copyright (C) 2001 by Willem van Straten
 *   Licensed under the Academic Free License version 2.1
 *
 ***************************************************************************/

/* $Source: /cvsroot/dspsr/dspsr/Kernel/Formats/cpsr/dsp/rdisk.h,v $
   $Revision: 1.6 $
   $Date: 2006/10/15 23:26:47 $
   $Author: straten $ */

#ifndef __RDISK_H
#define __RDISK_H

#include <iostream>
#include <string>
#include <vector>

class rdisk {

 public:
  std::string machine;
  std::string path;

  static std::string rsh;

  rdisk () {};
  rdisk (const std::string& parse) { load (parse); }

  void load (const std::string& parse);

  // returns the available space in bytes
  double space () const;

  static void load (std::vector<rdisk>& disks, const char* filename);

};

inline std::ostream& operator << (std::ostream& ostr, const rdisk& rd)
{ return ostr << rd.machine << ":" << rd.path; }

#endif
