
#include "DAINO.h"
#include "CUFLU.h"
#ifdef GRAVITY
#include "CUPOT.h"
#endif




//-------------------------------------------------------------------------------------------------------
// Function    :  Aux_TakeNote
// Description :  Record simulation parameters and the content in the file "Input__NoteScript" to the 
//                note file "Record__Note"
//-------------------------------------------------------------------------------------------------------
void Aux_TakeNote()
{

   if ( MPI_Rank == 0 )    Aux_Message( stdout, "Aux_TakeNote ... \n" ); 


   const char FileName[] = "Record__Note";
   FILE *Note;

   if ( MPI_Rank == 0 )    
   {
      FILE *File_Check = fopen( FileName, "r" );
      if ( File_Check != NULL )  
      {
         Aux_Message( stderr, "WARNING : the file \"%s\" already exists !!\n", FileName );
         fclose( File_Check );
      }
      
   
//    copy the content in the file "Input__NoteScript"
      Note = fopen( FileName, "a" );
      fprintf( Note, "\n\n\nSimulation Note\n" );
      fprintf( Note, "***********************************************************************************\n" );
      fclose( Note );
   
      system( "cat ./Input__NoteScript >> Record__Note" );
   
      Note = fopen( FileName, "a" );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "\n\n");
   
   
//    record the simulation options in the Makefile (numerical schemes)
      fprintf( Note, "Makefile Options (numerical schemes)\n" );
      fprintf( Note, "***********************************************************************************\n" );

//    a. options for all models
#     if   ( MODEL == HYDRO )
      fprintf( Note, "MODEL                     HYDRO\n" );
#     elif ( MODEL == MHD )
      fprintf( Note, "MODEL                     MHD\n" );
#     elif ( MODEL == ELBDM )
      fprintf( Note, "MODEL                     ELBDM\n" );
#     else
      fprintf( Note, "MODEL                     UNKNOWN\n" );
#     endif // MODEL

#     ifdef GRAVITY
      fprintf( Note, "GRAVITY                   ON\n" );
#     else
      fprintf( Note, "GRAVITY                   OFF\n" );
#     endif

#     ifdef GRAVITY
#     if   ( POT_SCHEME == SOR )
      fprintf( Note, "POT_SCHEME                SOR\n" );
#     elif ( POT_SCHEME == MG )
      fprintf( Note, "POT_SCHEME                MG\n" );
#     elif ( POT_SCHEME == NONE )
      fprintf( Note, "POT_SCHEME                NONE\n" );
#     else
      fprintf( Note, "POT_SCHEME                UNKNOWN\n" );
#     endif
#     endif // #ifdef GRAVITY
   
#     ifdef INDIVIDUAL_TIMESTEP
      fprintf( Note, "INDIVIDUAL_TIMESTEP       ON\n" );
#     else
      fprintf( Note, "INDIVIDUAL_TIMESTEP       OFF\n" );
#     endif
   
#     ifdef COMOVING
      fprintf( Note, "COMOVING                  ON\n" );
#     else
      fprintf( Note, "COMOVING                  OFF\n" );
#     endif

//    b. options in HYDRO
#     if   ( MODEL == HYDRO )

#     if   ( FLU_SCHEME == RTVD )
      fprintf( Note, "FLU_SCHEME                RTVD\n" );
#     elif ( FLU_SCHEME == MHM )
      fprintf( Note, "FLU_SCHEME                MHM\n" );
#     elif ( FLU_SCHEME == MHM_RP )
      fprintf( Note, "FLU_SCHEME                MHM with Riemann prediction\n" );
#     elif ( FLU_SCHEME == CTU )
      fprintf( Note, "FLU_SCHEME                CTU\n" );
#     elif ( FLU_SCHEME == WAF )
      fprintf( Note, "FLU_SCHEME                WAF\n" );
#     elif ( FLU_SCHEME == NONE )
      fprintf( Note, "FLU_SCHEME                NONE\n" );
#     else
      fprintf( Note, "FLU_SCHEME                UNKNOWN\n" );
#     endif

#     if   ( LR_SCHEME == PLM )
      fprintf( Note, "LR_SCHEME                 PLM\n" );
#     elif ( LR_SCHEME == PPM )            
      fprintf( Note, "LR_SCHEME                 PPM\n" );
#     elif ( LR_SCHEME == NONE )            
      fprintf( Note, "LR_SCHEME                 NONE\n" );
#     else
      fprintf( Note, "LR_SCHEME                 UNKNOWN\n" );
#     endif

#     if   ( RSOLVER == EXACT )
      fprintf( Note, "RSOLVER                   EXACT\n" );
#     elif ( RSOLVER == ROE )            
      fprintf( Note, "RSOLVER                   ROE\n" );
#     elif ( RSOLVER == HLLE )            
      fprintf( Note, "RSOLVER                   HLLE\n" );
#     elif ( RSOLVER == HLLC )            
      fprintf( Note, "RSOLVER                   HLLC\n" );
#     elif ( RSOLVER == NONE )            
      fprintf( Note, "RSOLVER                   NONE\n" );
#     else
      fprintf( Note, "RSOLVER                   UNKNOWN\n" );
#     endif

//    c. options in MHD 
#     elif ( MODEL == MHD )
#     warning : WAIT MHD !!!

//    d. options in ELBDM
#     elif ( MODEL == ELBDM )

#     else
#     error : ERROR : unsupported MODEL !!
#     endif // MODEL
   
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "\n\n");


