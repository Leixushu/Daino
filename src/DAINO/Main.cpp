
#include "DAINO.h"
#include "CUFLU.h"


// ***********************
// **  GLOBAL VARIABLES **
// ***********************

// 1. common global variables
// =======================================================================================================
AMR_t            *patch     = NULL;
#ifdef OOC
AMR_t            *OOC_patch = NULL;
OOC_t             ooc;
#endif

double            Time[NLEVEL]           = { 0.0 };
uint              AdvanceCounter[NLEVEL] = { 0 }; 
long              Step                   = 0;
int               DumpID                 = 0;
double            DumpTime               = 0.0;

double            dTime_Base;
double            Time_Prev            [NLEVEL];
real              MinDtInfo_Fluid      [NLEVEL];
double            FlagTable_Rho        [NLEVEL-1]; 
double            FlagTable_RhoGradient[NLEVEL-1]; 
double            FlagTable_Lohner     [NLEVEL-1][3];
double            FlagTable_User       [NLEVEL-1];
double           *DumpTable = NULL;
int               DumpTable_NDump;

int               MPI_Rank, MPI_Rank_X[3], MPI_SibRank[26], NX0[3], NPatchTotal[NLEVEL];
int              *BaseP = NULL; 
int               Flu_ParaBuf;

double            BOX_SIZE, DT__FLUID, END_T, OUTPUT_DT;
long              END_STEP;
int               NX0_TOT[3], OUTPUT_STEP, REGRID_COUNT, FLU_GPU_NPGROUP, OMP_NTHREAD;
int               MPI_NRank, MPI_NRank_X[3], GPU_NSTREAM, FLAG_BUFFER_SIZE, MAX_LEVEL;

IntScheme_t       OPT__FLU_INT_SCHEME, OPT__REF_FLU_INT_SCHEME;
int               OPT__UM_START_LEVEL, OPT__UM_START_NVAR, OPT__GPUID_SELECT, OPT__PATCH_COUNT;
int               OPT__OUTPUT_TOTAL, OPT__CK_CONSERVATION, INIT_DUMPID, OPT__FLAG_LOHNER;
real              OPT__CK_MEMFREE, OUTPUT_PART_X, OUTPUT_PART_Y, OUTPUT_PART_Z;
bool              OPT__FLAG_RHO, OPT__FLAG_RHO_GRADIENT, OPT__FLAG_USER;
bool              OPT__DT_USER, OPT__RECORD_DT, OPT__RECORD_MEMORY, OPT__ADAPTIVE_DT;
bool              OPT__FIXUP_RESTRICT, OPT__INIT_RESTRICT, OPT__VERBOSE;
bool              OPT__INT_TIME, OPT__OUTPUT_ERROR, OPT__OUTPUT_BASE, OPT__OVERLAP_MPI, OPT__TIMING_BARRIER;
bool              OPT__OUTPUT_BASEPS, OPT__CK_REFINE, OPT__CK_PROPER_NESTING, OPT__CK_FINITE;
bool              OPT__CK_RESTRICT, OPT__CK_PATCH_ALLOCATE, OPT__FIXUP_FLUX, OPT__CK_FLUX_ALLOCATE;
OptInit_t         OPT__INIT;
OptRestartH_t     OPT__RESTART_HEADER;
OptOutputMode_t   OPT__OUTPUT_MODE;
OptOutputPart_t   OPT__OUTPUT_PART;


// 2. global variables for different applications
// =======================================================================================================
// (2-1) fluid solver in different models
#if   ( MODEL == HYDRO )
double         FlagTable_PresGradient[NLEVEL-1];
real           GAMMA, MINMOD_COEFF, EP_COEFF;
LR_Limiter_t   OPT__LR_LIMITER;
WAF_Limiter_t  OPT__WAF_LIMITER;
bool           OPT__FLAG_PRES_GRADIENT;
int            OPT__CK_NEGATIVE;

#elif ( MODEL == MHD )
#warning : WAIT MHD !!!

#elif ( MODEL == ELBDM )
double         DT__PHASE, FlagTable_EngyDensity[NLEVEL-1][2];
bool           OPT__FLAG_ENGY_DENSITY, OPT__INT_PHASE;
real           ELBDM_MASS, PLANCK_CONST, ETA;
real           MinDtInfo_Phase[NLEVEL];

