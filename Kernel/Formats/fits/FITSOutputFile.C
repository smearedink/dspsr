/***************************************************************************
 *
 *   Copyright (C) 2015 by Matthew Kerr
 *   Licensed under the Academic Free License version 2.1
 *
 ***************************************************************************/

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "dsp/FITSOutputFile.h"
#include "dsp/Observation.h"
#include "dsp/Rescale.h"
#include "FilePtr.h"

#include "FITSArchive.h"
#include "Pulsar/FITSHdrExtension.h"
#include "Pulsar/dspReduction.h"
#include "Pulsar/Telescope.h"
#include "Pulsar/Receiver.h"
#include "Pulsar/Backend.h"
#include "FITSError.h"
#include "psrfitsio.h"

#include <fcntl.h>

using namespace std;

int get_colnum(fitsfile* fptr, const char* label)
{
  int colnum(0),status(0);
  fits_get_colnum(fptr, CASEINSEN, (char*)label, &colnum, &status);
  if (status)
    throw FITSError(status,"dsp::FITSOutputFile::get_colnum");
  return colnum;
}

void write_col(fitsfile* fptr, const char* label, int irow, int start, 
    int stop, int* data)
{
  int colnum = get_colnum (fptr, label);
  int status = 0;
  fits_write_col(fptr,TINT,colnum,irow,start,stop,data,&status);
  if (status)
    throw FITSError(status,"dsp::FITSOutputFile::write_int_col");
}

void write_col(fitsfile* fptr, const char* label, int irow, int start, 
    int stop, unsigned* data)
{
  int colnum = get_colnum (fptr, label);
  int status = 0;
  fits_write_col(fptr,TINT,colnum,irow,start,stop,data,&status);
  if (status)
    throw FITSError(status,"dsp::FITSOutputFile::write_uint_col");
}

void write_col(fitsfile* fptr, const char* label, int irow, int start, 
    int stop, float* data)
{
  int colnum = get_colnum (fptr, label);
  int status = 0;
  fits_write_col(fptr,TFLOAT,colnum,irow,start,stop,data,&status);
  if (status)
    throw FITSError(status,"dsp::FITSOutputFile::write_dbl_col");
}

void write_col(fitsfile* fptr, const char* label, int irow, int start, 
    int stop, double* data)
{
  int colnum = get_colnum (fptr, label);
  int status = 0;
  fits_write_col(fptr,TDOUBLE,colnum,irow,start,stop,data,&status);
  if (status)
    throw FITSError(status,"dsp::FITSOutputFile::write_dbl_col");
}


void modify_vector_len(fitsfile* fptr, const char* label, int len)
{
  int colnum = get_colnum (fptr, label);
  int status = 0;
  fits_modify_vector_len (fptr, colnum, len, &status); 
  if (status)
    throw FITSError(status,"dsp::FITSOutputFile::modify_vector_len");
}

dsp::FITSOutputFile::FITSOutputFile (const char* filename) 
  : OutputFile ("FITSOutputFile")
{
  if (filename) 
    output_filename = filename;

  offset = 0;
  written = 0;
  isub = 0;
  fptr = NULL;

  // NB these variables should all be set from input
  nchan = 0;
  npol = 0;
  nsblk = 2048;
  nbblk = 0;
  nbit = 2;
}

dsp::FITSOutputFile::~FITSOutputFile ()
{
  finalize_fits ();
}

void dsp::FITSOutputFile::set_nsblk (unsigned nblk)
{
  if ( fptr && (nblk != nsblk) )
    throw Error (InvalidState, "dsp::FITSOutputFile::set_nsblk",
        "cannot change block size after initialization!");
  nsblk = nblk;
}

void dsp::FITSOutputFile::set_nbit (unsigned _nbit)
{
  if ( fptr && (_nbit != nbit) )
    throw Error (InvalidState, "dsp::FITSOutputFile::set_nbit",
        "cannot change nbit after initialization!");
  nbit= _nbit;
}

//! Get the extension to be added to the end of new filenames
std::string dsp::FITSOutputFile::get_extension () const
{
  return ".sf";
}

