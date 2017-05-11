
#include "DAINO.h"
#include "CUFLU.h"

#if (  !defined GPU  &&  MODEL == HYDRO  &&  ( FLU_SCHEME == MHM || FLU_SCHEME == MHM_RP )  )



extern void CPU_DataReconstruction( const real PriVar[][5], real FC_Var[][6][5], const int NIn, const int NGhost,
                                    const real Gamma, const LR_Limiter_t LR_Limiter, const real MinMod_Coeff, 
                                    const real EP_Coeff, const real dt, const real dh );
extern void CPU_Con2Flux( const int XYZ, real Flux[], const real Input[], const real Gamma );
extern void CPU_Con2Pri( const real In[], real Out[], const real  Gamma_m1 );
extern void CPU_Pri2Con( const real In[], real Out[], const real _Gamma_m1 );
extern void CPU_ComputeFlux( const real FC_Var[][6][5], real FC_Flux[][3][5], const int NFlux, const int Gap,
                             const real Gamma );
extern void CPU_FullStepUpdate( const real Input[][ FLU_NXT*FLU_NXT*FLU_NXT ], real Output[][ PS2*PS2*PS2 ], 
                                const real Flux[][3][5], const real dt, const real dh, 
                                const real Gamma );
extern void CPU_StoreFlux( real Flux_Array[][5][ PS2*PS2 ], const real FC_Flux[][3][5] );
#if   ( RSOLVER == EXACT )
extern void CPU_RiemannSolver_Exact( const int XYZ, real eival_out[], real L_star_out[], real R_star_out[], 
                                     real Flux_Out[], const real L_In[], const real R_In[], const real Gamma ); 
#elif ( RSOLVER == ROE )
extern void CPU_RiemannSolver_Roe( const int XYZ, real Flux_Out[], const real L_In[], const real R_In[], 
                                   const real Gamma );
#elif ( RSOLVER == HLLE )
extern void CPU_RiemannSolver_HLLE( const int XYZ, real Flux_Out[], const real L_In[], const real R_In[], 
                                    const real Gamma );
#elif ( RSOLVER == HLLC )
extern void CPU_RiemannSolver_HLLC( const int XYZ, real Flux_Out[], const real L_In[], const real R_In[], 
                                    const real Gamma );
#endif

#if   ( FLU_SCHEME == MHM_RP )
static void CPU_RiemannPredict( const real Flu_Array_In[][ FLU_NXT*FLU_NXT*FLU_NXT ], 
                                const real Half_Flux[][3][5], real Half_Var[][5], const real dt, 
                                const real dh, const real Gamma );
static void CPU_RiemannPredict_Flux( const real Flu_Array_In[][ FLU_NXT*FLU_NXT*FLU_NXT ], 
                                     real Half_Flux[][3][5], const real Gamma );
#elif ( FLU_SCHEME == MHM )
static void CPU_HancockPredict( real FC_Var[][6][5], const real dt, const real dh, const real Gamma,
                                const real C_Var[][ FLU_NXT*FLU_NXT*FLU_NXT ] );
#endif




//-------------------------------------------------------------------------------------------------------
// Function    :  CPU_FluidSolver_MHM
// Description :  CPU fluid solver based on the MUSCL-Hancock scheme
//
// Note        :  1. The three-dimensional evolution is achieved by using the unsplit method
//                2. Two half-step prediction schemes are supported, including "MHM" and "MHM_RP"
//                   MHM    : use interpolated face-centered values to calculate the half-step fluxes
//                   MHM_RP : use Riemann solver to calculate the half-step fluxes
//                3. Ref : 
//                   MHM    : "Riemann Solvers and Numerical Methods for Fluid Dynamics 
//                             - A Practical Introduction ~ by Eleuterio F. Toro"
//                   MHM_RP : Stone & Gardiner, NewA, 14, 139 (2009)
//
// Parameter   :  Flu_Array_In   : Array storing the input fluid variables
//                Flu_Array_Out  : Array to store the output fluid variables
//                Flux_Array     : Array to store the output fluxes
//                NPatchGroup    : Number of patch groups to be evaluated
//                dt             : Time interval to advance solution
//                dh             : Grid size
//                Gamma          : Ratio of specific heats
//                StoreFlux      : true --> store the coarse-fine fluxes
//                LR_Limiter     : Slope limiter for the data reconstruction in the MHM/MHM_RP/CTU schemes
//                                 (0/1/2/3/4) = (vanLeer/generalized MinMod/vanAlbada/
//                                                vanLeer + generalized MinMod/extrema-preserving) limiter
//                MinMod_Coeff   : Coefficient of the generalized MinMod limiter
//                EP_Coeff       : Coefficient of the extrema-preserving limiter
//-------------------------------------------------------------------------------------------------------
void CPU_FluidSolver_MHM( const real Flu_Array_In[][5][ FLU_NXT*FLU_NXT*FLU_NXT ], 
                          real Flu_Array_Out[][5][ PS2*PS2*PS2 ], 
                          real Flux_Array[][9][5][ PS2*PS2 ], 
                          const int NPatchGroup, const real dt, const real dh, const real Gamma, 
                          const bool StoreFlux, const LR_Limiter_t LR_Limiter, const real MinMod_Coeff, 
                          const real EP_Coeff )
                              
