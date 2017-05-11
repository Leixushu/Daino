#include "GetCube.h"




//-------------------------------------------------------------------------------------------------------
// Function    :  Flu_Restrict
// Description :  Replace the fluid data at level "lv" by the average fluid data at level "lv+1" 
//
// Parameter   :  lv          : Targeted refinement level at which the data are going to be replaced
//                GetAvePot   : Get the average potential field
//-------------------------------------------------------------------------------------------------------
void Flu_Restrict( const int lv, const bool GetAvePot )
{

   if ( lv == NLEVEL - 1 )
   {
      fprintf( stderr, "WARNING : the function \"%s\" should NOT be applied to the finest level !! \n",
               __FUNCTION__ );
      return;
   }

   if ( NPatchComma[lv+1][1] == 0 )
      return;


   int SonPID0, SonPID, ii0, jj0, kk0, ii, jj, kk, I, J, K, Ip, Jp, Kp; 

   for (int PID=0; PID<NPatchComma[lv][1]; PID++)
   {
      SonPID0 = patch.ptr[lv][PID]->son;

      if ( SonPID0 != -1 )
      {
//       restrict the data from 8 sons to itself 
         for (int LocalID=0; LocalID<8; LocalID++)
         {
            SonPID   = SonPID0 + LocalID;
            ii0      = TABLE_02( LocalID, 'x', 0, PATCH_SIZE/2 ); 
            jj0      = TABLE_02( LocalID, 'y', 0, PATCH_SIZE/2 ); 
            kk0      = TABLE_02( LocalID, 'z', 0, PATCH_SIZE/2 ); 


//          some son patches may lie outside the candidate box
            if ( patch.ptr[lv+1][SonPID]->fluid == NULL )   continue;


            for (int v=0; v<NCOMP; v++)         {
            for (int k=0; k<PATCH_SIZE/2; k++)  {  K = k*2;    Kp = K+1;   kk = k + kk0;
            for (int j=0; j<PATCH_SIZE/2; j++)  {  J = j*2;    Jp = J+1;   jj = j + jj0;
            for (int i=0; i<PATCH_SIZE/2; i++)  {  I = i*2;    Ip = I+1;   ii = i + ii0;

               patch.ptr[lv][PID]->fluid[v][kk][jj][ii] 
                  = 0.125 * ( patch.ptr[lv+1][SonPID]->fluid[v][K ][J ][I ] + 
                              patch.ptr[lv+1][SonPID]->fluid[v][K ][J ][Ip] +
                              patch.ptr[lv+1][SonPID]->fluid[v][K ][Jp][I ] +
                              patch.ptr[lv+1][SonPID]->fluid[v][Kp][J ][I ] +
                              patch.ptr[lv+1][SonPID]->fluid[v][K ][Jp][Ip] +
                              patch.ptr[lv+1][SonPID]->fluid[v][Kp][Jp][I ] +
                              patch.ptr[lv+1][SonPID]->fluid[v][Kp][J ][Ip] +
                              patch.ptr[lv+1][SonPID]->fluid[v][Kp][Jp][Ip]   );
            }}}}

            if ( GetAvePot )
            for (int k=0; k<PATCH_SIZE/2; k++)  {  K = k*2;    Kp = K+1;   kk = k + kk0;
            for (int j=0; j<PATCH_SIZE/2; j++)  {  J = j*2;    Jp = J+1;   jj = j + jj0;
            for (int i=0; i<PATCH_SIZE/2; i++)  {  I = i*2;    Ip = I+1;   ii = i + ii0;

               patch.ptr[lv][PID]->pot[kk][jj][ii] 
                  = 0.125 * ( patch.ptr[lv+1][SonPID]->pot[K ][J ][I ] + 
                              patch.ptr[lv+1][SonPID]->pot[K ][J ][Ip] +
                              patch.ptr[lv+1][SonPID]->pot[K ][Jp][I ] +
                              patch.ptr[lv+1][SonPID]->pot[Kp][J ][I ] +
                              patch.ptr[lv+1][SonPID]->pot[K ][Jp][Ip] +
                              patch.ptr[lv+1][SonPID]->pot[Kp][Jp][I ] +
                              patch.ptr[lv+1][SonPID]->pot[Kp][J ][Ip] +
                              patch.ptr[lv+1][SonPID]->pot[Kp][Jp][Ip]   );
            }}}

         } // for (int LocalID=0; LocalID<8; LocalID++)

//       rescale real and imaginary parts to get the correct density in ELBDM
#        if ( MODEL == ELBDM )
         real Real, Imag, Rho_Wrong, Rho_Corr, Rescale;

         if ( patch.ptr[lv][PID]->fluid != NULL )
         for (int k=0; k<PATCH_SIZE; k++)
         for (int j=0; j<PATCH_SIZE; j++)
         for (int i=0; i<PATCH_SIZE; i++)
         {
            Real      = patch.ptr[lv][PID]->fluid[REAL][k][j][i];
            Imag      = patch.ptr[lv][PID]->fluid[IMAG][k][j][i];
            Rho_Wrong = Real*Real + Imag*Imag;
            Rho_Corr  = patch.ptr[lv][PID]->fluid[DENS][k][j][i];
            Rescale   = sqrt( Rho_Corr/Rho_Wrong );

            patch.ptr[lv][PID]->fluid[REAL][k][j][i] *= Rescale;
            patch.ptr[lv][PID]->fluid[IMAG][k][j][i] *= Rescale;
         }
#        endif

      } // if ( SonPID0 != -1 )
   } // for (int PID=0; PID<NPatchComma[lv][1]; PID++)

}



