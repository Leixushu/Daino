
#include "DAINO.h"
#include "CUFLU.h"

#if ( !defined GPU  &&  MODEL == HYDRO  &&  FLU_SCHEME == CTU )



extern void CPU_DataReconstruction( const real PriVar[][5], real FC_Var[][6][5], const int NIn, const int NGhost,
                                    const real Gamma, const LR_Limiter_t LR_Limiter, const real MinMod_Coeff, 
                                    const real EP_Coeff, const real dt, const real dh );
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
#endif

static void TGradient_Correction( real FC_Var[][6][5], const real FC_Flux[][3][5], const real dt, const real dh );




//-------------------------------------------------------------------------------------------------------
// Function    :  CPU_FluidSolver_CTU
// Description :  CPU fluid solver based on the Corner-Transport-Upwind (CTU) scheme
//
// Note        :  Ref : Stone et al., ApJS, 178, 137 (2008)
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
void CPU_FluidSolver_CTU( const real Flu_Array_In[][5][ FLU_NXT*FLU_NXT*FLU_NXT ], 
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
      real (*FC_Flux)[3][5] = new real [ N_FC_FLUX*N_FC_FLUX*N_FC_FLUX ][3][5];
      real (*PriVar)[5]     = new real [ FLU_NXT*FLU_NXT*FLU_NXT ][5];


//    loop over all patch groups
#     pragma omp for
      for (int P=0; P<NPatchGroup; P++)
      {

//       1. conserved variables --> primitive variables
         for (int k=0; k<FLU_NXT; k++)
         for (int j=0; j<FLU_NXT; j++)
         for (int i=0; i<FLU_NXT; i++)
         {
            ID1 = (k*FLU_NXT + j)*FLU_NXT + i;

            for (int v=0; v<5; v++)    Input[v] = Flu_Array_In[P][v][ID1];

            CPU_Con2Pri( Input, PriVar[ID1], Gamma_m1 );
         }


//       2. evaluate the face-centered values at the half time-step
         CPU_DataReconstruction( PriVar, FC_Var, FLU_NXT, FLU_GHOST_SIZE-1, Gamma, LR_Limiter, 
                                 MinMod_Coeff, EP_Coeff, dt, dh );


//       3. primitive face-centered variables --> conserved face-centered variables
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


//       4. evaluate the face-centered half-step fluxes by solving the Riemann problem
         CPU_ComputeFlux( FC_Var, FC_Flux, N_HF_FLUX, 0, Gamma );


//       5. correct the face-centered variables by the transverse flux gradients
         TGradient_Correction( FC_Var, FC_Flux, dt, dh );


//       6. evaluate the face-centered full-step fluxes by solving the Riemann problem with the corrected data
         CPU_ComputeFlux( FC_Var, FC_Flux, N_FL_FLUX, 1, Gamma );


//       7. full-step evolution
         CPU_FullStepUpdate( Flu_Array_In[P], Flu_Array_Out[P], FC_Flux, dt, dh, Gamma );


//       8. store the inter-patch fluxes
         if ( StoreFlux )
         CPU_StoreFlux( Flux_Array[P], FC_Flux );

      } // for (int P=0; P<NPatchGroup; P++)


      delete [] FC_Var;
      delete [] FC_Flux;
      delete [] PriVar;

   } // OpenMP parallel region

} // FUNCTION : CPU_FluidSolver_CTU



//-------------------------------------------------------------------------------------------------------
// Function    :  TGradient_Correction
// Description :  1. Correct the face-centered variables by the transverse flux gradients 
//                2. This function assumes that "N_FC_VAR == N_FC_FLUX == NGrid"
//
// Parameter   :  FC_Var   : Array to store the input and output face-centered conserved variables
//                           --> Size is assumed to be N_FC_VAR
//                FC_Flux  : Array storing the input face-centered fluxes
//                           --> Size is assumed to be N_FC_FLUX
//                dt       : Time interval to advance solution
//                dh       : Grid size
//-------------------------------------------------------------------------------------------------------
void TGradient_Correction( real FC_Var[][6][5], const real FC_Flux[][3][5], const real dt, const real dh )
{

   const int  NGrid  = N_FC_VAR;    // size of the arrays FC_Var and FC_Flux in each direction
   const int  dID[3] = { 1, NGrid, NGrid*NGrid };
   const real dt_dh2 = (real)0.5*dt/dh; 

   real Correct, TGrad1, TGrad2;
   int dL, dR, ID, ID_L1, ID_L2, ID_R, TDir1, TDir2, Gap[3]={0};


// loop over different spatial directions
   for (int d=0; d<3; d++)
   {
      dL    = 2*d;
      dR    = dL+1;
      TDir1 = (d+1)%3;  // transverse direction ONE
      TDir2 = (d+2)%3;  // transverse direction TWO

      switch ( d )
      {
         case 0 : Gap[0] = 0;   Gap[1] = 1;   Gap[2] = 1;   break;
         case 1 : Gap[0] = 1;   Gap[1] = 0;   Gap[2] = 1;   break;
         case 2 : Gap[0] = 1;   Gap[1] = 1;   Gap[2] = 0;   break;
      }

      for (int k=Gap[2]; k<NGrid-Gap[2]; k++)
      for (int j=Gap[1]; j<NGrid-Gap[1]; j++)
      for (int i=Gap[0]; i<NGrid-Gap[0]; i++)
      {
         ID    = (k*NGrid + j)*NGrid + i;
         ID_R  = ID;
         ID_L1 = ID_R - dID[TDir1];
         ID_L2 = ID_R - dID[TDir2];

         for (int v=0; v<5; v++)    
         {
            TGrad1  = FC_Flux[ID_R][TDir1][v] - FC_Flux[ID_L1][TDir1][v];
            TGrad2  = FC_Flux[ID_R][TDir2][v] - FC_Flux[ID_L2][TDir2][v];
            Correct = -dt_dh2*( TGrad1 + TGrad2 ); 

            FC_Var[ID][dL][v] += Correct;
            FC_Var[ID][dR][v] += Correct;
         }
      }

   } // for (int d=0; d<3; d++)

} // FUNCTION : TGradient_Correction



#endif // #if ( !defined GPU  &&  MODEL == HYDRO  &&  FLU_SCHEME == CTU )