void dsp::FITSOutputFile::write_header ()
{
  // a dummy archive to handle all of the extensions
  Reference::To<Pulsar::Archive> archive = new Pulsar::FITSArchive;

  const unsigned nbin  = 1;
  const unsigned nsub  = 1;
  npol = get_input() -> get_npol();
  nchan = get_input() -> get_nchan();

  if (nbit == 0)
    throw Error (InvalidState, "dsp::FITSOutputFile::write_header",
        "nbit was not set");
  nbblk = (nsblk * npol * nchan * nbit)/8;
  tblk = double(nsblk) / input -> get_rate();

  //if (verbose)
    cerr << "dsp::FITSOutputFile::write_header" << endl
         << "nchan=" << nchan << " npol=" << npol << " nsblk=" << nsblk
         <<" tblk=" << tblk << " rate=" <<input->get_rate() 
         <<" nbit=" <<nbit << " nbblk=" << nbblk << endl;

  archive-> resize (nsub, npol, nchan, nbin);

  Pulsar::FITSHdrExtension* ext;
  ext = archive->get<Pulsar::FITSHdrExtension>();
  
  if (ext)
  {
    //if (verbose)
      cerr << "dsp::FITSOutputFile::write_header Pulsar::Archive FITSHdrExtension" << endl;

    // Make sure the start time is aligned with pulse phase zero
    // as this is what the PSRFITS format expects.

    MJD initial = get_input()->get_start_time();
    ext->set_start_time (initial);
    ext->set_coordmode("J2000");

    // Set the ASCII date stamp from the system clock (in UTC)

    time_t thetime;
    time(&thetime);
    string time_str = asctime(gmtime(&thetime));

    // Cut off the line feed character
    time_str = time_str.substr(0,time_str.length() - 1);
    ext->set_date_str(time_str);

    ext->set_obsbw ( abs(get_input()->get_bandwidth()) );
    ext->set_obsnchan ( get_input()->get_nchan() );
    ext->set_obsfreq ( get_input()->get_centre_frequency() );
  }

  archive-> set_telescope ( get_input()->get_telescope() );
  archive-> set_type ( get_input()->get_type() );

  switch (get_input()->get_state())
  {
  case Signal::NthPower:
  case Signal::PP_State:
  case Signal::QQ_State:
    archive->set_state (Signal::Intensity);
    break;
  
  case Signal::FourthMoment:
    archive->set_state (Signal::Stokes);
    break;

  default:
    archive-> set_state ( get_input()->get_state() );
  }

  // probably not correct for search mode
  //archive-> set_scale ( Signal::FluxDensity );

  if (verbose)
    cerr << "dsp::Archiver::set Archive source=" << get_input()->get_source()
         << "\n  coord=" << get_input()->get_coordinates()
         << "\n  bw=" << get_input()->get_bandwidth()
         << "\n  freq=" << get_input()->get_centre_frequency () << endl;

  archive-> set_source ( get_input()->get_source() );
  archive-> set_coordinates ( get_input()->get_coordinates() );
  archive-> set_bandwidth ( get_input()->get_bandwidth() );
  archive-> set_centre_frequency ( get_input()->get_centre_frequency() );
  archive-> set_dispersion_measure ( get_input()->get_dispersion_measure() );

  archive-> set_dedispersed( false );
  archive-> set_faraday_corrected (false);

  // Set any available extensions
  // TODO -- c.f. Archiver -- but I think add the ones we need on an
  // ad hoc basis

  //Pulsar::Backend* backend = archive -> get<Pulsar::Backend>();
  if (verbose)
    cerr << "dsp::FITSOutputFile::write_header; set Pulsar::Backend extension" << endl;
  Pulsar::Backend* backend = new Pulsar::Backend;
  backend->set_name( get_input()->get_machine() );
  // dspsr does not correct Stokes V for lower sideband down conversion
  backend->set_downconversion_corrected( false );
  // dspsr uses the conventional sign for complex phase
  backend->set_argument( Signal::Conventional );
  archive->add_extension( backend );


  // Note, this is now called before the set(Integration,...) call below
  // so that the DigitiserCounts extension gets set up correctly the
  // first time.

  //Pulsar::dspReduction* dspR = archive -> getadd<Pulsar::dspReduction>();
  //if (dspR)
  //{
    //if (verbose > 2)
      //cerr << "dsp::Archiver::set Pulsar::dspReduction extension" << endl;
    //set (dspR);
  //}

  Pulsar::Telescope* telescope = archive -> getadd<Pulsar::Telescope>();
  try
  {
    telescope->set_coordinates ( get_input() -> get_telescope() );
  }
  catch (Error& error)
  {
    if (verbose)
      cerr << "dsp::Archiver WARNING could not set telescope coordinates\n\t"
           << error.get_message() << endl;
  }

  // default Receiver extension
  Pulsar::Receiver* receiver = archive -> getadd<Pulsar::Receiver>();
  receiver->set_name ( get_input() -> get_receiver() );
  receiver->set_basis ( get_input() -> get_basis() );

  //for (unsigned iext=0; iext < extensions.size(); iext++)
  //  archive -> add_extension ( extensions[iext] );

  // set_model must be called after the Integration::MJD has been set

  //archive-> set_filename (get_filename (phase));
  archive -> unload (output_filename);
}

