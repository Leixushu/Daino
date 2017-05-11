
#include "DAINO.h"

#ifdef GRAVITY




//-------------------------------------------------------------------------------------------------------
// Function    :  Gra_Prepare_Pot 
// Description :  Prepare the input array "h_Pot_Array_P_Out" for the Gravity solver 
//
// Note        :  Invoke the function "Prepare_PatchGroupData"
//
// Parameter   :  lv                : Targeted refinement level
//                PrepTime          : Targeted physical time to prepare the coarse-grid data
//                h_Pot_Array_P_Out : Host array to store the prepared data
//                NPG               : Number of patch groups prepared at a time
//                PID0_List         : List recording the patch indicies with LocalID==0 to be udpated
//-------------------------------------------------------------------------------------------------------
void Gra_Prepare_Pot( const int lv, const double PrepTime, real h_Pot_Array_P_Out[][GRA_NXT][GRA_NXT][GRA_NXT], 
                      const int NPG, const int *PID0_List )
{

   const bool IntPhase_No = false;

   Prepare_PatchGroupData( lv, PrepTime, &h_Pot_Array_P_Out[0][0][0][0], GRA_GHOST_SIZE, NPG, PID0_List, _POTE,
                           OPT__GRA_INT_SCHEME, UNIT_PATCH, NSIDE_06, IntPhase_No );

} // FUNCTION : Gra_Prepare_Pot



#endif // #ifdef GRAVITY
