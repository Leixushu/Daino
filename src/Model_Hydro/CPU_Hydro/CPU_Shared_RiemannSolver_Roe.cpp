
#include "DAINO.h"
#include "CUFLU.h"

#if (  !defined GPU  &&  MODEL == HYDRO  &&  \
       ( RSOLVER == ROE )  &&  ( FLU_SCHEME == MHM || FLU_SCHEME == MHM_RP || FLU_SCHEME == CTU )  )



extern void CPU_Rotate3D( real InOut[], const int XYZ, const bool Forward );
extern void CPU_Con2Flux( const int XYZ, real Flux[], const real Input[], const real Gamma );
#if   ( CHECK_INTERMEDIATE == EXACT )
extern void CPU_Con2Pri( const real In[], real Out[], const real  Gamma_m1 );
extern void CPU_RiemannSolver_Exact( const int XYZ, real eival_out[], real L_star_out[], real R_star_out[], 
                                     real Flux_Out[], const real L_In[], const real R_In[], const real Gamma ); 
#elif ( CHECK_INTERMEDIATE == HLLE )
void CPU_RiemannSolver_HLLE( const int XYZ, real Flux_Out[], const real L_In[], const real R_In[], 
                             const real Gamma );
#elif ( CHECK_INTERMEDIATE == HLLC )
void CPU_RiemannSolver_HLLC( const int XYZ, real Flux_Out[], const real L_In[], const real R_In[], 
                             const real Gamma );
#endif