#else
#error : unsupported MODEL !!
#endif // MODEL

// (2-2) self-gravity
#ifdef GRAVITY
double         AveDensity = -1.0;   // initialize it < 0 to check if it is properly set later
real           MinDtInfo_Gravity[NLEVEL];
int            Pot_ParaBuf, Rho_ParaBuf;

double         DT__GRAVITY;
real           NEWTON_G;
int            POT_GPU_NPGROUP;
IntScheme_t    OPT__POT_INT_SCHEME, OPT__RHO_INT_SCHEME, OPT__GRA_INT_SCHEME, OPT__REF_POT_INT_SCHEME;
bool           OPT__OUTPUT_POT, OPT__GRA_P5_GRADIENT;
real           SOR_OMEGA;
int            SOR_MAX_ITER, SOR_MIN_ITER;
real           MG_TOLERATED_ERROR;
int            MG_MAX_ITER, MG_NPRE_SMOOTH, MG_NPOST_SMOOTH;
#endif

// (2-3) cosmological simulations
#ifdef COMOVING
double         A_INIT, OMEGA_M0, DT__MAX_DELTA_A;
#endif

// (2-4) load balance
#ifdef LOAD_BALANCE
real           LB_INPUT__WLI_MAX;
#endif


// 3. CPU (host) arrays for transferring data bewteen CPU and GPU
// =======================================================================================================
// (3-1) fluid solver
real (*h_Flu_Array_F_In [2])[FLU_NIN ][  FLU_NXT   *FLU_NXT   *FLU_NXT   ] = { NULL, NULL };
real (*h_Flu_Array_F_Out[2])[FLU_NOUT][8*PATCH_SIZE*PATCH_SIZE*PATCH_SIZE] = { NULL, NULL };   
real (*h_Flux_Array[2])[9][NCOMP][4*PATCH_SIZE*PATCH_SIZE]                 = { NULL, NULL };
real *h_MinDtInfo_Fluid_Array[2]                                           = { NULL, NULL };

// (3-2) gravity solver
#ifdef GRAVITY
real (*h_Rho_Array_P    [2])[RHO_NXT][RHO_NXT][RHO_NXT]                    = { NULL, NULL };
real (*h_Pot_Array_P_In [2])[POT_NXT][POT_NXT][POT_NXT]                    = { NULL, NULL };                   
real (*h_Pot_Array_P_Out[2])[GRA_NXT][GRA_NXT][GRA_NXT]                    = { NULL, NULL };
real (*h_Flu_Array_G    [2])[GRA_NIN][PATCH_SIZE][PATCH_SIZE][PATCH_SIZE]  = { NULL, NULL };
#endif


// 4. GPU (device) global memory arrays
// =======================================================================================================
#ifdef GPU
// (4-1) fluid solver
real (*d_Flu_Array_F_In )[FLU_NIN ][ FLU_NXT*FLU_NXT*FLU_NXT ]    = NULL;
real (*d_Flu_Array_F_Out)[FLU_NOUT][ PS2*PS2*PS2 ]                = NULL;
real (*d_Flux_Array)[9][5][ PS2*PS2 ]                             = NULL;
real  *d_MinDtInfo_Fluid_Array                                    = NULL;
#if ( MODEL == HYDRO )
#if ( FLU_SCHEME == MHM  ||  FLU_SCHEME == MHM_RP  ||  FLU_SCHEME == CTU )
real (*d_PriVar)     [5][ FLU_NXT*FLU_NXT*FLU_NXT ]               = NULL;
real (*d_Slope_PPM_x)[5][ N_SLOPE_PPM*N_SLOPE_PPM*N_SLOPE_PPM ]   = NULL;
real (*d_Slope_PPM_y)[5][ N_SLOPE_PPM*N_SLOPE_PPM*N_SLOPE_PPM ]   = NULL;
real (*d_Slope_PPM_z)[5][ N_SLOPE_PPM*N_SLOPE_PPM*N_SLOPE_PPM ]   = NULL;
real (*d_FC_Var_xL)  [5][ N_FC_VAR*N_FC_VAR*N_FC_VAR ]            = NULL;
real (*d_FC_Var_xR)  [5][ N_FC_VAR*N_FC_VAR*N_FC_VAR ]            = NULL;
real (*d_FC_Var_yL)  [5][ N_FC_VAR*N_FC_VAR*N_FC_VAR ]            = NULL;
real (*d_FC_Var_yR)  [5][ N_FC_VAR*N_FC_VAR*N_FC_VAR ]            = NULL;
real (*d_FC_Var_zL)  [5][ N_FC_VAR*N_FC_VAR*N_FC_VAR ]            = NULL;
real (*d_FC_Var_zR)  [5][ N_FC_VAR*N_FC_VAR*N_FC_VAR ]            = NULL;
real (*d_FC_Flux_x)  [5][ N_FC_FLUX*N_FC_FLUX*N_FC_FLUX ]         = NULL;
real (*d_FC_Flux_y)  [5][ N_FC_FLUX*N_FC_FLUX*N_FC_FLUX ]         = NULL;
real (*d_FC_Flux_z)  [5][ N_FC_FLUX*N_FC_FLUX*N_FC_FLUX ]         = NULL;
#endif // FLU_SCHEME
#elif ( MODEL == MHD )
#warning : WAIT MHD !!!
#endif // MODEL

