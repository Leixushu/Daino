
#ifndef __GLOBAL_H__
#define __GLOBAL_H__



#include "Macro.h"
#include "AMR.h"

#ifdef OOC
#include "OOC.h"
#endif


// **********************************************************************************************************
// ** Variables in CAPITAL letters are loaded from the parameter file "Input__Parameter". Please refer to  **
// ** that file for detailed descritpions of each variable.                                                **
// **********************************************************************************************************


// 1. common global variables
// ============================================================================================================
extern AMR_t     *patch;                              // pointer storing all patch information (data and relation)
#ifdef OOC
extern AMR_t     *OOC_patch;                          // pointer for IO/computation overlapping in OOC computing
extern OOC_t      ooc;                                // data structure for the out-of-core computing
#endif
extern double     Time[NLEVEL];                       // "present"  physical time at each level
extern double     Time_Prev[NLEVEL];                  // "previous" physical time at each level
extern uint       AdvanceCounter[NLEVEL];             // number of sub-steps that each level has been evolved
extern long int   Step;                               // number of main steps
extern double     dTime_Base;                         // time interval to update physical time at the base level

extern real       MinDtInfo_Fluid[NLEVEL];            // maximum CFL speed at each level
extern double     FlagTable_Rho        [NLEVEL-1];    // refinement criterion of density
extern double     FlagTable_RhoGradient[NLEVEL-1];    // refinement criterion of density gradient
extern double     FlagTable_Lohner     [NLEVEL-1][3]; // refinement criterion based on Lohner's error estimator
extern double     FlagTable_User       [NLEVEL-1];    // user-defined refinement criterion
extern double    *DumpTable;                          // dump table recording the physical times to output data
extern int        DumpTable_NDump;                    // number of data dumps in the dump table
extern int        DumpID;                             // index of the current output file
extern double     DumpTime;                           // time of the next dump (for OPT__OUTPUT_MODE=1)

extern int        MPI_Rank;                           // MPI rank ID in the MPI_COMM_WORLD
extern int        MPI_Rank_X[3];                      // order of this MPI process in x/y/z directions
extern int        MPI_SibRank[26];                    // sibling MPI rank ID: same order as sibling[26]
extern int        NX0[3];                             //  of base-level cells per process in x/y/z directions
extern int        NPatchTotal[NLEVEL];                // total number of patches in all ranks
extern int       *BaseP;                              // table recording the IDs of the base-level patches
extern int        Flu_ParaBuf;                        // number of parallel buffers to exchange all fluid 
                                                      // variables for the fluid solver and fluid refinement

extern double     BOX_SIZE, DT__FLUID, END_T, OUTPUT_DT;
extern long int   END_STEP;
extern int        NX0_TOT[3], OUTPUT_STEP, REGRID_COUNT, FLU_GPU_NPGROUP, OMP_NTHREAD;
extern int        MPI_NRank, MPI_NRank_X[3], GPU_NSTREAM, FLAG_BUFFER_SIZE, MAX_LEVEL;

extern int        OPT__UM_START_LEVEL, OPT__UM_START_NVAR, OPT__GPUID_SELECT, OPT__PATCH_COUNT;
extern int        OPT__OUTPUT_TOTAL, OPT__CK_CONSERVATION, INIT_DUMPID, OPT__FLAG_LOHNER;
extern real       OPT__CK_MEMFREE, OUTPUT_PART_X, OUTPUT_PART_Y, OUTPUT_PART_Z;
extern bool       OPT__FLAG_RHO, OPT__FLAG_RHO_GRADIENT, OPT__FLAG_USER;
extern bool       OPT__DT_USER, OPT__RECORD_DT, OPT__RECORD_MEMORY, OPT__ADAPTIVE_DT;
extern bool       OPT__FIXUP_RESTRICT, OPT__INIT_RESTRICT, OPT__VERBOSE;
extern bool       OPT__INT_TIME, OPT__OUTPUT_ERROR, OPT__OUTPUT_BASE, OPT__OVERLAP_MPI, OPT__TIMING_BARRIER;
extern bool       OPT__OUTPUT_BASEPS, OPT__CK_REFINE, OPT__CK_PROPER_NESTING, OPT__CK_FINITE;
extern bool       OPT__CK_RESTRICT, OPT__CK_PATCH_ALLOCATE, OPT__FIXUP_FLUX, OPT__CK_FLUX_ALLOCATE;