//    record the simulation options in the Makefile (optimization and compilation)
      fprintf( Note, "Makefile Options (optimization and compilation)\n" );
      fprintf( Note, "***********************************************************************************\n" );

#     ifdef GPU
      fprintf( Note, "GPU                       ON\n" );
#     else
      fprintf( Note, "GPU                       OFF\n" );
#     endif
   
#     ifdef DAINO_OPTIMIZATION
      fprintf( Note, "DAINO_OPTIMIZATION        ON\n" );
#     else
      fprintf( Note, "DAINO_OPTIMIZATION        OFF\n" );
#     endif
   
#     ifdef DAINO_DEBUG
      fprintf( Note, "DAINO_DEBUG               ON\n" );
#     else
      fprintf( Note, "DAINO_DEBUG               OFF\n" );
#     endif

#     ifdef TIMING
      fprintf( Note, "TIMING                    ON\n" );
#     else
      fprintf( Note, "TIMING                    OFF\n" );
#     endif
   
#     ifdef TIMING_SOLVER
      fprintf( Note, "TIMING_SOLVER             ON\n" );
#     else
      fprintf( Note, "TIMING_SOLVER             OFF\n" );
#     endif
   
#     ifdef INTEL
      fprintf( Note, "Compiler                  Intel\n" );
#     else
      fprintf( Note, "Compiler                  GNU\n" );
#     endif

#     ifdef FLOAT8
      fprintf( Note, "FLOAT8                    ON\n" );
#     else
      fprintf( Note, "FLOAT8                    OFF\n" );
#     endif
   
#     ifdef SERIAL
      fprintf( Note, "SERIAL                    ON\n" );
#     else
      fprintf( Note, "SERIAL                    OFF\n" );
#     endif
   
#     ifdef OOC
      fprintf( Note, "OOC                       ON\n" );
#     else
      fprintf( Note, "OOC                       OFF\n" );
#     endif

#     if   ( LOAD_BALANCE == HILBERT )
      fprintf( Note, "LOAD_BALANCE              HILBERT\n" );
#     elif ( LOAD_BALANCE == NONE )
      fprintf( Note, "LOAD_BALANCE              OFF\n" );
#     else
      fprintf( Note, "LOAD_BALANCE              UNKOWN\n" );
#     endif

#     ifdef OVERLAP_MPI
      fprintf( Note, "OVERLAP_MPI               ON\n" );
#     else
      fprintf( Note, "OVERLAP_MPI               OFF\n" );
#     endif
   
#     ifdef OPENMP
      fprintf( Note, "OPENMP                    ON\n" );
#     else
      fprintf( Note, "OPENMP                    OFF\n" );
#     endif

#     ifdef FERMI
      fprintf( Note, "FERMI                     ON\n" );
#     else
      fprintf( Note, "FERMI                     OFF\n" );
#     endif
   
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "\n\n");
   

//    record the simulation options in CUFLU.h and CUPOT.h
      fprintf( Note, "Other Options (in CUFLU.h and CUPOT.h)\n" );
      fprintf( Note, "***********************************************************************************\n" );

#     if   ( MODEL == HYDRO )
#     ifdef ENFORCE_POSITIVE
      fprintf( Note, "ENFORCE_POSITIVE          ON\n" );
#     else
      fprintf( Note, "ENFORCE_POSITIVE          OFF\n" );
#     endif

#     ifdef CHAR_RECONSTRUCTION
      fprintf( Note, "CHAR_RECONSTRUCTION       ON\n" );
#     else
      fprintf( Note, "CHAR_RECONSTRUCTION       OFF\n" );
#     endif

#     if   ( CHECK_INTERMEDIATE == EXACT )
      fprintf( Note, "CHECK_INTERMEDIATE        EXACT\n" );
#     elif ( CHECK_INTERMEDIATE == HLLE )
      fprintf( Note, "CHECK_INTERMEDIATE        HLLE\n" );
#     elif ( CHECK_INTERMEDIATE == HLLC )
      fprintf( Note, "CHECK_INTERMEDIATE        HLLC\n" );
#     elif ( CHECK_INTERMEDIATE == NONE )
      fprintf( Note, "CHECK_INTERMEDIATE        OFF\n" );
#     else
      fprintf( Note, "CHECK_INTERMEDIATE        UNKNOWN\n" );
#     endif

#     ifdef HLL_NO_REF_STATE
      fprintf( Note, "HLL_NO_REF_STATE          ON\n" );
#     else
      fprintf( Note, "HLL_NO_REF_STATE          OFF\n" );
#     endif

#     ifdef HLL_INCLUDE_ALL_WAVES
      fprintf( Note, "HLL_INCLUDE_ALL_WAVES     ON\n" );
#     else
      fprintf( Note, "HLL_INCLUDE_ALL_WAVES     OFF\n" );
#     endif

#     ifdef WAF_DISSIPATE
      fprintf( Note, "WAF_DISSIPATE             ON\n" );
#     else
      fprintf( Note, "WAF_DISSIPATE             OFF\n" );
#     endif