{

// check
#  ifdef DAINO_DEBUG
   if ( LR_Limiter != VANLEER  &&  LR_Limiter != GMINMOD  &&  LR_Limiter != ALBADA  &&  LR_Limiter != EXTPRE  &&
        LR_Limiter != VL_GMINMOD )
      Aux_Error( ERROR_INFO, "unsupported reconstruction limiter (%d) !!\n", LR_Limiter );
#  endif


#  pragma omp parallel
   {
      const real  Gamma_m1 = Gamma - (real)1.0;
      const real _Gamma_m1 = (real)1.0 / Gamma_m1;

      real Input[5];
      int ID1;

//    FC: Face-Centered variables/fluxes
      real (*FC_Var )[6][5] = new real [ N_FC_VAR*N_FC_VAR*N_FC_VAR ][6][5];
      real (*FC_Flux)[3][5] = new real [ N_FC_FLUX*N_FC_FLUX*N_FC_FLUX ][3][5];     // also used by "Half_Flux"
      real (*PriVar)[5]     = new real [ FLU_NXT*FLU_NXT*FLU_NXT ][5];              // also used by "Half_Var"

#     if ( FLU_SCHEME == MHM_RP )
      real (*const Half_Flux)[3][5] = FC_Flux;
      real (*const Half_Var)[5]     = PriVar;
#     endif


//    loop over all patch groups
#     pragma omp for
      for (int P=0; P<NPatchGroup; P++)
      {

//       1. half-step prediction
#        if ( FLU_SCHEME == MHM_RP ) // a. use Riemann solver to calculate the half-step fluxes

//       (1.a-1) evaluate the half-step first-order fluxes by Riemann solver
         CPU_RiemannPredict_Flux( Flu_Array_In[P], Half_Flux, Gamma );


//       (1.a-2) evaluate the half-step solutions
         CPU_RiemannPredict( Flu_Array_In[P], Half_Flux, Half_Var, dt, dh, Gamma );


//       (1.a-3) conserved variables --> primitive variables
         for (int k=0; k<N_HF_VAR; k++)
         for (int j=0; j<N_HF_VAR; j++)
         for (int i=0; i<N_HF_VAR; i++)
         {
            ID1 = (k*N_HF_VAR + j)*N_HF_VAR + i;

            for (int v=0; v<5; v++)    Input[v] = Half_Var[ID1][v];

            CPU_Con2Pri( Input, Half_Var[ID1], Gamma_m1 );
         }


//       (1.a-4) evaluate the face-centered values by data reconstruction 
         CPU_DataReconstruction( Half_Var, FC_Var, N_HF_VAR, FLU_GHOST_SIZE-2, Gamma, LR_Limiter, 
                                 MinMod_Coeff, EP_Coeff, NULL_REAL, NULL_INT );


//       (1.a-5) primitive face-centered variables --> conserved face-centered variables
         for (int k=0; k<N_FC_VAR; k++)
         for (int j=0; j<N_FC_VAR; j++)
         for (int i=0; i<N_FC_VAR; i++)
         {
            ID1 = (k*N_FC_VAR + j)*N_FC_VAR + i;

            for (int f=0; f<6; f++)
            {
               for (int v=0; v<5; v++)    Input[v] = FC_Var[ID1][f][v];

               CPU_Pri2Con( Input, FC_Var[ID1][f], _Gamma_m1 );
            }
         }

#        elif ( FLU_SCHEME == MHM ) // b. use interpolated face-centered values to calculate the half-step fluxes

//       (1.b-1) conserved variables --> primitive variables
         for (int k=0; k<FLU_NXT; k++)
         for (int j=0; j<FLU_NXT; j++)
         for (int i=0; i<FLU_NXT; i++)
         {
            ID1 = (k*FLU_NXT + j)*FLU_NXT + i;

            for (int v=0; v<5; v++)    Input[v] = Flu_Array_In[P][v][ID1];

            CPU_Con2Pri( Input, PriVar[ID1], Gamma_m1 );
         }


//       (1.b-2) evaluate the face-centered values by data reconstruction 
         CPU_DataReconstruction( PriVar, FC_Var, FLU_NXT, FLU_GHOST_SIZE-1, Gamma, LR_Limiter, 
                                 MinMod_Coeff, EP_Coeff, NULL_REAL, NULL_INT );


//       (1.b-3) primitive face-centered variables --> conserved face-centered variables
         for (int k=0; k<N_FC_VAR; k++)
         for (int j=0; j<N_FC_VAR; j++)
         for (int i=0; i<N_FC_VAR; i++)
         {
            ID1 = (k*N_FC_VAR + j)*N_FC_VAR + i;

            for (int f=0; f<6; f++)
            {
               for (int v=0; v<5; v++)    Input[v] = FC_Var[ID1][f][v];

               CPU_Pri2Con( Input, FC_Var[ID1][f], _Gamma_m1 );
            }
         }


//       (1.b-4) evaluate the half-step solutions
         CPU_HancockPredict( FC_Var, dt, dh, Gamma, Flu_Array_In[P] ); 

#        endif // #if ( FLU_SCHEME == MHM_RP ) ... else ...


//       2. evaluate the full-step fluxes
         CPU_ComputeFlux( FC_Var, FC_Flux, N_FL_FLUX, 1, Gamma );


//       3. full-step evolution
         CPU_FullStepUpdate( Flu_Array_In[P], Flu_Array_Out[P], FC_Flux, dt, dh, Gamma );


//       4. store the inter-patch fluxes
         if ( StoreFlux )
         CPU_StoreFlux( Flux_Array[P], FC_Flux );

      } // for (int P=0; P<NPatchGroup; P++)


      delete [] FC_Var;
      delete [] FC_Flux;
      delete [] PriVar;

   } // OpenMP parallel region

} // FUNCTION : CPU_FluidSolver_MHM