void dsp::FITSOutputFile::write_row ()
{
  //if (verbose)
      cerr << "dsp::FITSOutputFile::write_row writing row "<<isub<<endl;
  write_col(fptr,"INDEXVAL",isub,1,1,&isub);
  write_col(fptr,"TSUBINT",isub,1,1,&tblk);
  double offs_sub = tblk/2.0 + isub*tblk;
  write_col(fptr,"OFFS_SUB",isub,1,1,&offs_sub);
  write_col(fptr,"DAT_WTS",isub,1,nchan,&dat_wts[0]);
  write_col(fptr,"DAT_SCL",isub,1,nchan*npol,&dat_scl[0]);
  write_col(fptr,"DAT_OFFS",isub,1,nchan*npol,&dat_offs[0]);
  write_col(fptr,"DAT_FREQ",isub,1,nchan,&dat_freq[0]);
}

void dsp::FITSOutputFile::initialize ()
{
  if (verbose)
    cerr << "dsp::FITSOutputFile::initialize" << endl;

  dat_wts.resize(nchan);
  dat_freq.resize(nchan);
  for (unsigned i = 0; i < nchan; ++i)
  {
    dat_wts[i] = 1.;
    dat_freq[i] = get_input()->get_centre_frequency (i);
  }

  // do not re-initialize if already initialized by Rescale callback
  if (!dat_scl.size()) {
    dat_scl.resize (nchan*npol);
    dat_offs.resize (nchan*npol);
    for (unsigned i = 0; i < nchan*npol; ++i)
    {
      dat_scl[i] = 1.;
      dat_offs[i] = 0.;
    }
  }

  int status = 0;
  fits_open_file (&fptr,output_filename.c_str(), READWRITE, &status);
  if (status)
    throw FITSError (status, "dsp::FITSOutputFile::initialize",
        "unable to open FITS file for writing");

  psrfits_move_hdu(fptr,"SUBINT");

  // psrchive bungs in a "PERIOD" column -- delete it
  try { psrfits_delete_col (fptr, "PERIOD"); }
  catch (FITSError& e) {;}

  // set up channel-dependent entries with correct size
  modify_vector_len(fptr,"DAT_FREQ",nchan);
  modify_vector_len(fptr,"DAT_WTS",nchan);
  modify_vector_len(fptr,"DAT_SCL",nchan*npol);
  modify_vector_len(fptr,"DAT_OFFS",nchan*npol);

  // set the block (DATA) dim entries
  // NB that the total number of bytes (TFORM) can't be set consistently
  // with the dimension size for 2- and 4-bit data if one takes time 
  // samples as a dimension; the standard is for 
  // TDIM = (nchan,npol,nsblk*nbit/8)

  modify_vector_len(fptr,"DATA",nbblk);
  int colnum = get_colnum(fptr,"DATA");
  psrfits_update_tdim (fptr, colnum, nchan, npol, (nsblk*nbit)/8 );


  psrfits_update_key<int> (fptr, "NSBLK", nsblk);
  psrfits_update_key<double> (fptr, "TBIN", 1./get_input()->get_rate());
  psrfits_update_key<int> (fptr, "NBITS", nbit);
  psrfits_update_key<double> (fptr, "ZERO_OFF", pow(2,nbit-1)-0.5 );
  psrfits_update_key<int> (fptr, "SIGNINT", 0);

  // TODO -- will need to fix this later on
  psrfits_update_key<int> (fptr, "NSUBOFFS", 0);
}