#     elif ( MODEL == MHD )
#     warning : WAIT MHD !!!

#     elif ( MODEL == ELBDM )

#     else
#     error : ERROR : unsupported MODEL !!
#     endif // MODEL

#     if ( defined GRAVITY  &&  POT_SCHEME == SOR )
#     ifdef USE_PSOLVER_10TO14
      fprintf( Note, "USE_PSOLVER_10TO14        ON\n"  );
#     else
      fprintf( Note, "USE_PSOLVER_10TO14        OFF\n" );
#     endif
#     endif // #ifdef GRAVITY

      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "\n\n");

   
//    record the symbolic constants
      fprintf( Note, "Symbolic Constants\n" );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "#define NCOMP             %d\n",      NCOMP                   );
      fprintf( Note, "#define FLU_NIN           %d\n",      FLU_NIN                 );
      fprintf( Note, "#define FLU_NOUT          %d\n",      FLU_NOUT                );
#     ifdef GRAVITY
      fprintf( Note, "#define GRA_NIN           %d\n",      GRA_NIN                 );
#     endif
      fprintf( Note, "#define PATCH_SIZE        %d\n",      PATCH_SIZE              );
      fprintf( Note, "#define MAX_PATCH         %d\n",      MAX_PATCH               );
      fprintf( Note, "#define NLEVEL            %d\n",      NLEVEL                  );
      fprintf( Note, "\n" );
      fprintf( Note, "#define FLU_GHOST_SIZE    %d\n",      FLU_GHOST_SIZE          );
#     ifdef GRAVITY
      fprintf( Note, "#define POT_GHOST_SIZE    %d\n",      POT_GHOST_SIZE          );
      fprintf( Note, "#define RHO_GHOST_SIZE    %d\n",      RHO_GHOST_SIZE          );
      fprintf( Note, "#define GRA_GHOST_SIZE    %d\n",      GRA_GHOST_SIZE          );
#     endif
      fprintf( Note, "#define FLU_NXT           %d\n",      FLU_NXT                 );
#     ifdef GRAVITY
      fprintf( Note, "#define POT_NXT           %d\n",      POT_NXT                 );
      fprintf( Note, "#define RHO_NXT           %d\n",      RHO_NXT                 );
      fprintf( Note, "#define GRA_NXT           %d\n",      GRA_NXT                 );
#     endif
      fprintf( Note, "#define FLU_BLOCK_SIZE_X  %d\n",      FLU_BLOCK_SIZE_X        );
      fprintf( Note, "#define FLU_BLOCK_SIZE_Y  %d\n",      FLU_BLOCK_SIZE_Y        );
#     ifdef GRAVITY
#     if   ( POT_SCHEME == SOR )
      fprintf( Note, "#define POT_BLOCK_SIZE_Z  %d\n",      POT_BLOCK_SIZE_Z        );
#     elif ( POT_SCHEME == MG )
      fprintf( Note, "#define POT_BLOCK_SIZE_X  %d\n",      POT_BLOCK_SIZE_X        );
#     endif
      fprintf( Note, "#define GRA_BLOCK_SIZE_Z  %d\n",      GRA_BLOCK_SIZE_Z        );
#     endif // #ifdef GRAVITY
      fprintf( Note, "#define MIN_VALUE         %13.7e\n",  MIN_VALUE               );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "\n\n");
   
   
//    record the symbolic constants of Out-of-core computing
#     ifdef OOC
      fprintf( Note, "Symbolic Constants of Out-of-core Computing\n" );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "#define NDISK             %d\n",      NDISK                   );
      fprintf( Note, "#define DISKOFFSET        %lu\n",     DISKOFFSET              );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "\n\n");
#     endif
   
   
//    record the parameters of simulation scale
      fprintf( Note, "Parameters of Simulation Scale\n" );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "BOX_SIZE (input)          %13.7e\n",  BOX_SIZE                );
      fprintf( Note, "BOX_SIZE_X                %13.7e\n",  patch->BoxSize[0]       );
      fprintf( Note, "BOX_SIZE_Y                %13.7e\n",  patch->BoxSize[1]       );
      fprintf( Note, "BOX_SIZE_Z                %13.7e\n",  patch->BoxSize[2]       );
      fprintf( Note, "BOX_SCALE_X               %d\n",      patch->BoxScale[0]      );
      fprintf( Note, "BOX_SCALE_Y               %d\n",      patch->BoxScale[1]      );
      fprintf( Note, "BOX_SCALE_Z               %d\n",      patch->BoxScale[2]      );
      fprintf( Note, "NX0_TOT[0]                %d\n",      NX0_TOT[0]              );
      fprintf( Note, "NX0_TOT[1]                %d\n",      NX0_TOT[1]              );
      fprintf( Note, "NX0_TOT[2]                %d\n",      NX0_TOT[2]              );
      fprintf( Note, "MPI_NRank                 %d\n",      MPI_NRank               );
      fprintf( Note, "MPI_NRank_X[0]            %d\n",      MPI_NRank_X[0]          );
      fprintf( Note, "MPI_NRank_X[1]            %d\n",      MPI_NRank_X[1]          );
      fprintf( Note, "MPI_NRank_X[2]            %d\n",      MPI_NRank_X[2]          );
      fprintf( Note, "OMP_NTHREAD               %d\n",      OMP_NTHREAD             );
      fprintf( Note, "END_T                     %13.7e\n",  END_T                   );
      fprintf( Note, "END_STEP                  %ld\n",     END_STEP                );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "\n\n");
   
   