#if ( FLU_SCHEME == MHM_RP )
//-------------------------------------------------------------------------------------------------------
// Function    :  CPU_RiemannPredict_Flux
// Description :  Evaluate the half-step face-centered fluxes by Riemann solver 
//
// Note        :  1. Work for the MUSCL-Hancock method + Riemann-prediction (MHM_RP)
//                2. Currently support the exact, Roe, HLLE, and HLLC solvers
//
// Parameter   :  Flu_Array_In   : Array storing the input conserved variables
//                Half_Flux      : Array to store the output face-centered fluxes
//                                 --> The size is assumed to be N_HF_FLUX^3
//                Gamma          : Ratio of specific heats
//-------------------------------------------------------------------------------------------------------
void CPU_RiemannPredict_Flux( const real Flu_Array_In[][ FLU_NXT*FLU_NXT*FLU_NXT ], real Half_Flux[][3][5], 
                              const real Gamma )
{

   const int dr[3] = { 1, FLU_NXT, FLU_NXT*FLU_NXT };
   int ID1, ID2, dN[3]={ 0 };
   real ConVar_L[5], ConVar_R[5];

#  if ( RSOLVER == EXACT )
   const real Gamma_m1 = Gamma - (real)1.0;
   real PriVar_L[5], PriVar_R[5];
#  endif


// loop over different spatial directions
   for (int d=0; d<3; d++)
   {
      switch ( d )
      {
         case 0 : dN[0] = 0;  dN[1] = 1;  dN[2] = 1;  break;
         case 1 : dN[0] = 1;  dN[1] = 0;  dN[2] = 1;  break;
         case 2 : dN[0] = 1;  dN[1] = 1;  dN[2] = 0;  break;
      }

      for (int k1=0, k2=dN[2];  k1<N_HF_FLUX-dN[2];  k1++, k2++)
      for (int j1=0, j2=dN[1];  j1<N_HF_FLUX-dN[1];  j1++, j2++)
      for (int i1=0, i2=dN[0];  i1<N_HF_FLUX-dN[0];  i1++, i2++)
      {
         ID1 = (k1*N_HF_FLUX + j1)*N_HF_FLUX + i1;
         ID2 = (k2*FLU_NXT   + j2)*FLU_NXT   + i2;

//       get the left and right states
         for (int v=0; v<5; v++) 
         {
            ConVar_L[v] = Flu_Array_In[v][ ID2       ];
            ConVar_R[v] = Flu_Array_In[v][ ID2+dr[d] ];
         }

//       invoke the Riemann solver
#        if   ( RSOLVER == EXACT )
         CPU_Con2Pri( ConVar_L, PriVar_L, Gamma_m1 );
         CPU_Con2Pri( ConVar_R, PriVar_R, Gamma_m1 );

         CPU_RiemannSolver_Exact( d, NULL, NULL, NULL, Half_Flux[ID1][d], PriVar_L, PriVar_R, Gamma );
#        elif ( RSOLVER == ROE )
         CPU_RiemannSolver_Roe ( d, Half_Flux[ID1][d], ConVar_L, ConVar_R, Gamma );
#        elif ( RSOLVER == HLLE )
         CPU_RiemannSolver_HLLE( d, Half_Flux[ID1][d], ConVar_L, ConVar_R, Gamma );
#        elif ( RSOLVER == HLLC )
         CPU_RiemannSolver_HLLC( d, Half_Flux[ID1][d], ConVar_L, ConVar_R, Gamma );
#        else
#        error : ERROR : unsupported Riemann solver (EXACT/ROE) !!
#        endif

      }
   } // for (int d=0; d<3; d++)

} // FUNCTION : CPU_RiemannPredict_Flux



