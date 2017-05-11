
#ifndef __CUFLU_H__
#define __CUFLU_H__



// *********************************************************************
// ** This header will be included by all CPU/GPU fluid/ELBDM solvers **
// *********************************************************************


// include "Typedef" here since the header "DAINO.h" is NOT included in GPU solvers
#include "Typedef.h"


// faster integer multiplication in Fermi
#if ( defined __CUDACC__  &&  __CUDA_ARCH__ >= 200 )
#  define __umul24( a, b )   ( (a)*(b) )
#  define  __mul24( a, b )   ( (a)*(b) )
#endif


// #################################
// ## macros for different models ##
// #################################

// 1. hydro macro
//=========================================================================================
#if   ( MODEL == HYDRO )
// 5-element structure for GPU kernels
struct FluVar { real Rho, Px, Py, Pz, Egy; };


// size of different arrays 
#if ( FLU_SCHEME == MHM  ||  FLU_SCHEME == MHM_RP  ||  FLU_SCHEME == CTU )

#  define N_FC_VAR        ( PS2 + 2      )
#  define N_SLOPE_PPM     ( N_FC_VAR + 2 )
   
#  if   ( FLU_SCHEME == MHM )

#     define N_FL_FLUX    ( PS2 + 1      )
#     define N_FC_FLUX    ( N_FL_FLUX    )

#  elif ( FLU_SCHEME == MHM_RP )

#     define N_FL_FLUX    ( PS2 + 1      )
#     define N_HF_VAR     ( FLU_NXT - 2  )
#     define N_HF_FLUX    ( FLU_NXT - 1  )
#     define N_FC_FLUX    ( N_HF_FLUX    )

#  elif ( FLU_SCHEME == CTU )

#     define N_FL_FLUX    ( N_FC_VAR     )
#     define N_HF_FLUX    ( N_FC_VAR     ) 
#     define N_FC_FLUX    ( N_FC_VAR     )

#  endif

#endif // #if ( FLU_SCHEME == MHM  ||  FLU_SCHEME == MHM_RP  ||  FLU_SCHEME == CTU )


// enforce pressure to be positive
#define ENFORCE_POSITIVE


// perform spatial data reconstruction in characteristic variables (default: primitive variables)
#if ( FLU_SCHEME == MHM  ||  FLU_SCHEME == MHM_RP  ||  FLU_SCHEME == CTU )
#  ifndef GRAVITY
#     define CHAR_RECONSTRUCTION
#  endif
#endif


// Verify that the density and pressure in the intermediate states of Roe's Riemann solver are positive.
// If either the density of pressure is negative, we switch to other Riemann solvers (EXACT/HLLE/HLLC)
#if (  ( FLU_SCHEME == MHM || FLU_SCHEME == MHM_RP || FLU_SCHEME == CTU )  &&  RSOLVER == ROE  )
#  define CHECK_INTERMEDIATE    HLLC
#endif


// do not use the reference states for HLL solvers during the data reconstruction, as suggested in ATHENA
#if (  FLU_SCHEME != RTVD  &&  ( RSOLVER == HLLE || RSOLVER == HLLC )  )

#  define HLL_NO_REF_STATE

// include waves both from left and right directions during the data reconstruction, as suggested in ATHENA
#  ifdef HLL_NO_REF_STATE 
#     define HLL_INCLUDE_ALL_WAVES
#  endif

#endif


// use the dissipative structure for the WAF scheme
#if ( FLU_SCHEME == WAF )
// #define WAF_DISSIPATE
#endif


// maximum allowed error for the exact Riemann solver and the WAF scheme
#if ( FLU_SCHEME == WAF  ||  ( FLU_SCHEME != RTVD && RSOLVER == EXACT )  ||  CHECK_INTERMEDIATE == EXACT )
#  ifdef FLOAT8
#     define MAX_ERROR    1.e-15
#  else
#     define MAX_ERROR    1.e-06f
#  endif
#endif


// 2. MHD macro
//=========================================================================================
#elif ( MODEL == MHD )
#warning : WAIT MHD !!!!


// 3. ELBDM macro
//=========================================================================================
#elif ( MODEL == ELBDM )


#else
#  error : ERROR : unsupported MODEL !!
#endif // MODEL



// ###############################################################
// ## number of threads in x/y directions for different solvers ##
// ###############################################################

// 1. hydro solver
//=========================================================================================
#if   ( MODEL == HYDRO )
#if   ( FLU_SCHEME == RTVD )

#     define FLU_BLOCK_SIZE_X   FLU_NXT

#  ifdef FLOAT8
#     define FLU_BLOCK_SIZE_Y    4
#  else
#     define FLU_BLOCK_SIZE_Y    8
#  endif

#elif ( FLU_SCHEME == WAF )

 #    define FLU_BLOCK_SIZE_X    FLU_NXT

#  ifdef FLOAT8
#     define FLU_BLOCK_SIZE_Y    4
#  else
#     ifdef FERMI
#     define FLU_BLOCK_SIZE_Y    8
#     else
#     define FLU_BLOCK_SIZE_Y    6
#     endif
#  endif

#elif ( FLU_SCHEME == MHM  ||  FLU_SCHEME == MHM_RP )

#  ifdef FERMI
#     define FLU_BLOCK_SIZE_X    512
#  else
#     define FLU_BLOCK_SIZE_X    128
#  endif

#     define FLU_BLOCK_SIZE_Y    1

#elif ( FLU_SCHEME == CTU )

#  ifdef FERMI
#     define FLU_BLOCK_SIZE_X    512
#  else
#     define FLU_BLOCK_SIZE_X    128
#  endif

#     define FLU_BLOCK_SIZE_Y    1

#else
#  error : ERROR : unsupported hydro scheme in the makefile !!
#endif


// 2. MHD solver
//=========================================================================================
#elif ( MODEL == MHD )
#     warning : WAIT MHD !!!
#     define FLU_BLOCK_SIZE_X    0
#     define FLU_BLOCK_SIZE_Y    0


// 3. ELBDM kinematic solver
//=========================================================================================
#elif ( MODEL == ELBDM )
#     define FLU_BLOCK_SIZE_X    PS2

#  ifdef FLOAT8
#     define FLU_BLOCK_SIZE_Y    4
#  else
#     define FLU_BLOCK_SIZE_Y   16
#  endif


#else
#  error : ERROR : unsupported MODEL !!
#endif // MODEL



#endif // #ifndef __CUFLU_H__