//    record the parameters of out-of-core computing
#     ifdef OOC
      fprintf( Note, "Parameters of Out-of-core Computing\n" );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "ooc.NRank                 %d\n",      ooc.NRank               );
      fprintf( Note, "ooc.NRank_X[0]            %d\n",      ooc.NRank_X[0]          );
      fprintf( Note, "ooc.NRank_X[1]            %d\n",      ooc.NRank_X[1]          );
      fprintf( Note, "ooc.NRank_X[2]            %d\n",      ooc.NRank_X[2]          );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "\n\n");
#     endif
   
   
//    record the parameters of cosmological simulations (comoving frame)
#     ifdef COMOVING
      fprintf( Note, "Parameters of Cosmological Simulation\n" );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "A_INIT                    %13.7e\n",  A_INIT                  );
      fprintf( Note, "OMEGA_M0                  %13.7e\n",  OMEGA_M0                );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "\n\n");
#     endif
   
   
//    record the parameters of time-step determination
      fprintf( Note, "Parameters of Time-step Determination\n" );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "DT__FLUID                 %13.7e\n",  DT__FLUID               );
#     ifdef GRAVITY
      fprintf( Note, "DT__GRAVITY               %13.7e\n",  DT__GRAVITY             );
#     endif
#     if ( MODEL == ELBDM )
      fprintf( Note, "DT__PHASE                 %13.7e\n",  DT__PHASE               );
#     endif
#     ifdef COMOVING
      fprintf( Note, "DT__MAX_DELTA_A           %13.7e\n",  DT__MAX_DELTA_A         );
#     endif
      fprintf( Note, "OPT__ADAPTIVE_DT          %d\n",      OPT__ADAPTIVE_DT        ); 
      fprintf( Note, "OPT__RECORD_DT            %d\n",      OPT__RECORD_DT          );
      fprintf( Note, "OPT__DT_USER              %d\n",      OPT__DT_USER            );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "\n\n");
   
   
//    record the parameters of domain refinement
      fprintf( Note, "Parameters of Domain Refinement\n" );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "REGRID_COUNT              %d\n",      REGRID_COUNT            );
      fprintf( Note, "FLAG_BUFFER_SIZE          %d\n",      FLAG_BUFFER_SIZE        );
      fprintf( Note, "MAX_LEVEL                 %d\n",      MAX_LEVEL               );
      fprintf( Note, "OPT__FLAG_RHO             %d\n",      OPT__FLAG_RHO           );
      fprintf( Note, "OPT__FLAG_RHO_GRADIENT    %d\n",      OPT__FLAG_RHO_GRADIENT  );
#     if   ( MODEL == HYDRO )
      fprintf( Note, "OPT__FLAG_PRES_GRADIENT   %d\n",      OPT__FLAG_PRES_GRADIENT );
#     elif ( MODEL == MHD )
#     warning : WAIT MHD !!!
#     endif
#     if ( MODEL == ELBDM )
      fprintf( Note, "OPT__FLAG_ENGY_DENSITY    %d\n",      OPT__FLAG_ENGY_DENSITY  );
#     endif
      fprintf( Note, "OPT__FLAG_LOHNER          %d\n",      OPT__FLAG_LOHNER        );
      fprintf( Note, "OPT__FLAG_USER            %d\n",      OPT__FLAG_USER          );
      fprintf( Note, "OPT__PATCH_COUNT          %d\n",      OPT__PATCH_COUNT        );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "\n\n");
   
   
//    record the parameters of parallelization
#     ifndef SERIAL
      fprintf( Note, "Parameters of Parallelization\n" );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "Flu_ParaBuf               %d\n",      Flu_ParaBuf             );     
#     ifdef GRAVITY
      fprintf( Note, "Pot_ParaBuf               %d\n",      Pot_ParaBuf             );     
      fprintf( Note, "Rho_ParaBuf               %d\n",      Rho_ParaBuf             );     
#     endif
#     ifdef LOAD_BALANCE
      fprintf( Note, "LB_WLI_MAX                %13.7e\n",  patch->LB->WLI_Max      );     
#     endif
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "\n\n");
#     endif // #ifndef SERIAL


//    record the parameters of Fluid solver in different models
      fprintf( Note, "Parameters of Fluid Solver (in different models)\n" );
      fprintf( Note, "***********************************************************************************\n" );
