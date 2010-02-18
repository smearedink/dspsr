//-*-C++-*-
/***************************************************************************
 *
 *   Copyright (C) 2002 by Willem van Straten
 *   Licensed under the Academic Free License version 2.1
 *
 ***************************************************************************/

/* $Source: /cvsroot/dspsr/dspsr/Signal/General/dsp/Filterbank.h,v $
   $Revision: 1.19 $
   $Date: 2010/02/18 13:07:17 $
   $Author: straten $ */

#ifndef __Filterbank_h
#define __Filterbank_h

#include "dsp/Convolution.h"

namespace dsp {
  
  //! Breaks a single-band TimeSeries into multiple frequency channels
  /* This class implements the coherent filterbank technique described
     in Willem van Straten's thesis.  */

  class Filterbank: public Convolution {

  public:

    //! Null constructor
    Filterbank ();

    //! Prepare all relevant attributes
    void prepare ();

    //! Reserve the maximum amount of output space required
    void reserve ();

    //! Get the minimum number of samples required for operation
    uint64_t get_minimum_samples () { return nsamp_fft; }

    //! Get the minimum number of samples lost
    uint64_t get_minimum_samples_lost () { return nsamp_overlap; }

    //! Return a descriptive string
    //virtual const string descriptor () const;

    //! Initialize from a descriptor string as output by above
    //virtual void initialize (const string& descriptor);

    //! Set the number of channels into which the input will be divided
    void set_nchan (unsigned _nchan) { nchan = _nchan; }

    //! Get the number of channels into which the input will be divided
    unsigned get_nchan () const { return nchan; }

    //! Set the frequency resolution factor
    void set_freq_res (unsigned _freq_res) { freq_res = _freq_res; }
    void set_frequency_resolution (unsigned fres) { freq_res = fres; }

    //! Get the frequency resolution factor
    unsigned get_freq_res () const { return freq_res; } 

    //! Set the order of the dimensions in the output TimeSeries
    void set_output_order (TimeSeries::Order);

    //! Engine used to perform discrete convolution step
    class Engine;
    void set_engine (Engine*);

  protected:

    //! Perform the convolution transformation on the input TimeSeries
    virtual void transformation ();

    //! Implements a time-major-order filterbank
    void tfp_filterbank ();

    //! Number of channels into which the input will be divided
    unsigned nchan;

    //! Frequency resolution factor
    unsigned freq_res;

    //! The order of the dimensions in the output TimeSeries
    TimeSeries::Order output_order;

    //! Interface to alternate processing engine (e.g. GPU)
    Reference::To<Engine> engine;

  private:

    void make_preparations ();
    void prepare_output (uint64_t ndat = 0, bool set_ndat = false);
    void resize_output (bool reserve_extra = false);

    unsigned nchan_subband;
  };
 
  class Filterbank::Engine : public Reference::Able
  {
  public:
    virtual void setup (unsigned nchan, unsigned bwd_nfft, float* kernel) = 0;
    virtual void perform (const float* in) = 0;
    std::vector<float*> output_ptr;
    float* scratch;
    unsigned nfilt_pos;
    unsigned nkeep;
    unsigned freq_res;
  }; 
}

#endif