//-------------------------------------------------------------------------------------------------------
// Function    :  CPU_RiemannSolver_Roe
// Description :  Approximate Riemann solver of Roe
//
// Note        :  1. The input data should be conserved variables 
//                2. Ref : "Riemann Solvers and Numerical Methods for Fluid Dynamics - A Practical Introduction
//                         ~ by Eleuterio F. Toro" 
//                3. This function is shared by MHM, MHM_RP, and CTU schemes
//
// Parameter   :  XYZ      : Targeted spatial direction : (0/1/2) --> (x/y/z)  
//                Flux_Out : Array to store the output flux
//                L_In     : Input left  state (conserved variables)
//                R_In     : Input right state (conserved variables)
//                Gamma    : Ratio of specific heats
//-------------------------------------------------------------------------------------------------------
void CPU_RiemannSolver_Roe( const int XYZ, real Flux_Out[], const real L_In[], const real R_In[], 
                            const real Gamma )
{

// 1. reorder the input variables for different spatial directions
   real L[5], R[5];

   for (int v=0; v<5; v++)    
   {
      L[v] = L_In[v];
      R[v] = R_In[v];
   }

   CPU_Rotate3D( L, XYZ, true );
   CPU_Rotate3D( R, XYZ, true );


// 2. evaluate the average values
   const real Gamma_m1 = Gamma - (real)1.0;
   real _RhoL, _RhoR, HL, HR, u, v, w, V2, H, Cs, RhoL_sqrt, RhoR_sqrt, _RhoL_sqrt, _RhoR_sqrt, _RhoLR_sqrt_sum;
   real TempP_Rho;
   
   _RhoL = (real)1.0 / L[0];
   _RhoR = (real)1.0 / R[0];
   HL    = (  L[4] + Gamma_m1*( L[4] - (real)0.5*( L[1]*L[1] + L[2]*L[2] + L[3]*L[3] )*_RhoL )  )*_RhoL;  
   HR    = (  R[4] + Gamma_m1*( R[4] - (real)0.5*( R[1]*R[1] + R[2]*R[2] + R[3]*R[3] )*_RhoR )  )*_RhoR;  

   RhoL_sqrt       = SQRT( L[0] );
   RhoR_sqrt       = SQRT( R[0] );
   _RhoL_sqrt      = (real)1.0 / RhoL_sqrt;
   _RhoR_sqrt      = (real)1.0 / RhoR_sqrt;
   _RhoLR_sqrt_sum = (real)1.0 / (RhoL_sqrt + RhoR_sqrt); 

   u  = _RhoLR_sqrt_sum*( _RhoL_sqrt*L[1] + _RhoR_sqrt*R[1] );
   v  = _RhoLR_sqrt_sum*( _RhoL_sqrt*L[2] + _RhoR_sqrt*R[2] );
   w  = _RhoLR_sqrt_sum*( _RhoL_sqrt*L[3] + _RhoR_sqrt*R[3] );
   V2 = u*u + v*v + w*w;
   H  = _RhoLR_sqrt_sum*(  RhoL_sqrt*HL   +  RhoR_sqrt*HR   );

   TempP_Rho = H - (real)0.5*V2;
#  ifdef ENFORCE_POSITIVE
   TempP_Rho = FMAX( TempP_Rho, MIN_VALUE );
#  endif
   Cs        = SQRT( Gamma_m1*TempP_Rho );


// 3. evaluate the eigenvalues and eigenvectors
   const real EigenVec[5][5] = {  { (real)1.0,       u-Cs,         v,         w,       H-u*Cs },
                                  { (real)1.0,          u,         v,         w, (real)0.5*V2 },
                                  { (real)0.0,  (real)0.0, (real)1.0, (real)0.0,            v },
                                  { (real)0.0,  (real)0.0, (real)0.0, (real)1.0,            w },
                                  { (real)1.0,       u+Cs,         v,         w,       H+u*Cs }  };
   real EigenVal[5] = { u-Cs, u, u, u, u+Cs };


// 4. evalute the left and right fluxes
   real Flux_L[5], Flux_R[5];

   CPU_Con2Flux( 0, Flux_L, L, Gamma );
   CPU_Con2Flux( 0, Flux_R, R, Gamma );


// 5. return the upwind fluxes if flow is supersonic
   if ( EigenVal[0] >= (real)0.0 )
   {
      for (int v=0; v<5; v++)    Flux_Out[v] = Flux_L[v];

      CPU_Rotate3D( Flux_Out, XYZ, false );

      return;
   }

   if ( EigenVal[4] <= (real)0.0 )
   {
      for (int v=0; v<5; v++)    Flux_Out[v] = Flux_R[v];

      CPU_Rotate3D( Flux_Out, XYZ, false );

      return;
   }


// 6. evalute the amplitudes along different characteristics (eigenvectors)
   real Jump[5], Amp[5];

   for (int v=0; v<5; v++)    Jump[v] = R[v] - L[v];

   Amp[2] = Jump[2] - v*Jump[0];
   Amp[3] = Jump[3] - w*Jump[0];
   Amp[1] = Gamma_m1/(Cs*Cs)*( Jump[0]*(H-u*u) + u*Jump[1] - Jump[4] + v*Amp[2] + w*Amp[3] );
   Amp[0] = (real)0.5/Cs*( Jump[0]*(u+Cs) - Jump[1] - Cs*Amp[1] );
   Amp[4] = Jump[0] - Amp[0] - Amp[1];


// 7. verify that the density and pressure in the intermediate states are positive
#  ifdef CHECK_INTERMEDIATE
   real I_Pres, I_States[5];

#  if ( CHECK_INTERMEDIATE == EXACT )
   real PriVar_L[5], PriVar_R[5];
#  endif


   for (int v=0; v<5; v++)    I_States[v] = L[v];

   for (int t=0; t<4; t++)
   {
      for (int v=0; v<5; v++)    I_States[v] += Amp[t]*EigenVec[t][v];

      if ( EigenVal[t+1] > EigenVal[t] )  // skip the degenerate states
      {
         I_Pres = I_States[4] - (real)0.5*( I_States[1]*I_States[1] + I_States[2]*I_States[2] + 
                                            I_States[3]*I_States[3] ) / I_States[0];

         if ( I_States[0] <= (real)0.0  ||  I_Pres <= (real)0.0 )
         {
#           ifdef DAINO_DEBUG
            Aux_Message( stderr, "WARNING : intermediate states check failed !!\n" );
#           endif

#           if   ( CHECK_INTERMEDIATE == EXACT )   // recalculate fluxes by exact solver

            CPU_Con2Pri( L, PriVar_L, Gamma_m1 );
            CPU_Con2Pri( R, PriVar_R, Gamma_m1 );

            CPU_RiemannSolver_Exact( 0, NULL, NULL, NULL, Flux_Out, PriVar_L, PriVar_R, Gamma );

#           elif ( CHECK_INTERMEDIATE == HLLE )    // recalculate fluxes by HLLE solver

            CPU_RiemannSolver_HLLE( 0, Flux_Out, L, R, Gamma );

#           elif ( CHECK_INTERMEDIATE == HLLC )    // recalculate fluxes by HLLC solver

            CPU_RiemannSolver_HLLC( 0, Flux_Out, L, R, Gamma );

#           else

#           error : ERROR : unsupported CHECK_INTERMEDIATE (EXACT/HLLE/HLLC) !!

#           endif

            CPU_Rotate3D( Flux_Out, XYZ, false );
            return;

         } // if ( I_States[0] <= (real)0.0  ||  I_Pres <= (real)0.0 )
      } // if ( EigenVal[t+1] > EigenVal[t] )
   } // for (int t=0; t<4; t++)
#  endif // #ifdef CHECK_INTERMEDIATE


// 8. evalute the Roe fluxes
   for (int v=0; v<5; v++)    Amp[v] *= FABS( EigenVal[v] );

   for (int v=0; v<5; v++)
      Flux_Out[v] = (real)0.5*( Flux_L[v] + Flux_R[v] ) - (real)0.5*(   Amp[0]*EigenVec[0][v]
                                                                      + Amp[1]*EigenVec[1][v]
                                                                      + Amp[2]*EigenVec[2][v]
                                                                      + Amp[3]*EigenVec[3][v]
                                                                      + Amp[4]*EigenVec[4][v] );

// 9. restore the correct order
   CPU_Rotate3D( Flux_Out, XYZ, false );

} // FUNCTION : CPU_RiemannSolver_Roe



#endif // #if (  !defined GPU  &&  MODEL == HYDRO  &&  ( RSOLVER == ROE )  &&  ( FLU_SCHEME == MHM/MHM_RP/CTU )  )