#     if   ( MODEL == HYDRO )
      fprintf( Note, "GAMMA                     %13.7e\n",  GAMMA                   );
      fprintf( Note, "MINMOD_COEFF              %13.7e\n",  MINMOD_COEFF            );
      fprintf( Note, "EP_COEFF                  %13.7e\n",  EP_COEFF                );
      fprintf( Note, "OPT__LR_LIMITER           %s\n",      ( OPT__LR_LIMITER == VANLEER    ) ? "VANLEER"    :
                                                            ( OPT__LR_LIMITER == GMINMOD    ) ? "GMINMOD"    :
                                                            ( OPT__LR_LIMITER == ALBADA     ) ? "ALBADA"     :
                                                            ( OPT__LR_LIMITER == VL_GMINMOD ) ? "VL_GMINMOD" :
                                                            ( OPT__LR_LIMITER == EXTPRE     ) ? "EXTPRE"     :
                                                            ( OPT__LR_LIMITER == LR_LIMITER_NONE ) ? "NONE"  :
                                                                                                "UNKNOWN" );
      fprintf( Note, "OPT__WAF_LIMITER          %s\n",      ( OPT__WAF_LIMITER == WAF_SUPERBEE ) ? "WAF_SUPERBEE":
                                                            ( OPT__WAF_LIMITER == WAF_VANLEER  ) ? "WAF_VANLEER" :
                                                            ( OPT__WAF_LIMITER == WAF_ALBADA   ) ? "WAF_ALBADA"  :
                                                            ( OPT__WAF_LIMITER == WAF_MINBEE   ) ? "WAF_MINBEE"  :
                                                            ( OPT__WAF_LIMITER == WAF_LIMITER_NONE ) ? "NONE"    :
                                                                                                   "UNKNOWN" );
#     elif ( MODEL == MHD )
#     warning : WAIT MHD !!!

#     elif ( MODEL == ELBDM )
      fprintf( Note, "ELBDM_MASS                %13.7e\n",  ELBDM_MASS              );
      fprintf( Note, "PLANCK_CONST              %13.7e\n",  PLANCK_CONST            );
      fprintf( Note, "ETA                       %13.7e\n",  ETA                     );

#     else
#     error : ERROR : unsupported MODEL !!
#     endif // MODEL
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "\n\n");


//    record the parameters of Fluid solver in different models
      fprintf( Note, "Parameters of Fluid Solver (in all models)\n" );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "FLU_GPU_NPGROUP           %d\n",      FLU_GPU_NPGROUP         );
      fprintf( Note, "GPU_NSTREAM               %d\n",      GPU_NSTREAM             );
      fprintf( Note, "OPT__FIXUP_FLUX           %d\n",      OPT__FIXUP_FLUX         );
      fprintf( Note, "OPT__FIXUP_RESTRICT       %d\n",      OPT__FIXUP_RESTRICT     );
      fprintf( Note, "OPT__OVERLAP_MPI          %d\n",      OPT__OVERLAP_MPI        );     
      fprintf( Note, "WITH_COARSE_FINE_FLUX     %d\n",      patch->WithFlux         );
#     ifndef SERIAL
      int MPI_Thread_Status;
      MPI_Query_thread( &MPI_Thread_Status );
      fprintf( Note, "MPI Thread Level          " );
      switch ( MPI_Thread_Status )
      {
         case MPI_THREAD_SINGLE:       fprintf( Note, "MPI_THREAD_SINGLE\n" );       break;
         case MPI_THREAD_FUNNELED:     fprintf( Note, "MPI_THREAD_FUNNELED\n" );     break;
         case MPI_THREAD_SERIALIZED:   fprintf( Note, "MPI_THREAD_SERIALIZED\n" );   break;
         case MPI_THREAD_MULTIPLE:     fprintf( Note, "MPI_THREAD_MULTIPLE\n" );     break;

         default:                      fprintf( Note, "UNKNOWN\n" );
      }
#     endif
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "\n\n");
   
   
//    record the parameters of Poisson and Gravity solvers
#     ifdef GRAVITY
      fprintf( Note, "Parameters of Poisson and Gravity Solvers\n" );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "NEWTON_G                  %13.7e\n",  NEWTON_G                );
#     if   ( POT_SCHEME == SOR )
      fprintf( Note, "SOR_OMEGA                 %13.7e\n",  SOR_OMEGA               );
      fprintf( Note, "SOR_MAX_ITER              %d\n",      SOR_MAX_ITER            );
      fprintf( Note, "SOR_MIN_ITER              %d\n",      SOR_MIN_ITER            );
#     elif ( POT_SCHEME == MG )
      fprintf( Note, "MG_MAX_ITER               %d\n",      MG_MAX_ITER             );
      fprintf( Note, "MG_NPRE_SMOOTH            %d\n",      MG_NPRE_SMOOTH          );
      fprintf( Note, "MG_NPOST_SMOOTH           %d\n",      MG_NPOST_SMOOTH         );
      fprintf( Note, "MG_TOLERATED_ERROR        %13.7e\n",  MG_TOLERATED_ERROR      );
#     endif
      fprintf( Note, "POT_GPU_NPGROUP           %d\n",      POT_GPU_NPGROUP         );
      fprintf( Note, "OPT__GRA_P5_GRADIENT      %d\n",      OPT__GRA_P5_GRADIENT    );
      fprintf( Note, "Average Density           %13.7e\n",  AveDensity              );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "\n\n");
#     endif // #ifdef GRAVITY
   
   
//    record the parameters of initialization
      fprintf( Note, "Parameters of Initialization\n" );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "OPT__INIT                 %d\n",      OPT__INIT               );
      fprintf( Note, "OPT__RESTART_HEADER       %d\n",      OPT__RESTART_HEADER     );
      fprintf( Note, "OPT__UM_START_LEVEL       %d\n",      OPT__UM_START_LEVEL     );
      fprintf( Note, "OPT__UM_START_NVAR        %d\n",      OPT__UM_START_NVAR      );
      fprintf( Note, "OPT__INIT_RESTRICT        %d\n",      OPT__INIT_RESTRICT      );
      fprintf( Note, "OPT__GPUID_SELECT         %d\n",      OPT__GPUID_SELECT       );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "\n\n");
   
   