//-------------------------------------------------------------------------------------------------------
// Function    :  CPU_RiemannPredict
// Description :  Evolve the cell-centered variables by half time-step by using the Riemann solvers
//
// Note        :  Work for the MUSCL-Hancock method + Riemann-prediction (MHM_RP)
//
// Parameter   :  Flu_Array_In   : Array storing the input conserved variables
//                Half_Flux      : Array storing the input face-centered fluxes
//                                 --> The size is assumed to be N_HF_FLUX^3
//                Half_Var       : Array to store the output conserved variables
//                                 --> The size is assumed to be N_HF_VAR^3
//                dt             : Time interval to advance solution
//                dh             : Grid size
//                Gamma          : Ratio of specific heats
//-------------------------------------------------------------------------------------------------------
void CPU_RiemannPredict( const real Flu_Array_In[][ FLU_NXT*FLU_NXT*FLU_NXT ], const real Half_Flux[][3][5], 
                         real Half_Var[][5], const real dt, const real dh, const real Gamma ) 
{

   const int  dID3[3] = { 1, N_HF_FLUX, N_HF_FLUX*N_HF_FLUX }; 
   const real dt_dh2  = (real)0.5*dt/dh;
   real dF[3][5];
   int ID1, ID2, ID3;

#  ifdef ENFORCE_POSITIVE
   const real  Gamma_m1 = Gamma - (real)1.0;
   const real _Gamma_m1 = (real)1.0 / Gamma_m1;
   real TempPres, Ek;
#  endif


   for (int k1=0, k2=1;  k1<N_HF_VAR;  k1++, k2++)
   for (int j1=0, j2=1;  j1<N_HF_VAR;  j1++, j2++)
   for (int i1=0, i2=1;  i1<N_HF_VAR;  i1++, i2++)
   {
      ID1 = (k1*N_HF_VAR  + j1)*N_HF_VAR  + i1;
      ID2 = (k2*FLU_NXT   + j2)*FLU_NXT   + i2;
      ID3 = (k1*N_HF_FLUX + j1)*N_HF_FLUX + i1;

      for (int d=0; d<3; d++)
      for (int v=0; v<5; v++)    dF[d][v] = Half_Flux[ ID3+dID3[d] ][d][v] - Half_Flux[ID3][d][v];

      for (int v=0; v<5; v++)
         Half_Var[ID1][v] = Flu_Array_In[v][ID2] - dt_dh2*( dF[0][v] + dF[1][v] + dF[2][v] );

#     ifdef ENFORCE_POSITIVE
      Ek                = (real)0.5*( Half_Var[ID1][1]*Half_Var[ID1][1] + Half_Var[ID1][2]*Half_Var[ID1][2] + 
                                      Half_Var[ID1][3]*Half_Var[ID1][3] ) / Half_Var[ID1][0];
      TempPres          = Gamma_m1*( Half_Var[ID1][4] - Ek );
      TempPres          = FMAX( TempPres, MIN_VALUE );
      Half_Var[ID1][4] = Ek + _Gamma_m1*TempPres;
#     endif
   }

} // FUNCTION : CPU_RiemannPredict
#endif // #if ( FLU_SCHEME == MHM_RP )



