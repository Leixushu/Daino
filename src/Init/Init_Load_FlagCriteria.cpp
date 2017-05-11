
#include "DAINO.h"




//-------------------------------------------------------------------------------------------------------
// Function    :  Init_Load_FlagCriteria
// Description :  Load the flag criteria from several input files named "Input__Flag_XXX"
//
// Note        :  The loaded data will be used as the refinement thresholds at each refinement level 
//-------------------------------------------------------------------------------------------------------
void Init_Load_FlagCriteria()
{

   if ( MPI_Rank == 0 )    Aux_Message( stdout, "%s ... ", __FUNCTION__ );


#  if ( MODEL != HYDRO  &&  MODEL != MHD )
   const bool OPT__FLAG_PRES_GRADIENT = false;
   double *FlagTable_PresGradient     = NULL; 
#  endif

#  if ( MODEL != ELBDM )
   const bool OPT__FLAG_ENGY_DENSITY  = false;
   double FlagTable_EngyDensity[NLEVEL-1][2];
#  endif

   const int  NFlagMode         = 6;
   const bool Flag[NFlagMode]   = { OPT__FLAG_RHO, OPT__FLAG_RHO_GRADIENT, OPT__FLAG_PRES_GRADIENT, 
                                    OPT__FLAG_ENGY_DENSITY, OPT__FLAG_LOHNER, OPT__FLAG_USER };
   const char ModeName[][100]   = { "OPT__FLAG_RHO", "OPT__FLAG_RHO_GRADIENT", "OPT__FLAG_PRES_GRADIENT", 
                                    "OPT__FLAG_ENGY_DENSITY", "OPT__FLAG_LOHNER", "OPT__FLAG_USER" };
   const char FileName[][100]   = { "Input__Flag_Rho", "Input__Flag_RhoGradient", "Input__Flag_PresGradient", 
                                    "Input__Flag_EngyDensity", "Input__Flag_Lohner", "Input__Flag_User" };
   double *FlagTable[NFlagMode] = { FlagTable_Rho, FlagTable_RhoGradient, FlagTable_PresGradient, 
                                    NULL, NULL, FlagTable_User };

   FILE *File;
   char *input_line = NULL, TargetName[100];
   size_t len = 0;
   int Trash, n;


   for (int FlagMode=0; FlagMode<NFlagMode; FlagMode++)
   {
      if ( Flag[FlagMode] )
      {
//       open the targeted file
         strcpy( TargetName, FileName[FlagMode] );

         File = fopen( TargetName, "r" );

         if ( File == NULL )
         {
            Aux_Message( stderr, "\n" );
            Aux_Error( ERROR_INFO, "the file \"%s\" does not exist for the mode \"%s\" !!\n",
                       TargetName, ModeName[FlagMode] );
         }

//       skip the header
         getline( &input_line, &len, File );

//       begin to read
         for (int lv=0; lv<NLEVEL-1; lv++)
         {
            n = getline( &input_line, &len, File );

//          check
            if ( n <= 1 )
            {
               Aux_Message( stderr, "\n" );
               Aux_Error( ERROR_INFO, "incorrect reading at level %d of the file <%s> !!\n",
                          lv, TargetName );
            }

//          OPT__FLAG_ENGY_DENSITY and OPT__FLAG_LOHNER hav two and three columns to be loaded, respectively
            if      ( FlagMode == 3 )  sscanf( input_line, "%d%lf%lf", &Trash, &FlagTable_EngyDensity[lv][0],
                                                                               &FlagTable_EngyDensity[lv][1] );
            else if ( FlagMode == 4 )  sscanf( input_line, "%d%lf%lf%lf", &Trash, &FlagTable_Lohner[lv][0],
                                                                                  &FlagTable_Lohner[lv][1], 
                                                                                  &FlagTable_Lohner[lv][2] );
            else                       sscanf( input_line, "%d%lf", &Trash, &FlagTable[FlagMode][lv] );
         }

         fclose( File );

      } // if ( Flag[FlagMode] )
   } // for (int FlagMode=0; FlagMode<NFlagMode; FlagMode++)


   if ( input_line != NULL )     free( input_line );


   if ( MPI_Rank == 0 )    Aux_Message( stdout, "done\n" );

} // FUNCTION : Init_Load_FlagCriteria