//    record the parameters of interpolation schemes
      fprintf( Note, "Parameters of Interpolation Schemes\n" );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "OPT__INT_TIME             %d\n",      OPT__INT_TIME           ); 
#     if ( MODEL == ELBDM )
      fprintf( Note, "OPT__INT_PHASE            %d\n",      OPT__INT_PHASE          ); 
#     endif
      fprintf( Note, "OPT__FLU_INT_SCHEME       %s\n",      ( OPT__FLU_INT_SCHEME == INT_CENTRAL ) ? "CENTRAL" :
                                                            ( OPT__FLU_INT_SCHEME == INT_MINMOD  ) ? "MINMOD"  :
                                                            ( OPT__FLU_INT_SCHEME == INT_VANLEER ) ? "VANLEER" :
                                                            ( OPT__FLU_INT_SCHEME == INT_CQUAD   ) ? "CQUAD"   :
                                                            ( OPT__FLU_INT_SCHEME == INT_QUAD    ) ? "QUAD"    :
                                                            ( OPT__FLU_INT_SCHEME == INT_CQUAR   ) ? "CQUAR"   :
                                                            ( OPT__FLU_INT_SCHEME == INT_QUAR    ) ? "QUAR"    :
                                                                                                     "UNKNOWN" );
#     ifdef GRAVITY
      fprintf( Note, "OPT__POT_INT_SCHEME       %s\n",      ( OPT__POT_INT_SCHEME == INT_CENTRAL ) ? "CENTRAL" :
                                                            ( OPT__POT_INT_SCHEME == INT_MINMOD  ) ? "MINMOD"  :
                                                            ( OPT__POT_INT_SCHEME == INT_VANLEER ) ? "VANLEER" :
                                                            ( OPT__POT_INT_SCHEME == INT_CQUAD   ) ? "CQUAD"   :
                                                            ( OPT__POT_INT_SCHEME == INT_QUAD    ) ? "QUAD"    :
                                                            ( OPT__POT_INT_SCHEME == INT_CQUAR   ) ? "CQUAR"   :
                                                            ( OPT__POT_INT_SCHEME == INT_QUAR    ) ? "QUAR"    :
                                                                                                     "UNKNOWN" );
      fprintf( Note, "OPT__RHO_INT_SCHEME       %s\n",      ( OPT__RHO_INT_SCHEME == INT_CENTRAL ) ? "CENTRAL" :
                                                            ( OPT__RHO_INT_SCHEME == INT_MINMOD  ) ? "MINMOD"  :
                                                            ( OPT__RHO_INT_SCHEME == INT_VANLEER ) ? "VANLEER" :
                                                            ( OPT__RHO_INT_SCHEME == INT_CQUAD   ) ? "CQUAD"   :
                                                            ( OPT__RHO_INT_SCHEME == INT_QUAD    ) ? "QUAD"    :
                                                            ( OPT__RHO_INT_SCHEME == INT_CQUAR   ) ? "CQUAR"   :
                                                            ( OPT__RHO_INT_SCHEME == INT_QUAR    ) ? "QUAR"    :
                                                                                                     "UNKNOWN" );
      fprintf( Note, "OPT__GRA_INT_SCHEME       %s\n",      ( OPT__GRA_INT_SCHEME == INT_CENTRAL ) ? "CENTRAL" :
                                                            ( OPT__GRA_INT_SCHEME == INT_MINMOD  ) ? "MINMOD"  :
                                                            ( OPT__GRA_INT_SCHEME == INT_VANLEER ) ? "VANLEER" :
                                                            ( OPT__GRA_INT_SCHEME == INT_CQUAD   ) ? "CQUAD"   :
                                                            ( OPT__GRA_INT_SCHEME == INT_QUAD    ) ? "QUAD"    :
                                                            ( OPT__GRA_INT_SCHEME == INT_CQUAR   ) ? "CQUAR"   :
                                                            ( OPT__GRA_INT_SCHEME == INT_QUAR    ) ? "QUAR"    :
                                                                                                     "UNKNOWN" );
#     endif
      fprintf( Note, "OPT__REF_FLU_INT_SCHEME   %s\n",   ( OPT__REF_FLU_INT_SCHEME == INT_CENTRAL ) ? "CENTRAL" :
                                                         ( OPT__REF_FLU_INT_SCHEME == INT_MINMOD  ) ? "MINMOD"  :
                                                         ( OPT__REF_FLU_INT_SCHEME == INT_VANLEER ) ? "VANLEER" :
                                                         ( OPT__REF_FLU_INT_SCHEME == INT_CQUAD   ) ? "CQUAD"   :
                                                         ( OPT__REF_FLU_INT_SCHEME == INT_QUAD    ) ? "QUAD"    :
                                                         ( OPT__REF_FLU_INT_SCHEME == INT_CQUAR   ) ? "CQUAR"   :
                                                         ( OPT__REF_FLU_INT_SCHEME == INT_QUAR    ) ? "QUAR"    :
                                                                                                      "UNKNOWN" );