#if ( FLU_SCHEME == MHM )
//-------------------------------------------------------------------------------------------------------
// Function    :  CPU_HancockPredict
// Description :  Evolve the face-centered variables by half time-step by calculating the face-centered fluxes
//                (no Riemann solver is required)
//
// Note        :  1. Work for the MHM scheme
//                2. Do NOT require data in the neighboring cells
//
// Parameter   :  FC_Var   : Face-centered conserved variables
//                           --> The size is assumed to be N_FC_VAR^3
//                dt       : Time interval to advance solution
//                dh       : Grid size
//                Gamma    : Ratio of specific heats
//                C_Var    : Array storing conservative variables 
//                           --> For the "ENFORCE_POSITIVE" operation
//-------------------------------------------------------------------------------------------------------
void CPU_HancockPredict( real FC_Var[][6][5], const real dt, const real dh, const real Gamma,
                         const real C_Var[][ FLU_NXT*FLU_NXT*FLU_NXT ] ) 
{

   const real dt_dh2  = (real)0.5*dt/dh;
   const int   NGhost = FLU_GHOST_SIZE - 1;
   real Flux[6][5], dFlux[5];
   int ID1;

#  ifdef ENFORCE_POSITIVE
   const real  Gamma_m1 = Gamma - (real)1.0;
   const real _Gamma_m1 = (real)1.0 / Gamma_m1;
   real TempPres, Ek;
   int  ID2;
#  endif


   for (int k1=0, k2=NGhost;  k1<N_FC_VAR;  k1++, k2++)
   for (int j1=0, j2=NGhost;  j1<N_FC_VAR;  j1++, j2++)
   for (int i1=0, i2=NGhost;  i1<N_FC_VAR;  i1++, i2++)
   {
      ID1 = (k1*N_FC_VAR + j1)*N_FC_VAR + i1;
#     ifdef ENFORCE_POSITIVE
      ID2 = (k2*FLU_NXT  + j2)*FLU_NXT  + i2;
#     endif

      for (int f=0; f<6; f++)    CPU_Con2Flux( f/2, Flux[f], FC_Var[ID1][f], Gamma );

      for (int v=0; v<5; v++)
      {
         dFlux[v] = dt_dh2 * ( Flux[1][v] - Flux[0][v] + Flux[3][v] - Flux[2][v] + Flux[5][v] - Flux[4][v] );

         for (int f=0; f<6; f++)    FC_Var[ID1][f][v] -= dFlux[v];
      }

#     ifdef ENFORCE_POSITIVE
//    check the negative pressure      
      for (int f=0; f<6; f++)
      {
         Ek               = (real)0.5*(  FC_Var[ID1][f][1]*FC_Var[ID1][f][1] + FC_Var[ID1][f][2]*FC_Var[ID1][f][2]
                                       + FC_Var[ID1][f][3]*FC_Var[ID1][f][3]  ) / FC_Var[ID1][f][0];
         TempPres         = Gamma_m1*( FC_Var[ID1][f][4] - Ek );
         TempPres         = FMAX( TempPres, MIN_VALUE );
         FC_Var[ID1][f][4] = Ek + _Gamma_m1*TempPres;
      }

//    check the negative density
      for (int f=0; f<6; f++)
      {
         if ( FC_Var[ID1][f][0] <= (real)0.0 )
         {
//          set to the values before update
            for (int v=0; v<5; v++)    
            {
               FC_Var[ID1][0][v] = FC_Var[ID1][1][v] = FC_Var[ID1][2][v] = FC_Var[ID1][3][v] =
               FC_Var[ID1][4][v] = FC_Var[ID1][5][v] = C_Var[v][ID2]; 
            }

            break;
         }
      }
#     endif
   } // i,j,k

} // FUNCTION : CPU_HancockPredict
#endif // #if ( FLU_SCHEME == MHM )



#endif // #if (  !defined GPU  &&  MODEL == HYDRO  &&  ( FLU_SCHEME == MHM || FLU_SCHEME == MHM_RP )  )