// (4-2) gravity solver
#ifdef GRAVITY
real (*d_Rho_Array_P    )[ RHO_NXT*RHO_NXT*RHO_NXT ]              = NULL;
real (*d_Pot_Array_P_In )[ POT_NXT*POT_NXT*POT_NXT ]              = NULL;
real (*d_Pot_Array_P_Out)[ GRA_NXT*GRA_NXT*GRA_NXT ]              = NULL;
real (*d_Flu_Array_G    )[GRA_NIN][ PS1*PS1*PS1 ]                 = NULL;
#endif

// (4-3) CUDA stream (put in CUAPI_MemAllocate_Fluid.cu)
//cudaStream_t *Stream                                              = NULL;
#endif // #ifdef GPU


// 5. timers
// =======================================================================================================
Timer_t *Timer_Main[6];
Timer_t *Timer_Flu_Advance [NLEVEL];
Timer_t *Timer_Gra_Advance [NLEVEL];
Timer_t *Timer_FixUp       [NLEVEL];
Timer_t *Timer_Flag        [NLEVEL];
Timer_t *Timer_Refine      [NLEVEL];
Timer_t *Timer_GetBuf      [NLEVEL][6];

#ifdef INDIVIDUAL_TIMESTEP
Timer_t *Timer_Total       [NLEVEL];
#else
Timer_t *Timer_Flu_Total   [NLEVEL];
Timer_t *Timer_Gra_Restrict[NLEVEL];
#endif

#ifdef TIMING_SOLVER
Timer_t *Timer_Pre         [NLEVEL][4];
Timer_t *Timer_Sol         [NLEVEL][4];
Timer_t *Timer_Clo         [NLEVEL][4];
Timer_t *Timer_Poi_PreRho  [NLEVEL];
Timer_t *Timer_Poi_PreFlu  [NLEVEL];
Timer_t *Timer_Poi_PrePot_C[NLEVEL];
Timer_t *Timer_Poi_PrePot_F[NLEVEL];
#endif

#if ( defined OOC  &&  defined INDIVIDUAL_TIMESTEP )
Timer_t *Timer_OOC_Flu_Total [NLEVEL];
Timer_t *Timer_OOC_Flu_Load  [NLEVEL][2];
Timer_t *Timer_OOC_Flu_Dump  [NLEVEL][2];
Timer_t *Timer_OOC_Flu_GetBuf[NLEVEL][2];
Timer_t *Timer_OOC_Flu_MPIBuf[NLEVEL][2];
Timer_t *Timer_OOC_Flu_UpdBuf[NLEVEL];

Timer_t *Timer_OOC_Gra_Total  [NLEVEL];
Timer_t *Timer_OOC_Gra_Load   [NLEVEL][2];
Timer_t *Timer_OOC_Gra_Dump   [NLEVEL];
Timer_t *Timer_OOC_Gra_GetBuf [NLEVEL][2];
Timer_t *Timer_OOC_Gra_MPIBuf [NLEVEL][2];
Timer_t *Timer_OOC_Gra_UpdBuf [NLEVEL];

