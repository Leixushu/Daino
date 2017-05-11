
#ifndef __CUFLU_RIEMANNSOLVER_HLLC_CU__
#define __CUFLU_RIEMANNSOLVER_HLLC_CU__



#include "CUFLU.h"
#include "CUFLU_Shared_FluUtility.cu"

static __device__ FluVar CUFLU_RiemannSolver_HLLC( const int XYZ, const FluVar L_In, const FluVar R_In, 
                                                   const real Gamma );




//-------------------------------------------------------------------------------------------------------
// Function    :  CUFLU_RiemannSolver_HLLC
// Description :  Approximate Riemann solver of Harten, Lax, and van Leer.
//                The wave speed is estimated by the same formula in HLLE solver
//
// Note        :  1. The input data should be conserved variables 
//                2. Ref : a. Riemann Solvers and Numerical Methods for Fluid Dynamics - A Practical Introduction
//                             ~ by Eleuterio F. Toro 
//                         b. Batten, P., Clarke, N., Lambert, C., & Causon, D. M. 1997, SIAM J. Sci. Comput.,
//                            18, 1553
//                3. This function is shared by MHM, MHM_RP, and CTU schemes
//                4. The "__noinline__" qualifier is added in Fermi GPUs when "CHECK_INTERMEDIATE == HLLC" 
//                   is adopted for higher performance 
//
// Parameter   :  XYZ      : Targeted spatial direction : (0/1/2) --> (x/y/z)  
//                L_In     : Input left  state (conserved variables)
//                R_In     : Input right state (conserved variables)
//                Gamma    : Ratio of specific heats
//-------------------------------------------------------------------------------------------------------
#if ( __CUDA_ARCH__ >= 200  &&  RSOLVER != HLLC ) // for CHECK_INTERMEDIATE == HLLC
__noinline__ 
#endif
__device__ FluVar CUFLU_RiemannSolver_HLLC( const int XYZ, const FluVar L_In, const FluVar R_In, 
                                            const real Gamma )
{

// 1. reorder the input variables for different spatial directions
   FluVar L = CUFLU_Rotate3D( L_In, XYZ, true );  
   FluVar R = CUFLU_Rotate3D( R_In, XYZ, true );


// 2. evaluate the Roe's average values
   const real Gamma_m1 = Gamma - (real)1.0;
   real _RhoL, _RhoR, P_L, P_R, H_L, H_R, u, v, w, V2, H, Cs; 
   real RhoL_sqrt, RhoR_sqrt, _RhoL_sqrt, _RhoR_sqrt, _RhoLR_sqrt_sum, TempP_Rho;
   
   _RhoL = (real)1.0 / L.Rho;
   _RhoR = (real)1.0 / R.Rho;
   P_L   = Gamma_m1*(  L.Egy - (real)0.5*( L.Px*L.Px + L.Py*L.Py + L.Pz*L.Pz )*_RhoL  );
   P_R   = Gamma_m1*(  R.Egy - (real)0.5*( R.Px*R.Px + R.Py*R.Py + R.Pz*R.Pz )*_RhoR  );
#  ifdef ENFORCE_POSITIVE
   P_L   = FMAX( P_L, MIN_VALUE );
   P_R   = FMAX( P_R, MIN_VALUE );
#  endif
   H_L   = ( L.Egy + P_L )*_RhoL;  
   H_R   = ( R.Egy + P_R )*_RhoR;  

   RhoL_sqrt       = SQRT( L.Rho );
   RhoR_sqrt       = SQRT( R.Rho );
   _RhoL_sqrt      = (real)1.0 / RhoL_sqrt;
   _RhoR_sqrt      = (real)1.0 / RhoR_sqrt;
   _RhoLR_sqrt_sum = (real)1.0 / (RhoL_sqrt + RhoR_sqrt); 

   u  = _RhoLR_sqrt_sum*( _RhoL_sqrt*L.Px + _RhoR_sqrt*R.Px );
   v  = _RhoLR_sqrt_sum*( _RhoL_sqrt*L.Py + _RhoR_sqrt*R.Py );
   w  = _RhoLR_sqrt_sum*( _RhoL_sqrt*L.Pz + _RhoR_sqrt*R.Pz );
   V2 = u*u + v*v + w*w;
   H  = _RhoLR_sqrt_sum*(  RhoL_sqrt*H_L  +  RhoR_sqrt*H_R  );

   TempP_Rho = H - (real)0.5*V2;
#  ifdef ENFORCE_POSITIVE
   TempP_Rho = FMAX( TempP_Rho, MIN_VALUE );
#  endif
   Cs        = SQRT( Gamma_m1*TempP_Rho );


// 3. estimate the maximum wave speeds
   FluVar EVal;
   real u_L, u_R, Cs_L, Cs_R, W_L, W_R, MaxV_L, MaxV_R;

   EVal.Rho = u - Cs;
   EVal.Px  = u;
   EVal.Py  = u;
   EVal.Pz  = u;
   EVal.Egy = u + Cs;

   u_L    = _RhoL*L.Px;
   u_R    = _RhoR*R.Px;
   Cs_L   = SQRT( Gamma*P_L*_RhoL );
   Cs_R   = SQRT( Gamma*P_R*_RhoR );
   W_L    = FMIN( EVal.Rho, u_L-Cs_L );
   W_R    = FMAX( EVal.Egy, u_R+Cs_R );
   MaxV_L = FMIN( W_L, (real)0.0 );
   MaxV_R = FMAX( W_R, (real)0.0 );


// 4. evaluate the star-region velocity (V_S) and pressure (P_S)
   real V_S, P_S, temp1_L, temp1_R, temp2_L, temp2_R, temp3;

   temp1_L = L.Rho*( u_L - W_L );
   temp1_R = R.Rho*( u_R - W_R );
   temp2_L = P_L + temp1_L*u_L;
   temp2_R = P_R + temp1_R*u_R;
   temp3   = real(1.0) / ( temp1_L - temp1_R );

   V_S = temp3*( P_L - P_R + temp1_L*u_L - temp1_R*u_R );
   P_S = temp3*( temp1_L*temp2_R - temp1_R*temp2_L );

#  ifdef ENFORCE_POSITIVE
   P_S = FMAX( P_S, MIN_VALUE );
#  endif


// 5. evaluate the weightings of the left(right) fluxes and contact wave
   FluVar Flux_LR;
   real temp4, Coeff_S;

   if ( V_S >= (real)0.0 )
   {
      Flux_LR = CUFLU_Con2Flux( L, Gamma_m1, 0 );

//    fluxes along the maximum wave speed      
      Flux_LR.Rho -= MaxV_L*L.Rho;
      Flux_LR.Px  -= MaxV_L*L.Px;
      Flux_LR.Py  -= MaxV_L*L.Py;
      Flux_LR.Pz  -= MaxV_L*L.Pz;
      Flux_LR.Egy -= MaxV_L*L.Egy;

      temp4   = (real)1.0 / ( V_S - MaxV_L );
      Coeff_S = -temp4*MaxV_L*P_S;
   }

   else // V_S < 0.0
   {
      Flux_LR = CUFLU_Con2Flux( R, Gamma_m1, 0 );

//    fluxes along the maximum wave speed      
      Flux_LR.Rho -= MaxV_R*R.Rho;
      Flux_LR.Px  -= MaxV_R*R.Px;
      Flux_LR.Py  -= MaxV_R*R.Py;
      Flux_LR.Pz  -= MaxV_R*R.Pz;
      Flux_LR.Egy -= MaxV_R*R.Egy;

      temp4   = (real)1.0 / ( V_S - MaxV_R );
      Coeff_S = -temp4*MaxV_R*P_S;
   }


// 6. evaluate the HLLC fluxes
   const real Coeff_LR = temp4*V_S;

   Flux_LR.Rho *= Coeff_LR;
   Flux_LR.Px  *= Coeff_LR;
   Flux_LR.Py  *= Coeff_LR;
   Flux_LR.Pz  *= Coeff_LR;
   Flux_LR.Egy *= Coeff_LR;

   Flux_LR.Px  += Coeff_S;
   Flux_LR.Egy += Coeff_S*V_S;


// 7. restore the correct order
   Flux_LR = CUFLU_Rotate3D( Flux_LR, XYZ, false );

   return Flux_LR;

} // FUNCTION : CUFLU_RiemannSolver_HLLC



#endif // #ifndef __CUFLU_RIEMANNSOLVER_HLLC_CU__
