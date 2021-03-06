//-*-C++-*-
/***************************************************************************
 *
 *   Copyright (C) 2015 by Matthew Kerr
 *   Licensed under the Academic Free License version 2.1
 *
 ***************************************************************************/

// Recall that PSRFITS search mode data are in TPF order, which complicates
// packing into bytes.

#ifndef __FITSDigitizer_h
#define __FITSDigitizer_h

#include "dsp/Digitizer.h"

namespace dsp
{  
  //! Converts floating point values to N-bit PSRFITS search-mode format
  class FITSDigitizer: public Digitizer
  {
  public:

    //! Default constructor
    FITSDigitizer (unsigned _nbit);

    unsigned get_nbit () const {return nbit;}

    //! Pack the data
    void pack ();

    //! Return minimum samples
    // TODO -- is this needed?
    uint64_t get_minimum_samples () { return 4096; }


  protected:

    void set_nbit (unsigned);

  };
}

#endif
