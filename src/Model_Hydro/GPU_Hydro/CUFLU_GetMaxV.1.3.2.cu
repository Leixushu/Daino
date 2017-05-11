
#include "Macro.h"
#include "CUFLU.h"

#if ( defined GPU  &&  MODEL == HYDRO )



//-------------------------------------------------------------------------------------------------------
// Function    :  CUFLU_GetMaxCFL
// Description :  Evaluate the maximum propagation speed in each patch group 
//
// Note        :  Prefix "g" for pointers pointing to the "Global" memory space
//                Prefix "s" for pointers pointing to the "Shared" memory space
//
// Parameter   :  g_Fluid  : Global memory array to store the fluid variables
//                g_MaxCFL : Global memory array to store the maximum propagation speed in each patch group
//                Gamma    : Ratio of specific heats
//-------------------------------------------------------------------------------------------------------
__global__ void CUFLU_GetMaxCFL( real g_Fluid[][5][ PS2*PS2*PS2 ], real g_MaxCFL[], const real Gamma )
{

   const uint bx       = blockIdx.x;
   const uint tx       = threadIdx.x; 
   const uint ty       = threadIdx.y; 
   const uint ID0      = ty*PS2 + tx;
   const real Gamma_m1 = Gamma - (real)1.0;

   real u[5], Ek, Pres, Cs, MaxV;
   int ID;

   volatile __shared__ real s_MaxCFL_xy[PS2*PS2];
   volatile __shared__ real s_MaxCFL_z [PS2];


   for (int z=0; z<PS2; z++)
   {
      ID = ID0 + z*PS2*PS2;

      u[0] = (real)1.0 / g_Fluid[bx][0][ID];
      u[1] = FABS( g_Fluid[bx][1][ID] );
      u[2] = FABS( g_Fluid[bx][2][ID] );
      u[3] = FABS( g_Fluid[bx][3][ID] );
      u[4] = g_Fluid[bx][4][ID];

      Ek   = (real)0.5*( u[1]*u[1] + u[2]*u[2] + u[3]*u[3] )*u[0]; 
      Pres = Gamma_m1*( u[4] - Ek );
#     ifdef ENFORCE_POSITIVE
//    Pres = FMAX( Pres, MIN_VALUE ); // replaced by the following line for detecting NaN
      Pres = ( Pres < MIN_VALUE ) ? MIN_VALUE : Pres;
#     endif
      Cs   = SQRT( Gamma*Pres*u[0] );

#     if   ( FLU_SCHEME == RTVD  ||  FLU_SCHEME == CTU  ||  FLU_SCHEME == WAF )
      MaxV             = ( u[1] > u[2] ) ? u[1] : u[2];
      MaxV             = ( u[3] > MaxV ) ? u[3] : MaxV;
      MaxV            *= u[0];
      s_MaxCFL_xy[ID0] = MaxV + Cs;

#     elif ( FLU_SCHEME == MHM  ||  FLU_SCHEME == MHM_RP )
      MaxV             = u[0]*( u[1] + u[2] + u[3] );
      s_MaxCFL_xy[ID0] = MaxV + (real)3.0*Cs;
#     endif

      __syncthreads();


//    perform the reduction operation to get the maximum CFL speed in each z slice
      if ( ID0 < 128 )
         s_MaxCFL_xy[ID0] = ( s_MaxCFL_xy[ID0+128] > s_MaxCFL_xy[ID0] ) ? s_MaxCFL_xy[ID0+128] : s_MaxCFL_xy[ID0];

      __syncthreads();

      if ( ID0 < 64 )
         s_MaxCFL_xy[ID0] = ( s_MaxCFL_xy[ID0+ 64] > s_MaxCFL_xy[ID0] ) ? s_MaxCFL_xy[ID0+ 64] : s_MaxCFL_xy[ID0];

      __syncthreads();

      if ( ID0 < 32 ) 
      {  
         s_MaxCFL_xy[ID0] = ( s_MaxCFL_xy[ID0+ 32] > s_MaxCFL_xy[ID0] ) ? s_MaxCFL_xy[ID0+ 32] : s_MaxCFL_xy[ID0];
         s_MaxCFL_xy[ID0] = ( s_MaxCFL_xy[ID0+ 16] > s_MaxCFL_xy[ID0] ) ? s_MaxCFL_xy[ID0+ 16] : s_MaxCFL_xy[ID0];
         s_MaxCFL_xy[ID0] = ( s_MaxCFL_xy[ID0+  8] > s_MaxCFL_xy[ID0] ) ? s_MaxCFL_xy[ID0+  8] : s_MaxCFL_xy[ID0];
         s_MaxCFL_xy[ID0] = ( s_MaxCFL_xy[ID0+  4] > s_MaxCFL_xy[ID0] ) ? s_MaxCFL_xy[ID0+  4] : s_MaxCFL_xy[ID0];
         s_MaxCFL_xy[ID0] = ( s_MaxCFL_xy[ID0+  2] > s_MaxCFL_xy[ID0] ) ? s_MaxCFL_xy[ID0+  2] : s_MaxCFL_xy[ID0];
         s_MaxCFL_xy[ID0] = ( s_MaxCFL_xy[ID0+  1] > s_MaxCFL_xy[ID0] ) ? s_MaxCFL_xy[ID0+  1] : s_MaxCFL_xy[ID0];
      }

      __syncthreads();

      if ( ID0 == 0 )      s_MaxCFL_z[z] = s_MaxCFL_xy[0];

      __syncthreads();

   } // for (int z=FLU_GHOST_SIZE; z<FLU_GHOST_SIZE+PS2; z++)


// perform the reduction operation to get the maximum CFL speed of each patch
   if ( ID0 < 8 ) 
   {  
      s_MaxCFL_z[ID0] = ( s_MaxCFL_z[ID0+8] > s_MaxCFL_z[ID0] ) ? s_MaxCFL_z[ID0+8] : s_MaxCFL_z[ID0];
      s_MaxCFL_z[ID0] = ( s_MaxCFL_z[ID0+4] > s_MaxCFL_z[ID0] ) ? s_MaxCFL_z[ID0+4] : s_MaxCFL_z[ID0];
      s_MaxCFL_z[ID0] = ( s_MaxCFL_z[ID0+2] > s_MaxCFL_z[ID0] ) ? s_MaxCFL_z[ID0+2] : s_MaxCFL_z[ID0];
      s_MaxCFL_z[ID0] = ( s_MaxCFL_z[ID0+1] > s_MaxCFL_z[ID0] ) ? s_MaxCFL_z[ID0+1] : s_MaxCFL_z[ID0];
   }


// store the maximum CFL speed among each patch back to the global memory
   if ( ID0 == 0 )   g_MaxCFL[bx] = s_MaxCFL_z[0];

} // FUNCTION : CUFLU_GetMaxCFL



#endif // #if ( defined GPU  &&  MODEL == HYDRO )