extern OptInit_t        OPT__INIT;
extern OptRestartH_t    OPT__RESTART_HEADER;
extern IntScheme_t      OPT__FLU_INT_SCHEME, OPT__REF_FLU_INT_SCHEME;
extern OptOutputMode_t  OPT__OUTPUT_MODE;
extern OptOutputPart_t  OPT__OUTPUT_PART;



// 2. global variables for different applications
// ============================================================================================================
// (2-1) fluid solver in different models
#if   ( MODEL == HYDRO )
extern double     FlagTable_PresGradient[NLEVEL-1];   // refinement criterion of pressure gradient
extern real       GAMMA, MINMOD_COEFF, EP_COEFF; 
extern LR_Limiter_t  OPT__LR_LIMITER;
extern WAF_Limiter_t OPT__WAF_LIMITER;
extern bool       OPT__FLAG_PRES_GRADIENT;
extern int        OPT__CK_NEGATIVE;

#elif ( MODEL == MHD )
#warning WAIT MHD !!!

#elif ( MODEL == ELBDM )
extern double     DT__PHASE, FlagTable_EngyDensity[NLEVEL-1][2];
extern bool       OPT__FLAG_ENGY_DENSITY, OPT__INT_PHASE;
extern real       ELBDM_MASS, PLANCK_CONST, ETA;
extern real       MinDtInfo_Phase[NLEVEL];            // maximum time derivative of phase at each level

#else
#  error : ERROR : unsupported MODEL !!
#endif // MODEL


// (2-2) self-gravity
// ============================================================================================================
#ifdef GRAVITY
extern double     AveDensity;                         // average density in all levels
extern real       MinDtInfo_Gravity[NLEVEL];          // maximum gravitational acceleration at each level
extern int        Pot_ParaBuf;                        // number of parallel buffers to exchange potential for the
                                                      // Poisson/Gravity solvers and the potential refinement
extern int        Rho_ParaBuf;                        // number of parallel buffers to exchange density for the 
                                                      // Poisson solver
extern double     DT__GRAVITY; 
extern real       NEWTON_G;
extern int        POT_GPU_NPGROUP;
extern bool       OPT__OUTPUT_POT, OPT__GRA_P5_GRADIENT;
extern real       SOR_OMEGA;
extern int        SOR_MAX_ITER, SOR_MIN_ITER;
extern real       MG_TOLERATED_ERROR;
extern int        MG_MAX_ITER, MG_NPRE_SMOOTH, MG_NPOST_SMOOTH;

extern IntScheme_t   OPT__POT_INT_SCHEME, OPT__RHO_INT_SCHEME, OPT__GRA_INT_SCHEME, OPT__REF_POT_INT_SCHEME;
#endif


// (2-3) cosmology simulations
// ============================================================================================================
#ifdef COMOVING
extern double     A_INIT, OMEGA_M0, DT__MAX_DELTA_A;
#endif


// (2-4) load balance
// ============================================================================================================
#ifdef LOAD_BALANCE
extern real       LB_INPUT__WLI_MAX;                  // LB->WLI_Max loaded from "Input__Parameter"
#endif



// 3. CPU (host) arrays for transferring data bewteen CPU and GPU
// ============================================================================================================
extern real       (*h_Flu_Array_F_In [2])[FLU_NIN ][  FLU_NXT   *FLU_NXT   *FLU_NXT   ];
extern real       (*h_Flu_Array_F_Out[2])[FLU_NOUT][8*PATCH_SIZE*PATCH_SIZE*PATCH_SIZE];
extern real       (*h_Flux_Array[2])[9][NCOMP][4*PATCH_SIZE*PATCH_SIZE];
extern real       *h_MinDtInfo_Fluid_Array[2];

#ifdef GRAVITY
extern real       (*h_Rho_Array_P    [2])[RHO_NXT][RHO_NXT][RHO_NXT];
extern real       (*h_Pot_Array_P_In [2])[POT_NXT][POT_NXT][POT_NXT];
extern real       (*h_Pot_Array_P_Out[2])[GRA_NXT][GRA_NXT][GRA_NXT];
extern real       (*h_Flu_Array_G    [2])[GRA_NIN][PATCH_SIZE][PATCH_SIZE][PATCH_SIZE];
#endif



// 4. GPU (device) global memory arrays and timers
// ============================================================================================================
/*** These global variables are NOT included here. Instead, they are included by individual files 
     only if necessary. ***/



#endif // #ifndef __GLOBAL_H__