Timer_t *Timer_OOC_Ref_Total  [NLEVEL][2];
Timer_t *Timer_OOC_Ref_Load   [NLEVEL][5];
Timer_t *Timer_OOC_Ref_Dump   [NLEVEL][4];
Timer_t *Timer_OOC_Ref_GetBuf [NLEVEL][4];
Timer_t *Timer_OOC_Ref_MPIBuf [NLEVEL][3];
Timer_t *Timer_OOC_Ref_UpdBuf [NLEVEL][2];
Timer_t *Timer_OOC_Ref_GetFlag[NLEVEL];
Timer_t *Timer_OOC_Ref_MPIFlag[NLEVEL];
#endif // #if ( defined OOC  &&  defined INDIVIDUAL_TIMESTEP )




//-------------------------------------------------------------------------------------------------------
// Function    :  main
// Description :  DAINO main function 
//-------------------------------------------------------------------------------------------------------
int main( int argc, char *argv[] )
{

   Init_DAINO( &argc, &argv );

   Aux_TakeNote();

#  ifdef GPU
   CUAPI_DiagnoseDevice();
#  endif

   Output_DumpData( 0 );

   if ( OPT__PATCH_COUNT > 0 )   Aux_PatchCount();
   if ( OPT__RECORD_MEMORY   )   Aux_GetMemInfo();

   Aux_Check();

#  ifdef TIMING
   Aux_ResetTimer();
#  endif



// main loop
// ======================================================================================================
   MPI_Barrier( MPI_COMM_WORLD );
   Timer_t  Timer_Total( 1 );
   Timer_Total.Start();


   while ( (Time[0]-END_T < -1.e-10)  &&  (Step < END_STEP) )
   {

#     ifdef TIMING
      MPI_Barrier( MPI_COMM_WORLD );
      Timer_Main[0]->Start();    // timer for one iteration
#     endif


//    a. determine the time step
//    ---------------------------------------------------------------------------------------------------
      TIMING_FUNC(   Mis_GetTimeStep(),   Timer_Main[1],   false   );

      if ( MPI_Rank == 0  &&  Step%1 == 0 )   
         Aux_Message( stdout, "Time : %13.7e -> %13.7e,    Step : %7ld -> %7ld,    dt = %14.7e\n", 
                      Time[0], Time[0]+dTime_Base, Step, Step+1, dTime_Base );
//    ---------------------------------------------------------------------------------------------------


//    b. advance all physical attributes by one step
//    ---------------------------------------------------------------------------------------------------
#     ifdef INDIVIDUAL_TIMESTEP
#        ifdef OOC
         TIMING_FUNC(   OOC_Integration_IndiviTimeStep( 0, dTime_Base ),   Timer_Main[2],   false   );
#        else
         TIMING_FUNC(       Integration_IndiviTimeStep( 0, dTime_Base ),   Timer_Main[2],   false   );
#        endif
#     else
         TIMING_FUNC(       Integration_SharedTimeStep( dTime_Base ),      Timer_Main[2],   false   );
#     endif

      Step ++;
//    ---------------------------------------------------------------------------------------------------


//    c. restrict all data and re-calculate potential in the debug mode (in order to check the RESTART process)
//    ---------------------------------------------------------------------------------------------------
#     ifdef DAINO_DEBUG
      for (int lv=NLEVEL-2; lv>=0; lv--)
      {
         Flu_Restrict( lv, patch->FluSg[lv+1], patch->FluSg[lv], NULL_INT, NULL_INT, _FLU );

#        ifdef LOAD_BALANCE 
         LB_GetBufferData( lv, patch->FluSg[lv], NULL_INT, DATA_RESTRICT, _FLU, NULL_INT );
#        endif

         Buf_GetBufferData( lv, patch->FluSg[lv], NULL_INT, DATA_GENERAL, _FLU, Flu_ParaBuf, USELB_YES );
      }

#     ifdef GRAVITY
      for (int lv=0; lv<NLEVEL; lv++)
      {
#        ifdef COMOVING
         const real Poi_Coeff = 4.0*M_PI*NEWTON_G*Time[lv];
#        else
         const real Poi_Coeff = 4.0*M_PI*NEWTON_G;
#        endif

         if ( lv == 0 )    
            CPU_PoissonSolver_FFT( Poi_Coeff, patch->PotSg[lv] );

         else              
         {
            Buf_GetBufferData( lv, patch->FluSg[lv], NULL_INT, DATA_GENERAL, _DENS, Rho_ParaBuf, USELB_YES );

            InvokeSolver( POISSON_SOLVER, lv, Time[lv], NULL_REAL, Poi_Coeff, patch->PotSg[lv], false, false );
         }

         Buf_GetBufferData( lv, NULL_INT, patch->PotSg[lv], POT_FOR_POISSON, _POTE, Pot_ParaBuf, USELB_YES );
      }
#     endif // #ifdef GRAVITY
#     endif // #ifdef DAINO_DEBUG


//    d. output data and execute auxiliary functions
//    ---------------------------------------------------------------------------------------------------
      TIMING_FUNC(   Output_DumpData( 1 ),   Timer_Main[3],   false   );

      if ( OPT__PATCH_COUNT == 1  ||  OPT__PATCH_COUNT == 2 )     
      TIMING_FUNC(   Aux_PatchCount(),     Timer_Main[4],   false   );

      if ( OPT__RECORD_MEMORY )   
      TIMING_FUNC(   Aux_GetMemInfo(),     Timer_Main[4],   false   );

      TIMING_FUNC(   Aux_Check(),          Timer_Main[4],   false   );
//    ---------------------------------------------------------------------------------------------------


//    e. check whether to manually terminate the run 
//    ---------------------------------------------------------------------------------------------------
      int Terminate = false;
      TIMING_FUNC(   End_StopManually( Terminate ),   Timer_Main[4],   false   );
//    ---------------------------------------------------------------------------------------------------


//    f. check whether to redistribute all patches for LOAD_BALANCE
//    ---------------------------------------------------------------------------------------------------
#     ifdef LOAD_BALANCE
      const bool DuringRestart_No = false;

      if ( patch->LB->WLI > patch->LB->WLI_Max )
      {
         if ( MPI_Rank == 0 )    
         {
            Aux_Message( stdout, "Weighted load-imbalance factor (%13.7e) > threshold (%13.7e) ", 
                         patch->LB->WLI, patch->LB->WLI_Max );
            Aux_Message( stdout, "--> redistributing all patches ...\n" );
         }

         TIMING_FUNC(   patch->LB->reset(),                      Timer_Main[5],   false   );
         TIMING_FUNC(   LB_Init_LoadBalance( DuringRestart_No ), Timer_Main[5],   false   );
         TIMING_FUNC(   Aux_PatchCount(),                        Timer_Main[5],   false   );
      }
#     endif
//    ---------------------------------------------------------------------------------------------------


//    g. record timing
//    ---------------------------------------------------------------------------------------------------
#     ifdef TIMING
      MPI_Barrier( MPI_COMM_WORLD );
      Timer_Main[0]->Stop( false ); 

      Aux_RecordTiming();

      Aux_ResetTimer();
#     endif
//    ---------------------------------------------------------------------------------------------------


      if ( Terminate )  break;

   } // while ( (Time[0]-END_T < -1.e-10)  &&  (Step < END_STEP) )


   MPI_Barrier( MPI_COMM_WORLD );
   Timer_Total.Stop( false );
// ======================================================================================================



// output the final result
   Output_DumpData( 2 );


// record the total simulation time
   if ( MPI_Rank == 0 )
   {
#     ifdef TIMING
      FILE *File = fopen( "Record__Timing", "a" );
      fprintf( File, "\n" );
      fprintf( File, "Total Processing Time : %f s\n", Timer_Total.GetValue( 0 ) );
      fprintf( File, "\n" );
      fclose( File );
#     endif

      FILE *Note = fopen( "Record__Note", "a" );
      fprintf( Note, "\n" );
      fprintf( Note, "Total Processing Time : %f s\n", Timer_Total.GetValue( 0 ) );
      fprintf( Note, "\n" );
      fclose( Note );
   }


   End_DAINO();
   return 0;

} // FUNCTION : Main