#     ifdef GRAVITY
      fprintf( Note, "OPT__REF_POT_INT_SCHEME   %s\n",   ( OPT__REF_POT_INT_SCHEME == INT_CENTRAL ) ? "CENTRAL" :
                                                         ( OPT__REF_POT_INT_SCHEME == INT_MINMOD  ) ? "MINMOD"  :
                                                         ( OPT__REF_POT_INT_SCHEME == INT_VANLEER ) ? "VANLEER" :
                                                         ( OPT__REF_POT_INT_SCHEME == INT_CQUAD   ) ? "CQUAD"   :
                                                         ( OPT__REF_POT_INT_SCHEME == INT_QUAD    ) ? "QUAD"    :
                                                         ( OPT__REF_POT_INT_SCHEME == INT_CQUAR   ) ? "CQUAR"   :
                                                         ( OPT__REF_POT_INT_SCHEME == INT_QUAR    ) ? "QUAR"    :
                                                                                                      "UNKNOWN" );
#     endif
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "\n\n");
   
   
//    record the parameters of data dump
      fprintf( Note, "Parameters of Data Dump\n" );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "OPT__OUTPUT_TOTAL         %d\n",      OPT__OUTPUT_TOTAL       );
      fprintf( Note, "OPT__OUTPUT_PART          %d\n",      OPT__OUTPUT_PART        );
      fprintf( Note, "OPT__OUTPUT_ERROR         %d\n",      OPT__OUTPUT_ERROR       );
      fprintf( Note, "OPT__OUTPUT_BASEPS        %d\n",      OPT__OUTPUT_BASEPS      );
      fprintf( Note, "OPT__OUTPUT_BASE          %d\n",      OPT__OUTPUT_BASE        );
#     ifdef GRAVITY
      fprintf( Note, "OPT__OUTPUT_POT           %d\n",      OPT__OUTPUT_POT         );
#     endif
      fprintf( Note, "OPT__OUTPUT_MODE          %d\n",      OPT__OUTPUT_MODE        );
      fprintf( Note, "OUTPUT_STEP               %d\n",      OUTPUT_STEP             );
      fprintf( Note, "OUTPUT_DT                 %13.7e\n",  OUTPUT_DT               );
      fprintf( Note, "OUTPUT_PART_X             %13.7e\n",  OUTPUT_PART_X           );
      fprintf( Note, "OUTPUT_PART_Y             %13.7e\n",  OUTPUT_PART_Y           );
      fprintf( Note, "OUTPUT_PART_Z             %13.7e\n",  OUTPUT_PART_Z           );
      fprintf( Note, "INIT_DUMPID               %d\n",      INIT_DUMPID             );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "\n\n");


//    record the parameters of miscellaneous purposes
      fprintf( Note, "Parameters of Miscellaneous Purposes\n" );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "OPT__VERBOSE              %d\n",      OPT__VERBOSE            );
      fprintf( Note, "OPT__TIMING_BARRIER       %d\n",      OPT__TIMING_BARRIER     );
      fprintf( Note, "OPT__RECORD_MEMORY        %d\n",      OPT__RECORD_MEMORY      );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "\n\n");
   
   
//    record the parameters of simulation checks
      fprintf( Note, "Parameters of Simulation Checks\n" );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "OPT__CK_REFINE            %d\n",      OPT__CK_REFINE          );
      fprintf( Note, "OPT__CK_PROPER_NESTING    %d\n",      OPT__CK_PROPER_NESTING  );
      fprintf( Note, "OPT__CK_CONSERVATION      %d\n",      OPT__CK_CONSERVATION    );
      fprintf( Note, "OPT__CK_RESTRICT          %d\n",      OPT__CK_RESTRICT        );
      fprintf( Note, "OPT__CK_FINITE            %d\n",      OPT__CK_FINITE          );
      fprintf( Note, "OPT__CK_PATCH_ALLOCATE    %d\n",      OPT__CK_PATCH_ALLOCATE  );
      fprintf( Note, "OPT__CK_FLUX_ALLOCATE     %d\n",      OPT__CK_FLUX_ALLOCATE   );
#     if   ( MODEL == HYDRO )
      fprintf( Note, "OPT__CK_NEGATIVE          %d\n",      OPT__CK_NEGATIVE        );
#     elif ( MODEL == MHD )
#     warning : WAIT MHD !!!
#     endif // MODEL
      fprintf( Note, "OPT__CK_MEMFREE           %13.7e\n",  OPT__CK_MEMFREE         );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "\n\n");
   
   
//    record the flag criterion (density/density gradient/pressure gradient/user-defined)
      if ( OPT__FLAG_RHO )   
      {
         fprintf( Note, "Flag Criterion (Density)\n" );
         fprintf( Note, "***********************************************************************************\n" );
         fprintf( Note, "  Level             Density\n" );
         for (int lv=0; lv<NLEVEL-1; lv++)   fprintf( Note, "%7d%20.5lf\n", lv, FlagTable_Rho[lv] );
         fprintf( Note, "***********************************************************************************\n" );
         fprintf( Note, "\n\n");
      }

      if ( OPT__FLAG_RHO_GRADIENT )   
      {
         fprintf( Note, "Flag Criterion (Density Gradient)\n" );
         fprintf( Note, "***********************************************************************************\n" );
         fprintf( Note, "  Level    Density Gradient\n" );
         for (int lv=0; lv<NLEVEL-1; lv++)   fprintf( Note, "%7d%20.5lf\n", lv, FlagTable_RhoGradient[lv] );
         fprintf( Note, "***********************************************************************************\n" );
         fprintf( Note, "\n\n");
      }