void dsp::FITSOutputFile::operation ()
{

  if (!fptr) {
    write_header ();
    initialize ();
  }
  if (verbose)
    cerr << "dsp::FITSOutputFile::operation" << endl;

  cerr << "dsp::FITSOutputFile::operation" << " start_time="<<input->get_start_time().printall()<<" end_time="<<input->get_end_time().printall() << " input_sample="<<input->get_input_sample()<<std::endl;
  unload_bytes (get_input()->get_rawptr(), get_input()->get_nbytes());

}

int64_t dsp::FITSOutputFile::unload_bytes (const void* void_buffer, uint64_t bytes)
{

  //if (verbose)
    cerr << "dsp::FITSOutputFile::unload_bytes" << endl
         << "    bytes=" << bytes << " nbblk=" << nbblk
         <<" offset=" << offset << " isub=" << isub << endl;
  // cast to char buffer for profit
  unsigned char* buffer = (unsigned char*) void_buffer;

  unsigned to_write = bytes;
  int status = 0;
  int colnum = get_colnum (fptr, "DATA");
  
  // write to incomplete block first
  if (offset)
  {
    if (verbose)
      cerr << "writing to incomplete block" << endl;
    unsigned remainder = nbblk - offset;

    // finish remainder of subint
    if (bytes > remainder)
    {
      fits_write_col_byt (fptr, colnum, isub, offset, remainder, 
          buffer, &status);
      buffer += remainder;
      to_write -= remainder;
      offset = 0;
    }

    // write all available bytes without advancing subint
    else
    {
      fits_write_col_byt (fptr, colnum, isub, offset, bytes, 
          buffer, &status);
      written += bytes;
      offset += bytes;
      return bytes;
    }
  }

  while (to_write >= nbblk)
  {
    if (verbose)
      cerr << "writing to new block" << endl;
    // will give correct one-based index first time through
    isub += 1;

    // write ancillary data for new block/subint
    write_row ();

    // Now write that data into a subintegration in the PSRFITS file
    fits_write_col_byt (fptr, colnum, isub, 1, nbblk, buffer, &status);
    to_write -= nbblk;
    buffer += nbblk;
  }

  // write out remaining bytes to partial subbint
  if (to_write)
  {
    isub += 1;
    write_row();
    fits_write_col_byt (fptr, colnum, isub, 1, to_write, buffer, &status);
    offset += to_write;
  }

  return bytes;

}

void dsp::FITSOutputFile::finalize_fits ()
{
  if (verbose)
    cerr << "dsp::FITSOutputFile::finalize_fits" << endl;
  if (fptr) {
    psrfits_update_key<int> (fptr, "NAXIS2", isub);
    psrfits_update_key<int> (fptr, "NSTOT", written * (8/nbit) );
    int status = 0;
    fits_close_file(fptr, &status);
    if (status)
      throw FITSError(status, "dsp::FITSOutputFile");
    fptr = NULL;
  }
}

void dsp::FITSOutputFile::set_reference_spectrum (Rescale* rescale)
{
  // Because this is implemeneted via callback, the first call happens
  // before initialization of the arrays, etc.  So we take it on faith that
  // we can initialize based on rescale's input/output.
  // Reference spectrum packed in PF order
  if (verbose)
    cerr << "dsp::FITSOutputFile::set_reference_spectrum" << endl;
  if (nchan == 0)
  {
    nchan = rescale->get_input()->get_nchan();
    npol = rescale->get_input()->get_npol();
    //if (verbose)
      cerr << "    initializing from Rescale" 
           << " nchan=" << nchan << " npol=" << npol << endl;
    // need to initialize
    dat_scl.resize(nchan*npol);
    dat_offs.resize(nchan*npol);
  }

  bool pol_bad = npol != rescale->get_input()->get_npol();
  bool chan_bad = nchan != rescale->get_input()->get_nchan();
  if (pol_bad || chan_bad)
    throw Error(InvalidState, "dsp::FITSOutputFile::set_reference_spectrum",
        "channels/polarizations did not match Rescale");

  for (unsigned ipol = 0; ipol < npol; ++ipol) 
  {
    unsigned offset = ipol * nchan;
    const float* scl = rescale->get_scale (ipol);
    const float* offs = rescale->get_offset (ipol);
    for (unsigned jchan = 0; jchan < nchan; ++jchan)
    {
      // Rescale uses opposite convention to PSRFITS DAT_SCL/DAT_OFFS
      dat_scl[jchan+offset] = 1./scl[jchan];
      dat_offs[jchan+offset] = -offs[jchan];
    }
  }
}