#     if   ( MODEL == HYDRO )
      if ( OPT__FLAG_PRES_GRADIENT )   
      {
         fprintf( Note, "Flag Criterion (Pressure Gradient in HYDRO)\n" );
         fprintf( Note, "***********************************************************************************\n" );
         fprintf( Note, "  Level   Pressure Gradient\n" );
         for (int lv=0; lv<NLEVEL-1; lv++)   fprintf( Note, "%7d%20.5lf\n", lv, FlagTable_PresGradient[lv] );
         fprintf( Note, "***********************************************************************************\n" );
         fprintf( Note, "\n\n");
      }
#     elif ( MODEL == MHD )
#     warning : WAIT MHD !!!
#     endif

#     if ( MODEL == ELBDM )
      if ( OPT__FLAG_ENGY_DENSITY )   
      {
         fprintf( Note, "Flag Criterion (Energy Density in ELBDM)\n" );
         fprintf( Note, "***********************************************************************************\n" );
         fprintf( Note, "  Level     Angle_over_2*PI              Soften\n" );
         for (int lv=0; lv<NLEVEL-1; lv++)   
            fprintf( Note, "%7d%20.5lf%20.5lf\n", lv, FlagTable_EngyDensity[lv][0], FlagTable_EngyDensity[lv][1] );
         fprintf( Note, "***********************************************************************************\n" );
         fprintf( Note, "\n\n");
      }
#     endif

      if ( OPT__FLAG_LOHNER )   
      {
         fprintf( Note, "Flag Criterion (Lohner error estimator)\n" );
         fprintf( Note, "***********************************************************************************\n" );
         fprintf( Note, "  Level           Threshold              Filter              Soften\n" );
         for (int lv=0; lv<NLEVEL-1; lv++)   
            fprintf( Note, "%7d%20.5lf%20.5lf%20.5lf\n", lv, FlagTable_Lohner[lv][0], FlagTable_Lohner[lv][1],
                     FlagTable_Lohner[lv][2] );
         fprintf( Note, "***********************************************************************************\n" );
         fprintf( Note, "\n\n");
      }

      if ( OPT__FLAG_USER )   
      {
         fprintf( Note, "Flag Criterion (User-defined)\n" );
         fprintf( Note, "***********************************************************************************\n" );
         fprintf( Note, "  Level           Threshold\n" );
         for (int lv=0; lv<NLEVEL-1; lv++)   fprintf( Note, "%7d%20.5lf\n", lv, FlagTable_User[lv] );
         fprintf( Note, "***********************************************************************************\n" );
         fprintf( Note, "\n\n");
      }
   
   
//    record the grid size in different refinement level
      fprintf( Note, "Cell Size and Scale (scale = number of cells at the finest level)\n" );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "%7s%*c%26s%*c%16s\n", "Level", 10, ' ', "Cell Size", 10, ' ', "Cell Scale" );
      for (int lv=0; lv<NLEVEL; lv++)  
      fprintf( Note, "%7d%*c%26.20f%*c%16d\n", lv, 10, ' ', patch->dh[lv], 10, ' ', patch->scale[lv] );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "\n\n");


//    record the compilation time (of the file "Aux_TakeNote")
      fprintf( Note, "Compilation Time\n" );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "%s %s\n", __DATE__, __TIME__ );
      fprintf( Note, "***********************************************************************************\n" );
      fprintf( Note, "\n\n");
   
   
      fclose( Note );
   } // if ( MPI_Rank == 0 )


// record the hostname and PID of each MPI process (the function "CUAPI_DiagnoseDevice" will also record them)
#  ifndef GPU
   const int PID = getpid();
   char Host[1024];
   gethostname( Host, 1024 );

   if ( MPI_Rank == 0 )
   {
       Note = fopen( FileName, "a" );
       fprintf( Note, "Device Diagnosis\n" );
       fprintf( Note, "***********************************************************************************\n" );
       fclose( Note );
   }
   
   for (int YourTurn=0; YourTurn<MPI_NRank; YourTurn++)
   {
      if ( MPI_Rank == YourTurn )
      {
         Note = fopen( FileName, "a" );
         fprintf( Note, "MPI_Rank = %3d, hostname = %10s, PID = %5d\n", MPI_Rank, Host, PID );
         fprintf( Note, "CPU Info :\n" );
         fflush( Note );

         Aux_GetCPUInfo( FileName );

         fprintf( Note, "\n" );
         fclose( Note );
      }

      MPI_Barrier( MPI_COMM_WORLD );
   }

   if ( MPI_Rank == 0 )
   {
      Note = fopen( FileName, "a" );
      fprintf( Note, "***********************************************************************************\n" );
      fclose( Note );
   }
#  endif // #ifndef GPU


   if ( MPI_Rank == 0 )    Aux_Message( stdout, "Aux_TakeNote ... done\n" ); 

} // FUNCTION : Aux_TakeNote
