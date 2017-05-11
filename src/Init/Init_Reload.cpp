
#include "DAINO.h"
#include "CUFLU.h"
#ifdef GRAVITY
#include "CUPOT.h"
#endif

void ResetParameter();
static void Load_Parameter_Before_1200( FILE *File, const int FormatVersion, int &NLv_Restart, 
                                        bool &DataOrder_xyzv, bool &LoadPot );
static void Load_Parameter_After_1200 ( FILE *File, const int FormatVersion, int &NLv_Restart, 
                                        bool &DataOrder_xyzv, bool &LoadPot );
static void CompareVar( const char *VarName, const bool   RestartVar, const bool   RuntimeVar, const bool Fatal );
static void CompareVar( const char *VarName, const int    RestartVar, const int    RuntimeVar, const bool Fatal );
static void CompareVar( const char *VarName, const long   RestartVar, const long   RuntimeVar, const bool Fatal );
static void CompareVar( const char *VarName, const real   RestartVar, const real   RuntimeVar, const bool Fatal );
static void CompareVar( const char *VarName, const double RestartVar, const double RuntimeVar, const bool Fatal );




//-------------------------------------------------------------------------------------------------------
// Function    :  Init_Reload
// Description :  Reload a previous output as the initial condition
//
// Note        :  1. This function will always load the file named "RESTART"
//                   --> You can just make a symbolic link named RESTART to the file you want to use as the
//                       initial condition
//
//                2. "OPT__RESTART_HEADER == RESTART_HEADER_CHECK"   
//                   --> check if the parameters loaded from the RESTART file are consistent with the 
//                       parameters loaded from the Input__Parameter file
//
//                   "OPT__RESTART_HEADER == RESTART_HEADER_SKIP"
//                   --> skip the header information in the RESTART file
//-------------------------------------------------------------------------------------------------------
void Init_Reload()
{

   if ( MPI_Rank == 0 )    Aux_Message( stdout, "Init_Reload ...\n" ); 


   const char FileName[] = "RESTART";

   FILE *File = fopen( FileName, "rb" );

   if ( File == NULL  &&  MPI_Rank == 0 )
      Aux_Error( ERROR_INFO, "the restart file \"%s\" does not exist !!\n", FileName );

   MPI_Barrier( MPI_COMM_WORLD );


// record the size of different data types
   const int size_bool   = sizeof( bool   );
   const int size_int    = sizeof( int    );
   const int size_uint   = sizeof( uint   );
   const int size_long   = sizeof( long   );
   const int size_ulong  = sizeof( ulong  );
   const int size_real   = sizeof( real   );
   const int size_double = sizeof( double );

   if ( size_int != size_uint )
      Aux_Error( ERROR_INFO, "sizeof(int) = %d != sizeof(uint) = %d !!\n", size_int, size_uint );

   if ( size_long != size_ulong )
      Aux_Error( ERROR_INFO, "sizeof(long) = %d != sizeof(ulong) = %d !!\n", size_long, size_ulong );


// a. load the information of data format
// =================================================================================================
   long FormatVersion, HeaderSize, CheckCode;

   fread( &FormatVersion, sizeof(long), 1, File );
   fread( &HeaderSize,    sizeof(long), 1, File );
   fread( &CheckCode,     sizeof(long), 1, File );


// verify the input data format version
   if ( MPI_Rank == 0 ) 
   {
      Aux_Message( stdout, "   The version of the RESTART file's format = %ld\n", FormatVersion );

      if ( FormatVersion < 1100 )
         Aux_Error( ERROR_INFO, "unsupported data format version (only support version >= 1100) !!\n" );
   }
   MPI_Barrier( MPI_COMM_WORLD );


// check if the size of different data types are consistent (only for version >= 1200)
   if ( FormatVersion >= 1200 )
   {
      int size_bool_restart, size_int_restart, size_long_restart, size_real_restart, size_double_restart;

      fread( &size_bool_restart,   sizeof(int), 1, File );
      fread( &size_int_restart,    sizeof(int), 1, File );
      fread( &size_long_restart,   sizeof(int), 1, File );
      fread( &size_real_restart,   sizeof(int), 1, File );
      fread( &size_double_restart, sizeof(int), 1, File );

      if ( size_bool_restart != size_bool )  
         Aux_Error( ERROR_INFO, "sizeof(bool) is inconsistent : RESTART file = %d, runtime = %d !!\n",
                    size_bool_restart, size_bool );

      if ( size_int_restart != size_int )  
         Aux_Error( ERROR_INFO, "sizeof(int) is inconsistent : RESTART file = %d, runtime = %d !!\n",
                    size_int_restart, size_int );

      if ( size_long_restart != size_long )  
         Aux_Error( ERROR_INFO, "sizeof(long) is inconsistent : RESTART file = %d, runtime = %d !!\n",
                    size_long_restart, size_long );

      if ( size_real_restart != size_real )  
         Aux_Error( ERROR_INFO, "sizeof(real) is inconsistent : RESTART file = %d, runtime = %d !!\n",
                    size_real_restart, size_real );

      if ( size_double_restart != size_double )  
         Aux_Error( ERROR_INFO, "sizeof(double) is inconsistent : RESTART file = %d, runtime = %d !!\n",
                    size_double_restart, size_double );
   } // if ( FormatVersion >= 1200 )


// skip the buffer space
   const int NBuf_Format_1200 = 256 - 0*size_bool -  5*size_int -  3*size_long -  0*size_real -  0*size_double;
   const int NBuf_Format      = ( FormatVersion >= 1200 ) ? NBuf_Format_1200 : 40; 

   fseek( File, NBuf_Format, SEEK_CUR );



// b. load all simulation parameters
// =================================================================================================
   int  NLv_Restart    = NLEVEL;
   bool DataOrder_xyzv = false;
   bool LoadPot        = false;

   if ( OPT__RESTART_HEADER != RESTART_HEADER_SKIP )
   {
      if ( FormatVersion < 1200 )   
         Load_Parameter_Before_1200( File, FormatVersion, NLv_Restart, DataOrder_xyzv, LoadPot );
      else
         Load_Parameter_After_1200 ( File, FormatVersion, NLv_Restart, DataOrder_xyzv, LoadPot );
   }

   else
   {
      if ( MPI_Rank == 0 )    
         Aux_Message( stderr, "WARNING : skipping header information is dangerous and is not recommended !!\n" );
   }


// set the rescale factor for different NLEVEL
   const int rescale = 1 << ( NLEVEL - NLv_Restart );
   if ( MPI_Rank == 0  &&  rescale != 1 )    
      Aux_Message( stderr, "WARNING : the rescale factor is set to %d\n", rescale );


// set the file position indicator to "HeaderSize" and verify the check code
   long checkcode;
   fseek( File, HeaderSize, SEEK_SET );
   fread( &checkcode, sizeof(long), 1, File );

   if ( checkcode != CheckCode )
      Aux_Error( ERROR_INFO, "incorrect check code in the RESTART file (input %ld <-> expeect %ld) !!\n",
                 checkcode, CheckCode );



// c. load the simulation information
// =================================================================================================
   if ( MPI_Rank == 0 )    Aux_Message( stdout, "   Loading simulation information ... \n" );

   int NDataPatch_Total[NLv_Restart];

   fread( &DumpID,          sizeof(int),              1, File );
   fread( Time,             sizeof(double), NLv_Restart, File );
   fread( &Step,            sizeof(long),             1, File );
   fread( NPatchTotal,      sizeof(int),    NLv_Restart, File );
   fread( NDataPatch_Total, sizeof(int),    NLv_Restart, File );
   fread( AdvanceCounter,   sizeof(uint),   NLv_Restart, File );
   if ( FormatVersion >= 1200 )
#  ifdef GRAVITY
   fread( &AveDensity,      sizeof(double),           1, File );
#  else
   fseek( File, sizeof(double), SEEK_CUR );
#  endif


// set parameters in levels that do not exist in the input file
   for (int lv=NLv_Restart; lv<NLEVEL; lv++)
   {
      Time          [lv] = Time[0];
      NPatchTotal   [lv] = 0;
      AdvanceCounter[lv] = 0;
   }


// skip the buffer space
   const int NBuf_Info_1200 = 1024 - 0*size_bool - (1+3*NLv_Restart)*size_int - 2*size_long 
                                   - 0*size_real - (1+NLv_Restart)*size_double;
   const int NBuf_Info      = ( FormatVersion >= 1200 ) ? NBuf_Info_1200 : 80-size_double;

   fseek( File, NBuf_Info, SEEK_CUR );


// set the next dump ID
   if ( INIT_DUMPID < 0 )  DumpID ++;
   else                    DumpID = INIT_DUMPID;


// verify the size of the RESTART file
   long InfoSize, DataSize[NLv_Restart], ExpectSize, InputSize, PatchDataSize;
   int NVar;   // number of variables ( NCOMP or NCOMP+1 -> potential )

   InfoSize =     sizeof(int   )*( 1 + 2*NLv_Restart )
                + sizeof(long  )*( 2                 )   // Step + checkcode
                + sizeof(uint  )*(       NLv_Restart )
                + sizeof(double)*( 1 +   NLv_Restart )
                + NBuf_Info;

#  ifdef GRAVITY
   NVar = ( LoadPot ) ? NCOMP+1 : NCOMP;
#  else
   NVar = NCOMP;
#  endif

   PatchDataSize = PATCH_SIZE*PATCH_SIZE*PATCH_SIZE*NVar*sizeof(real);
   ExpectSize    = HeaderSize + InfoSize;

   for (int lv=0; lv<NLv_Restart; lv++)
   {
      DataSize[lv]  = 0;
      DataSize[lv] += NPatchTotal[lv]*4*sizeof(int);        // 4 = corner(3) + son(1)
      DataSize[lv] += NDataPatch_Total[lv]*PatchDataSize;

      ExpectSize   += DataSize[lv];
   }

   fseek( File, 0, SEEK_END );
   InputSize = ftell( File );

   if ( InputSize != ExpectSize  &&  MPI_Rank == 0 )
      Aux_Error( ERROR_INFO, "the size of the file <%s> is incorrect --> input = %ld <-> expect = %ld !!\n",
                 FileName, InputSize, ExpectSize );

   fclose( File );

   if ( MPI_Rank == 0 )    Aux_Message( stdout, "   Loading simulation information ... done\n" );


// d. load the simulation data
// =================================================================================================
   const long Offset0 = HeaderSize + InfoSize;
   int LoadCorner[3], LoadSon;

// array for re-ordering the fluid data from "xyzv" to "vxyz"
   real (*InvData_Flu)[PATCH_SIZE][PATCH_SIZE][NCOMP] = NULL;
   if ( DataOrder_xyzv )   InvData_Flu = new real [PATCH_SIZE][PATCH_SIZE][PATCH_SIZE][NCOMP];


// d0. set the load-balance cut points
#  ifdef LOAD_BALANCE
   if ( MPI_Rank == 0 )    Aux_Message( stdout, "   Setting load-balance cut points ...\n" );

   const bool InputLBIdxList_Yes = true;
   long *LBIdx_AllRank = NULL;

   if ( MPI_Rank == 0 )
   {
      File = fopen( FileName, "rb" );
      fseek( File, Offset0, SEEK_SET );
   }

   for (int lv=0; lv<NLv_Restart; lv++)
   {
//    d0-1. construct the LBIdx_AllRank list at rank 0
      if ( MPI_Rank == 0 )
      {
         LBIdx_AllRank = new long [ NPatchTotal[lv] ];

         for (int LoadPID=0; LoadPID<NPatchTotal[lv]; LoadPID++)
         {
            fread(  LoadCorner, sizeof(int), 3, File );
            fread( &LoadSon,    sizeof(int), 1, File );

            for (int d=0; d<3; d++)    LoadCorner[d] *= rescale;

            LBIdx_AllRank[LoadPID] = LB_Corner2Index( lv, LoadCorner, CHECK_ON );

            if ( LoadSon == -1 )    fseek( File, PatchDataSize, SEEK_CUR );
         }
      } // if ( MPI_Rank == 0 )

//    d0-2. set the cut points
      LB_SetCutPoint( lv, patch->LB->CutPoint[lv], InputLBIdxList_Yes, LBIdx_AllRank );

      if ( MPI_Rank == 0 )    delete [] LBIdx_AllRank;
   } // for (int lv=0; lv<NLv_Restart; lv++)

   if ( MPI_Rank == 0 )    fclose( File );
   if ( MPI_Rank == 0 )    Aux_Message( stdout, "   Setting load-balance cut points ... done\n" );
#  endif // #ifdef LOAD_BALANCE


// begin to load data
   long Offset = Offset0;
   int  PID;
#  ifndef OOC
#  ifndef LOAD_BALANCE
   int TargetRange_Min[3], TargetRange_Max[3];
#  endif
#  endif // #ifndef OOC

   for (int lv=0; lv<NLv_Restart; lv++)
   {
      for (int TargetMPIRank=0; TargetMPIRank<MPI_NRank; TargetMPIRank++)
      {
         if ( MPI_Rank == 0 )    Aux_Message( stdout, "   Loading data at level %2d, MPI_Rank %3d ... ", lv, TargetMPIRank );

         if ( MPI_Rank == TargetMPIRank )
         {

#ifndef OOC

//          d1. set the range of the targeted sub-domain
#           ifndef LOAD_BALANCE
            for (int d=0; d<3; d++)
            {
               TargetRange_Min[d] = DAINO_RANK_X(d)*NX0[d]*patch->scale[0];
               TargetRange_Max[d] = TargetRange_Min[d] + NX0[d]*patch->scale[0];
            }
#           endif


            File = fopen( FileName, "rb" );
            fseek( File, Offset, SEEK_SET );

            for (int LoadPID=0; LoadPID<NPatchTotal[lv]; LoadPID++)
            {
//             d2. load the patch information
               fread(  LoadCorner, sizeof(int), 3, File );
               fread( &LoadSon,    sizeof(int), 1, File );

               for (int d=0; d<3; d++)    LoadCorner[d] *= rescale;


//             verify that the loaded patch is within the targeted range
#              ifdef LOAD_BALANCE
               if (  MPI_Rank == LB_Index2Rank( lv, LB_Corner2Index(lv,LoadCorner,CHECK_ON), CHECK_ON )  )
#              else
               if (  LoadCorner[0] >= TargetRange_Min[0]  &&  LoadCorner[0] < TargetRange_Max[0]  &&
                     LoadCorner[1] >= TargetRange_Min[1]  &&  LoadCorner[1] < TargetRange_Max[1]  &&
                     LoadCorner[2] >= TargetRange_Min[2]  &&  LoadCorner[2] < TargetRange_Max[2]     )
#              endif
               {

                  patch->pnew( lv, LoadCorner[0], LoadCorner[1], LoadCorner[2], -1, true, true );

//                d3. load the physical data if it is a leaf patch
                  if ( LoadSon == -1 )
                  {
                     PID = patch->num[lv] - 1;

//                   d3-1. load the fluid variables
                     if ( DataOrder_xyzv )
                     {
                        fread( InvData_Flu, sizeof(real), PATCH_SIZE*PATCH_SIZE*PATCH_SIZE*NCOMP, File );

                        for (int v=0; v<NCOMP; v++)
                        for (int k=0; k<PATCH_SIZE; k++)
                        for (int j=0; j<PATCH_SIZE; j++)
                        for (int i=0; i<PATCH_SIZE; i++)    
                           patch->ptr[patch->FluSg[lv]][lv][PID]->fluid[v][k][j][i] = InvData_Flu[k][j][i][v];
                     }

                     else
                        fread( patch->ptr[ patch->FluSg[lv] ][lv][PID]->fluid, sizeof(real), 
                               PATCH_SIZE*PATCH_SIZE*PATCH_SIZE*NCOMP, File );

#                    ifdef GRAVITY
//                   d3-2. abandon the gravitational potential
                     if ( LoadPot )
                        fseek( File, PATCH_SIZE*PATCH_SIZE*PATCH_SIZE*sizeof(real), SEEK_CUR );
#                    endif
                  } // if ( DataOrder_xyzv )
               } // within the targeted range

               else // for the case that the patch is NOT within the targeted range
                  if ( LoadSon == -1 )    fseek( File, PatchDataSize, SEEK_CUR );

            } // for (int LoadPID=0; LoadPID<NPatchTotal[lv]; LoadPID++)

            fclose( File );


//          d4. record the number of the real patches and the LB_IdxList_real
            for (int m=1; m<28; m++)   patch->NPatchComma[lv][m] = patch->num[lv];

#           ifdef LOAD_BALANCE
            if ( patch->LB->IdxList_Real         [lv] != NULL )   delete [] patch->LB->IdxList_Real         [lv];
            if ( patch->LB->IdxList_Real_IdxTable[lv] != NULL )   delete [] patch->LB->IdxList_Real_IdxTable[lv];

            patch->LB->IdxList_Real         [lv] = new long [ patch->NPatchComma[lv][1] ];
            patch->LB->IdxList_Real_IdxTable[lv] = new int  [ patch->NPatchComma[lv][1] ];

            for (int RPID=0; RPID<patch->NPatchComma[lv][1]; RPID++)   
               patch->LB->IdxList_Real[lv][RPID] = patch->ptr[0][lv][RPID]->LB_Idx;

            Mis_Heapsort( patch->NPatchComma[lv][1], patch->LB->IdxList_Real[lv], patch->LB->IdxList_Real_IdxTable[lv] );
#           endif // #ifdef LOAD_BALANCE

#else // OOC

            OOC_Init_Reload_LoadData( lv, PatchDataSize, Offset, DataOrder_xyzv, InvData_Flu, LoadPot );

#endif

            Offset += DataSize[lv];

         } // if ( MPI_Rank == TargetMPIRank )

         MPI_Barrier( MPI_COMM_WORLD );

         if ( MPI_Rank == 0 )    Aux_Message( stdout, "done\n" );

      } // for (int TargetMPIRank=0; TargetMPIRank<NGPU; TargetMPIRank++)
   } // for (int lv=0; lv<NLv_Restart; lv++)


   if ( DataOrder_xyzv )  delete [] InvData_Flu;


#  ifndef LOAD_BALANCE
// the following operations are useful only when LOAD_BALANCE is NOT enabled
// ===================================================================================================================
  
// g. construct the relation "father <-> son" for the out-of-core computing
#  ifdef OOC
   OOC_Init_Reload_FindFather();
#  endif 


// h. complete all levels 
   for (int lv=0; lv<NLEVEL; lv++)
   {
#ifndef OOC

//    construct the relation "father <-> son" for the in-core computing
      if ( lv > 0 )     FindFather( lv, 1 ); 

//    allocate the buffer patches 
      Buf_AllocateBufferPatch( patch, lv, 3, 0 );

//    set up the BaseP List
      if ( lv == 0 )    Init_RecordBasePatch();

//    set up the BounP_IDMap 
      Buf_RecordBoundaryPatch( lv );

//    construct the sibling relation
      SiblingSearch( lv );

//    get the IDs of patches for sending and receiving data between neighbor ranks
      Buf_RecordExchangeDataPatchID( lv );

//    allocate the flux arrays at the level "lv-1"
      if ( lv > 0  &&  patch->WithFlux )  Flu_AllocateFluxArray( lv-1 );

#else // OOC

      OOC_Init_Reload_ConstructAllLevels( lv );

#endif
   } // for (int lv=0; lv<NLEVEL; lv++)


// i. fill up the data for patches that are not leaf patches
   for (int lv=NLEVEL-2; lv>=0; lv--)     
   {
#ifndef OOC

      Flu_Restrict( lv, patch->FluSg[lv+1], patch->FluSg[lv], NULL_INT, NULL_INT, _FLU );

//    fill up the data in the buffer patches
      Buf_GetBufferData( lv,   patch->FluSg[lv  ], NULL_INT, DATA_GENERAL, _FLU, Flu_ParaBuf, USELB_NO );

      if ( lv == NLEVEL-2 )
      Buf_GetBufferData( lv+1, patch->FluSg[lv+1], NULL_INT, DATA_GENERAL, _FLU, Flu_ParaBuf, USELB_NO );

#else // OOC

      OOC_Init_Reload_Restrict( lv );

#endif
   } // for (int lv=NLEVEL-2; lv>=0; lv--)

// ===================================================================================================================
#  endif // #ifndef LOAD_BALANCE


   if ( MPI_Rank == 0 )    Aux_Message( stdout, "Init_Reload ... done\n" ); 

} // FUNCTION : Init_Reload



//-------------------------------------------------------------------------------------------------------
// Function    :  Load_Parameter_Before_1200
// Description :  Load all simulation parameters from the RESTART file with format version < 1200
//
// Note        :  Only work in HYDRO 
//
// Parameter   :  File           : RESTART file pointer 
//                FormatVersion  : Format version of the RESTART file
//                NLv_Restart    : NLEVEL recorded in the RESTART file
//                DataOrder_xyzv : Order of data stored in the RESTART file (true/false --> xyzv/vxyz)
//                LoadPot        : Whether or not the RESTART file stores the potential data
//
// Return      :  NLv_Restart, DataOrder_xyzv, LoadPot
//-------------------------------------------------------------------------------------------------------
void Load_Parameter_Before_1200( FILE *File, const int FormatVersion, int &NLv_Restart, bool &DataOrder_xyzv, 
                                 bool &LoadPot )
{
   
// set the size of the output buffers
   const int NBuf_MakefileOption = 40  - 0*sizeof(int) - 0*sizeof(real) - 0*sizeof(double);
   const int NBuf_MakefileConst  = 80  - 0*sizeof(int) - 0*sizeof(real) - 0*sizeof(double);
   const int NBuf_Parameter      = 196 - 0*sizeof(int) - 0*sizeof(real) - 1*sizeof(double);


   if ( MPI_Rank == 0 )    Aux_Message( stdout, "   Loading simulation parameters ...\n" );


// a. load the simulation options defined in the Makefile
// =================================================================================================
   bool gravity, comoving, float8;

   fread( &gravity,                    sizeof(bool),                    1,             File );
   fread( &comoving,                   sizeof(bool),                    1,             File );
   fread( &float8,                     sizeof(bool),                    1,             File );

// skip the buffer space
   fseek( File, NBuf_MakefileOption, SEEK_CUR );


// b. load the symbolic constants defined in the Makefile
// =================================================================================================
   int ncomp, patch_size, max_patch, nlevel, flu_ghost_size, pot_ghost_size, gra_ghost_size;

   fread( &ncomp,                      sizeof(int),                     1,             File );
   fread( &patch_size,                 sizeof(int),                     1,             File );
   fread( &max_patch,                  sizeof(int),                     1,             File );
   fread( &nlevel,                     sizeof(int),                     1,             File );
   fread( &flu_ghost_size,             sizeof(int),                     1,             File );
   fread( &pot_ghost_size,             sizeof(int),                     1,             File );
   fread( &gra_ghost_size,             sizeof(int),                     1,             File );

// skip the buffer space
   fseek( File, NBuf_MakefileConst, SEEK_CUR );


// c. load the simulation parameters recorded in the file "Input__Parameter"
// =================================================================================================
   int    nx0_tot[3], DAINO_nrank, DAINO_nrank_x[3], regrid_count, flag_buffer_size;
   int    opt__flu_int_scheme, opt__pot_int_scheme, opt__rho_int_scheme, opt__gra_int_scheme;
   int    opt__ref_flu_int_scheme, opt__ref_pot_int_scheme, opt__output_total;
   double omega_m0, dt__fluid, dt__gravity, dt__max_delta_a, box_size;
   real   gamma, newton_g;
   bool   opt__gra_p5_gradient, opt__output_pot;

   fread(  nx0_tot,                    sizeof(int),                     3,             File );
   fread( &DAINO_nrank,                sizeof(int),                     1,             File );
   fread(  DAINO_nrank_x,              sizeof(int),                     3,             File );
   fread( &omega_m0,                   sizeof(double),                  1,             File );
   fread( &dt__fluid,                  sizeof(double),                  1,             File );
   fread( &dt__gravity,                sizeof(double),                  1,             File );
   fread( &dt__max_delta_a,            sizeof(double),                  1,             File );
   fread( &regrid_count,               sizeof(int),                     1,             File );
   fread( &flag_buffer_size,           sizeof(int),                     1,             File );
   fread( &gamma,                      sizeof(real),                    1,             File );
   fread( &newton_g,                   sizeof(real),                    1,             File );
   fread( &opt__gra_p5_gradient,       sizeof(bool),                    1,             File );
   fread( &opt__flu_int_scheme,        sizeof(int),                     1,             File );
   fread( &opt__pot_int_scheme,        sizeof(int),                     1,             File );
   fread( &opt__rho_int_scheme,        sizeof(int),                     1,             File );
   fread( &opt__gra_int_scheme,        sizeof(int),                     1,             File );
   fread( &opt__ref_flu_int_scheme,    sizeof(int),                     1,             File );
   fread( &opt__ref_pot_int_scheme,    sizeof(int),                     1,             File );
   fread( &opt__output_pot,            sizeof(bool),                    1,             File );
   fread( &opt__output_total,          sizeof(int),                     1,             File );
   fread( &box_size,                   sizeof(double),                  1,             File );

// skip the buffer space
   fseek( File, NBuf_Parameter, SEEK_CUR );

// reset "box_size" for format version < 1102 (in which this parameter is not added yet)
   if ( FormatVersion < 1102 )
   {
      int nx0_max;
      nx0_max = ( nx0_tot[0] > nx0_tot[1] ) ? nx0_tot[0] : nx0_tot[1];
      nx0_max = ( nx0_tot[2] > nx0_max    ) ? nx0_tot[2] : nx0_max;

      box_size = nx0_max*( 1<<(nlevel-1) );

      if ( MPI_Rank == 0 )
         Aux_Message( stderr, "WARNING : loading data with format version < 1102 --> assuming BOX_SIZE = %f\n",
                      box_size );
   }


   if ( MPI_Rank == 0 )    Aux_Message( stdout, "   Loading simulation parameters ... done\n" );


// d. check parameters (before loading any size-dependent parameters)
// =================================================================================================
   if ( MPI_Rank == 0 )
   {
      Aux_Message( stdout, "   Checking loaded parameters ...\n" );


//    errors
//    ------------------
#     ifdef GRAVITY
      if ( !gravity )
         Aux_Error( ERROR_INFO, "the loaded RESTART file is for a purely hydrodynamic sysmtem !!\n" );
#     else
      if ( gravity )
         Aux_Error( ERROR_INFO, "the loaded RESTART file is for a hydrodynamic + gravity system !!\n" );
#     endif

#     ifdef COMOVING
      if ( !comoving )
         Aux_Error( ERROR_INFO, "the loaded RESTART file is NOT simulated in the comoving frame !!\n" );
#     else
      if ( comoving )
         Aux_Error( ERROR_INFO, "the loaded RESTART file is simulated in the comoving frame !!\n" );
#     endif

#     ifdef FLOAT8
      if ( !float8 )
         Aux_Error( ERROR_INFO, "the loaded RESTART file is simulated using single precision !!\n" );
#     else
      if ( float8 )
         Aux_Error( ERROR_INFO, "the loaded RESTART file is simulated using double precision !!\n" );
#     endif

      if ( ncomp != NCOMP )
         Aux_Error( ERROR_INFO, "%s : RESTART file (%d) != runtime (%d) !!\n", "NCOMP", ncomp, NCOMP );

      if ( patch_size != PATCH_SIZE )
         Aux_Error( ERROR_INFO, "%s : RESTART file (%d) != runtime (%d) !!\n", "PATCH_SIZE", patch_size, PS1 );

      if ( nlevel > NLEVEL )
         Aux_Error( ERROR_INFO, "%s : RESTART file (%d) > runtime (%d) (please set NLEVEL larger) !!\n",
                    "NLEVEL", nlevel, NLEVEL );

      if ( nx0_tot[0] != NX0_TOT[0] )
         Aux_Error( ERROR_INFO, "%s : RESTART file (%d) != Input__Parameter (%d) !!\n", 
                    "NX0_TOT[0]", nx0_tot[0], NX0_TOT[0] );

      if ( nx0_tot[1] != NX0_TOT[1] )
         Aux_Error( ERROR_INFO, "%s : RESTART file (%d) != Input__Parameter (%d) !!\n", 
                    "NX0_TOT[1]", nx0_tot[1], NX0_TOT[1] );

      if ( nx0_tot[2] != NX0_TOT[2] )
         Aux_Error( ERROR_INFO, "%s : RESTART file (%d) != Input__Parameter (%d) !!\n", 
                    "NX0_TOT[2]", nx0_tot[2], NX0_TOT[2] );

#     if ( MODEL != ELBDM )
      if ( box_size != BOX_SIZE )
         Aux_Error( ERROR_INFO, "%s : RESTART file (%14.7e) != Input__Parameter (%14.7e) !!\n", 
                    "BOX_SIZE", box_size, BOX_SIZE );
#     endif


//    warnings 
//    ------------------
      if ( nlevel < NLEVEL )
      {
         Aux_Message( stderr, "WARNING : %s : RESTART file (%d) < runtime (%d) !!\n", 
                      "NLEVEL", nlevel, NLEVEL );
         Aux_Message( stderr, "          --> Grid scale will be rescaled\n" );
      }

      if ( max_patch != MAX_PATCH )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%d) != runtime (%d) !!\n", 
                      "MAX_PATCH", max_patch, MAX_PATCH );

      if ( flu_ghost_size != FLU_GHOST_SIZE )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%d) != runtime (%d) !!\n", 
                      "FLU_GHOST_SIZE", flu_ghost_size, FLU_GHOST_SIZE );

      if ( DAINO_nrank != DAINO_NRANK )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%d) != runtime (%d) !!\n", 
                      "DAINO_NRANK", DAINO_nrank ,DAINO_NRANK );

      if ( DAINO_nrank_x[0] != DAINO_NRANK_X(0) )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%d) != runtime (%d) !!\n", 
                      "DAINO_NRANK_X[0]", DAINO_nrank_x[0], DAINO_NRANK_X(0) );

      if ( DAINO_nrank_x[1] != DAINO_NRANK_X(1) )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%d) != runtime (%d) !!\n", 
                      "DAINO_NRANK_X[1]", DAINO_nrank_x[1], DAINO_NRANK_X(1) );

      if ( DAINO_nrank_x[2] != DAINO_NRANK_X(2) )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%d) != runtime (%d) !!\n", 
                      "DAINO_NRANK_X[2]", DAINO_nrank_x[2], DAINO_NRANK_X(2) );

      if ( dt__fluid != DT__FLUID ) 
         Aux_Message( stderr, "WARNING : %s : RESTART file (%14.7e) != runtime (%14.7e) !!\n", 
                      "DT__FLUID", dt__fluid, DT__FLUID );

      if ( regrid_count != REGRID_COUNT )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%d) != runtime (%d) !!\n", 
                      "REGRID_COUNT", regrid_count , REGRID_COUNT );

      if ( flag_buffer_size != FLAG_BUFFER_SIZE )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%d) != runtime (%d) !!\n", 
                      "FLAG_BUFFER_SIZE", flag_buffer_size, FLAG_BUFFER_SIZE );

      if ( opt__flu_int_scheme+1 != (int)OPT__FLU_INT_SCHEME )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%d) != runtime (%d) !!\n", 
                      "OPT__FLU_INT_SCHEME", opt__flu_int_scheme+1, OPT__FLU_INT_SCHEME );

      if ( opt__ref_flu_int_scheme+1 != (int)OPT__REF_FLU_INT_SCHEME )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%d) != runtime (%d) !!\n", 
                      "OPT__REF_FLU_INT_SCHEME", opt__ref_flu_int_scheme+1, OPT__REF_FLU_INT_SCHEME );

#     if ( MODEL == HYDRO )
      if ( gamma != GAMMA )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%14.7e) != runtime (%14.7e) !!\n", 
                      "GAMMA", gamma, GAMMA );
#     endif

#     ifdef COMOVING
      if ( omega_m0 != OMEGA_M0 )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%14.7e) != runtime (%14.7e) !!\n", 
                      "OMEGA_M0", omega_m0, OMEGA_M0 );

      if ( dt__max_delta_a != DT__MAX_DELTA_A ) 
         Aux_Message( stderr, "WARNING : %s : RESTART file (%14.7e) != runtime (%14.7e) !!\n", 
                      "DT__MAX_DELTA_A", dt__max_delta_a, DT__MAX_DELTA_A );
#     endif

#     ifdef GRAVITY
      if ( pot_ghost_size != POT_GHOST_SIZE )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%d) != runtime (%d) !!\n", 
                      "POT_GHOST_SIZE", pot_ghost_size, POT_GHOST_SIZE );

      if ( gra_ghost_size != GRA_GHOST_SIZE )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%d) != runtime (%d) !!\n", 
                      "GRA_GHOST_SIZE", gra_ghost_size, GRA_GHOST_SIZE );

      if ( dt__gravity != DT__GRAVITY ) 
         Aux_Message( stderr, "WARNING : %s : RESTART file (%14.7e) != runtime (%14.7e) !!\n", 
                      "DT__GRAVITY", dt__gravity, DT__GRAVITY );

      if ( newton_g != NEWTON_G )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%14.7e) != runtime (%14.7e) !!\n", 
                      "NEWTON_G", newton_g, NEWTON_G );

      if ( opt__gra_p5_gradient != OPT__GRA_P5_GRADIENT )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%d) != runtime (%d) !!\n", 
                      "OPT__GRA_P5_GRADIENT", opt__gra_p5_gradient, OPT__GRA_P5_GRADIENT );

      if ( opt__pot_int_scheme+1 != (int)OPT__POT_INT_SCHEME )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%d) != runtime (%d) !!\n", 
                      "OPT__POT_INT_SCHEME", opt__pot_int_scheme+1, OPT__POT_INT_SCHEME );

      if ( opt__rho_int_scheme+1 != (int)OPT__RHO_INT_SCHEME )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%d) != runtime (%d) !!\n", 
                      "OPT__RHO_INT_SCHEME", opt__rho_int_scheme+1, OPT__RHO_INT_SCHEME );

      if ( opt__gra_int_scheme+1 != (int)OPT__GRA_INT_SCHEME )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%d) != runtime (%d) !!\n", 
                      "OPT__GRA_INT_SCHEME", opt__gra_int_scheme+1, OPT__GRA_INT_SCHEME );

      if ( opt__ref_pot_int_scheme+1 != (int)OPT__REF_POT_INT_SCHEME )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%d) != runtime (%d) !!\n", 
                      "OPT__REF_POT_INT_SCHEME", opt__ref_pot_int_scheme+1, OPT__REF_POT_INT_SCHEME );
#     endif // #ifdef GRAVITY


      Aux_Message( stdout, "   Checking loaded parameters ... done\n" );

   } // if ( MPI_Rank == 0 )


// set the returned variables
   DataOrder_xyzv = ( FormatVersion >= 1101 ) ? ( (opt__output_total==1)?true:false ) : true;
   LoadPot        = opt__output_pot;
   NLv_Restart    = nlevel;

} // FUNCTION : Load_Parameter_Before_1200



//-------------------------------------------------------------------------------------------------------
// Function    :  Load_Parameter_After_1200
// Description :  Load all simulation parameters from the RESTART file with format version >= 1200
//
// Note        :  "OPT__RESTART_HEADER == RESTART_HEADER_INHERIC" can be used in this function
//
// Parameter   :  File           : RESTART file pointer 
//                FormatVersion  : Format version of the RESTART file
//                NLv_Restart    : NLEVEL recorded in the RESTART file
//                DataOrder_xyzv : Order of data stored in the RESTART file (true/false --> xyzv/vxyz)
//                LoadPot        : Whether or not the RESTART file stores the potential data
//
// Return      :  NLv_Restart, DataOrder_xyzv, LoadPot
//-------------------------------------------------------------------------------------------------------
void Load_Parameter_After_1200( FILE *File, const int FormatVersion, int &NLv_Restart, bool &DataOrder_xyzv, 
                                bool &LoadPot )
{

   const int size_bool      = sizeof( bool   );
   const int size_int       = sizeof( int    );
   const int size_long      = sizeof( long   );
   const int size_real      = sizeof( real   );
   const int size_double    = sizeof( double );

   const int NBuf_Makefile  =  256 - 15*size_bool -  8*size_int -  0*size_long -  0*size_real -  0*size_double;
   const int NBuf_Constant  =  256 -  6*size_bool - 11*size_int -  0*size_long -  2*size_real -  0*size_double;
   const int NBuf_Parameter = 1024 - 18*size_bool - 35*size_int -  1*size_long - 12*size_real -  8*size_double;


   if ( MPI_Rank == 0 )    Aux_Message( stdout, "   Loading simulation parameters ...\n" );


// a. load the simulation options and parameters defined in the Makefile
// =================================================================================================
   bool gravity, individual_timestep, comoving, gpu, DAINO_optimization, DAINO_debug, timing, timing_solver;
   bool intel, float8, serial, ooc, overlap_mpi, openmp, fermi;
   int  model, pot_scheme, flu_scheme, lr_scheme, rsolver, load_balance, nlevel, max_patch;

   fread( &model,                      sizeof(int),                     1,             File );
   fread( &gravity,                    sizeof(bool),                    1,             File );
   fread( &pot_scheme,                 sizeof(int),                     1,             File );
   fread( &individual_timestep,        sizeof(bool),                    1,             File );
   fread( &comoving,                   sizeof(bool),                    1,             File );
   fread( &flu_scheme,                 sizeof(int),                     1,             File );
   fread( &lr_scheme,                  sizeof(int),                     1,             File );
   fread( &rsolver,                    sizeof(int),                     1,             File );
   fread( &gpu,                        sizeof(bool),                    1,             File );
   fread( &DAINO_optimization,         sizeof(bool),                    1,             File );
   fread( &DAINO_debug,                sizeof(bool),                    1,             File );
   fread( &timing,                     sizeof(bool),                    1,             File );
   fread( &timing_solver,              sizeof(bool),                    1,             File );
   fread( &intel,                      sizeof(bool),                    1,             File );
   fread( &float8,                     sizeof(bool),                    1,             File );
   fread( &serial,                     sizeof(bool),                    1,             File );
   fread( &ooc,                        sizeof(bool),                    1,             File );
   fread( &load_balance,               sizeof(int),                     1,             File );
   fread( &overlap_mpi,                sizeof(bool),                    1,             File );
   fread( &openmp,                     sizeof(bool),                    1,             File );
   fread( &fermi,                      sizeof(bool),                    1,             File );
   fread( &nlevel,                     sizeof(int),                     1,             File );
   fread( &max_patch,                  sizeof(int),                     1,             File );

// skip the buffer space
   fseek( File, NBuf_Makefile, SEEK_CUR );


// b. load the symbolic constants defined in "Macro.h, CUPOT.h, and CUFLU.h"
// =================================================================================================
   bool enforce_positive, char_reconstruction, hll_no_ref_state, hll_include_all_waves, waf_dissipate;
   bool use_psolver_10to14;
   int  ncomp, patch_size, flu_ghost_size, pot_ghost_size, gra_ghost_size, check_intermediate; 
   int  flu_block_size_x, flu_block_size_y, pot_block_size_x, pot_block_size_z, gra_block_size_z;
   real min_value, max_error;

   fread( &ncomp,                      sizeof(int),                     1,             File );
   fread( &patch_size,                 sizeof(int),                     1,             File );
   fread( &min_value,                  sizeof(real),                    1,             File );
   fread( &flu_ghost_size,             sizeof(int),                     1,             File );
   fread( &pot_ghost_size,             sizeof(int),                     1,             File );
   fread( &gra_ghost_size,             sizeof(int),                     1,             File );
   fread( &enforce_positive,           sizeof(bool),                    1,             File );
   fread( &char_reconstruction,        sizeof(bool),                    1,             File );
   fread( &check_intermediate,         sizeof(int),                     1,             File );
   fread( &hll_no_ref_state,           sizeof(bool),                    1,             File );
   fread( &hll_include_all_waves,      sizeof(bool),                    1,             File );
   fread( &waf_dissipate,              sizeof(bool),                    1,             File );
   fread( &max_error,                  sizeof(real),                    1,             File );
   fread( &flu_block_size_x,           sizeof(int),                     1,             File );
   fread( &flu_block_size_y,           sizeof(int),                     1,             File );
   fread( &use_psolver_10to14,         sizeof(bool),                    1,             File );
   fread( &pot_block_size_x,           sizeof(int),                     1,             File );
   fread( &pot_block_size_z,           sizeof(int),                     1,             File );
   fread( &gra_block_size_z,           sizeof(int),                     1,             File );

// skip the buffer space
   fseek( File, NBuf_Constant, SEEK_CUR );


// c. load the simulation parameters recorded in the file "Input__Parameter"
// =================================================================================================
   bool   opt__adaptive_dt, opt__dt_user, opt__flag_rho, opt__flag_rho_gradient, opt__flag_pres_gradient;
   bool   opt__flag_engy_density, opt__flag_user, opt__fixup_flux, opt__fixup_restrict, opt__overlap_mpi;
   bool   opt__gra_p5_gradient, opt__int_time, opt__output_error, opt__output_base, opt__output_pot; 
   bool   opt__output_baseps, opt__timing_barrier, opt__int_phase;
   int    nx0_tot[3], DAINO_nrank, DAINO_nrank_x[3], omp_nthread, ooc_nrank, ooc_nrank_x[3], regrid_count;
   int    flag_buffer_size, max_level, opt__lr_limiter, opt__waf_limiter, flu_gpu_npgroup, gpu_nstream; 
   int    sor_max_iter, sor_min_iter, mg_max_iter, mg_npre_smooth, mg_npost_smooth, pot_gpu_npgroup;
   int    opt__flu_int_scheme, opt__pot_int_scheme, opt__rho_int_scheme;
   int    opt__gra_int_scheme, opt__ref_flu_int_scheme, opt__ref_pot_int_scheme;
   int    opt__output_total, opt__output_part, opt__output_mode, output_step;
   long   end_step; 
   real   lb_wli_max, gamma, minmod_coeff, ep_coeff, elbdm_mass, planck_const, newton_g, sor_omega;
   real   mg_tolerated_error, output_part_x, output_part_y, output_part_z;
   double box_size, end_t, omega_m0, dt__fluid, dt__gravity, dt__phase, dt__max_delta_a, output_dt;

   fread( &box_size,                   sizeof(double),                  1,             File );
   fread(  nx0_tot,                    sizeof(int),                     3,             File );
   fread( &DAINO_nrank,                sizeof(int),                     1,             File );
   fread(  DAINO_nrank_x,              sizeof(int),                     3,             File );
   fread( &omp_nthread,                sizeof(int),                     1,             File );
   fread( &end_t,                      sizeof(double),                  1,             File );
   fread( &end_step,                   sizeof(long),                    1,             File );
   fread( &ooc_nrank,                  sizeof(int),                     1,             File );
   fread(  ooc_nrank_x,                sizeof(int),                     3,             File );
   fread( &omega_m0,                   sizeof(double),                  1,             File );
   fread( &dt__fluid,                  sizeof(double),                  1,             File );
   fread( &dt__gravity,                sizeof(double),                  1,             File );
   fread( &dt__phase,                  sizeof(double),                  1,             File );
   fread( &dt__max_delta_a,            sizeof(double),                  1,             File );
   fread( &opt__adaptive_dt,           sizeof(bool),                    1,             File );
   fread( &opt__dt_user,               sizeof(bool),                    1,             File );
   fread( &regrid_count,               sizeof(int),                     1,             File );
   fread( &flag_buffer_size,           sizeof(int),                     1,             File );
   fread( &max_level,                  sizeof(int),                     1,             File );
   fread( &opt__flag_rho,              sizeof(bool),                    1,             File );
   fread( &opt__flag_rho_gradient,     sizeof(bool),                    1,             File );
   fread( &opt__flag_pres_gradient,    sizeof(bool),                    1,             File );
   fread( &opt__flag_engy_density,     sizeof(bool),                    1,             File );
   fread( &opt__flag_user,             sizeof(bool),                    1,             File );
   fread( &lb_wli_max,                 sizeof(real),                    1,             File );
   fread( &gamma,                      sizeof(real),                    1,             File );
   fread( &minmod_coeff,               sizeof(real),                    1,             File );
   fread( &ep_coeff,                   sizeof(real),                    1,             File );
   fread( &opt__lr_limiter,            sizeof(int),                     1,             File );
   fread( &opt__waf_limiter,           sizeof(int),                     1,             File );
   fread( &elbdm_mass,                 sizeof(real),                    1,             File );
   fread( &planck_const,               sizeof(real),                    1,             File );
   fread( &flu_gpu_npgroup,            sizeof(int),                     1,             File );
   fread( &gpu_nstream,                sizeof(int),                     1,             File );
   fread( &opt__fixup_flux,            sizeof(bool),                    1,             File );
   fread( &opt__fixup_restrict,        sizeof(bool),                    1,             File );
   fread( &opt__overlap_mpi,           sizeof(bool),                    1,             File );
   fread( &newton_g,                   sizeof(real),                    1,             File );
   fread( &sor_omega,                  sizeof(real),                    1,             File );
   fread( &sor_max_iter,               sizeof(int),                     1,             File );
   fread( &sor_min_iter,               sizeof(int),                     1,             File );
   fread( &mg_max_iter,                sizeof(int),                     1,             File );
   fread( &mg_npre_smooth,             sizeof(int),                     1,             File );
   fread( &mg_npost_smooth,            sizeof(int),                     1,             File );
   fread( &mg_tolerated_error,         sizeof(real),                    1,             File );
   fread( &pot_gpu_npgroup,            sizeof(int),                     1,             File );
   fread( &opt__gra_p5_gradient,       sizeof(bool),                    1,             File );
   fread( &opt__int_time,              sizeof(bool),                    1,             File );
   fread( &opt__int_phase,             sizeof(bool),                    1,             File );
   fread( &opt__flu_int_scheme,        sizeof(int),                     1,             File );
   fread( &opt__pot_int_scheme,        sizeof(int),                     1,             File );
   fread( &opt__rho_int_scheme,        sizeof(int),                     1,             File );
   fread( &opt__gra_int_scheme,        sizeof(int),                     1,             File );
   fread( &opt__ref_flu_int_scheme,    sizeof(int),                     1,             File );
   fread( &opt__ref_pot_int_scheme,    sizeof(int),                     1,             File );
   fread( &opt__output_total,          sizeof(int),                     1,             File );
   fread( &opt__output_part,           sizeof(int),                     1,             File );
   fread( &opt__output_error,          sizeof(bool),                    1,             File );
   fread( &opt__output_base,           sizeof(bool),                    1,             File );
   fread( &opt__output_pot,            sizeof(bool),                    1,             File );
   fread( &opt__output_mode,           sizeof(int),                     1,             File );
   fread( &output_step,                sizeof(int),                     1,             File );
   fread( &output_dt,                  sizeof(double),                  1,             File );
   fread( &output_part_x,              sizeof(real),                    1,             File );
   fread( &output_part_y,              sizeof(real),                    1,             File );
   fread( &output_part_z,              sizeof(real),                    1,             File );
   fread( &opt__timing_barrier,        sizeof(bool),                    1,             File );
   if ( FormatVersion >= 1201 )
   fread( &opt__output_baseps,         sizeof(bool),                    1,             File );

// skip the buffer space
   fseek( File, NBuf_Parameter, SEEK_CUR );


   if ( MPI_Rank == 0 )    Aux_Message( stdout, "   Loading simulation parameters ... done\n" );


// d. check parameters (before loading any size-dependent parameters)
// =================================================================================================
   if ( MPI_Rank == 0 )    
   {
      Aux_Message( stdout, "   Checking loaded parameters ...\n" );


      const bool Fatal    = true;
      const bool NonFatal = false;

//    d-1. check the simulation options and parameters defined in the Makefile
//    ========================================================================

//    errors
//    ------------------
      CompareVar( "MODEL", model, MODEL, Fatal );

#     ifdef GRAVITY
      if ( !gravity )   
         Aux_Error( ERROR_INFO, "%s : RESTART file (%s) != runtime (%s) !!\n", "GRAVITY", "OFF", "ON" );
#     else
      if (  gravity )   
         Aux_Error( ERROR_INFO, "%s : RESTART file (%s) != runtime (%s) !!\n", "GRAVITY", "ON", "OFF" );
#     endif

#     ifdef COMOVING
      if ( !comoving )
         Aux_Error( ERROR_INFO, "%s : RESTART file (%s) != runtime (%s) !!\n", "COMOVING", "OFF", "ON" );
#     else
      if (  comoving )
         Aux_Error( ERROR_INFO, "%s : RESTART file (%s) != runtime (%s) !!\n", "COMOVING", "ON", "OFF" );
#     endif

#     ifdef FLOAT8
      if ( !float8 )
         Aux_Error( ERROR_INFO, "%s : RESTART file (%s) != runtime (%s) !!\n", "FLOAT8", "OFF", "ON" );
#     else
      if (  float8 )
         Aux_Error( ERROR_INFO, "%s : RESTART file (%s) != runtime (%s) !!\n", "FLOAT8", "ON", "OFF" );
#     endif

      if ( nlevel > NLEVEL )
         Aux_Error( ERROR_INFO, "%s : RESTART file (%d) > runtime (%d) (please set NLEVEL larger) !!\n",
                    "NLEVEL", nlevel, NLEVEL );


//    warnings 
//    ------------------
#     ifdef INDIVIDUAL_TIMESTEP
      if ( !individual_timestep )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", 
                      "INDIVIDUAL_TIMESTEP", "OFF", "ON" );
#     else
      if (  individual_timestep )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", 
                      "INDIVIDUAL_TIMESTEP", "ON", "OFF" );
#     endif

#     ifdef GPU
      if ( !gpu )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", "GPU", "OFF", "ON" );
#     else
      if (  gpu )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", "GPU", "ON", "OFF" );
#     endif

#     ifdef DAINO_OPTIMIZATION
      if ( !DAINO_optimization )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", 
                      "DAINO_OPTIMIZATION", "OFF", "ON" );
#     else
      if (  DAINO_optimization )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", 
                      "DAINO_OPTIMIZATION", "ON", "OFF" );
#     endif

#     ifdef DAINO_DEBUG
      if ( !DAINO_debug )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", 
                      "DAINO_DEBUG", "OFF", "ON" );
#     else
      if (  DAINO_debug )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", 
                      "DAINO_DEBUG", "ON", "OFF" );
#     endif

#     ifdef TIMING
      if ( !timing )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", "TIMING", "OFF", "ON" );
#     else
      if (  timing )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", "TIMING", "ON", "OFF" );
#     endif

#     ifdef TIMING_SOLVER
      if ( !timing_solver )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", 
                      "TIMING_SOLVER", "OFF", "ON");
#     else
      if (  timing_solver )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", 
                      "TIMING_SOLVER", "ON", "OFF");
#     endif

#     ifdef INTEL
      if ( !intel )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", "INTEL", "OFF", "ON" );
#     else
      if (  intel )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", "INTEL", "ON", "OFF" );
#     endif

#     ifdef SERIAL
      if ( !serial )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", "SERIAL", "OFF", "ON" );
#     else
      if (  serial )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", "SERIAL", "ON", "OFF" );
#     endif

#     ifdef OOC
      if ( !ooc )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", "OOC", "OFF", "ON" );
#     else
      if (  ooc )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", "OOC", "ON", "OFF" );
#     endif

#     ifdef OVERLAP_MPI
      if ( !overlap_mpi )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", 
                      "OVERLAP_MPI", "OFF", "ON" );
#     else
      if (  overlap_mpi )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", 
                      "OVERLAP_MPI", "ON", "OFF" );
#     endif

#     ifdef OPENMP
      if ( !openmp )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", "OPENMP", "OFF", "ON" );
#     else
      if (  openmp )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", "OPENMP", "ON", "OFF" );
#     endif

#     ifdef FERMI
      if ( !fermi )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", "FERMI", "OFF", "ON" );
#     else
      if (  fermi )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", "FERMI", "ON", "OFF" );
#     endif

#     ifdef LOAD_BALANCE
      CompareVar( "LOAD_BALANCE", load_balance, LOAD_BALANCE, NonFatal );
#     else
      if (  load_balance )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%d) != runtime (%s) !!\n", 
                      "LOAD_BALANCE", load_balance, "OFF" );
#     endif

      if ( nlevel < NLEVEL )
      {
         Aux_Message( stderr, "WARNING : %s : RESTART file (%d) < runtime (%d) !!\n", "NLEVEL", nlevel, NLEVEL );
         Aux_Message( stderr, "          --> Grid scale will be rescaled\n" );
      }

      CompareVar( "MAX_PATCH", max_patch, MAX_PATCH, NonFatal );



//    d-2. check the symbolic constants defined in "Macro.h, CUPOT.h, and CUFLU.h"
//    ========================================================================
      CompareVar( "NCOMP",                   ncomp,                  NCOMP,                        Fatal );
      CompareVar( "PATCH_SIZE",              patch_size,             PATCH_SIZE,                   Fatal );

      CompareVar( "FLU_GHOST_SIZE",          flu_ghost_size,         FLU_GHOST_SIZE,            NonFatal );
      CompareVar( "FLU_BLOCK_SIZE_X",        flu_block_size_x,       FLU_BLOCK_SIZE_X,          NonFatal );
      CompareVar( "FLU_BLOCK_SIZE_Y",        flu_block_size_y,       FLU_BLOCK_SIZE_Y,          NonFatal );
      CompareVar( "MIN_VALUE",               min_value,              MIN_VALUE,                 NonFatal );


//    check in GRAVITY
//    ----------------
#     ifdef GRAVITY

      CompareVar( "POT_SCHEME",              pot_scheme,             POT_SCHEME,                NonFatal );
      CompareVar( "POT_GHOST_SIZE",          pot_ghost_size,         POT_GHOST_SIZE,            NonFatal );
      CompareVar( "GRA_GHOST_SIZE",          gra_ghost_size,         GRA_GHOST_SIZE,            NonFatal );

#     ifdef POT_BLOCK_SIZE_X
      CompareVar( "POT_BLOCK_SIZE_X",        pot_block_size_x,       POT_BLOCK_SIZE_X,          NonFatal );
#     endif

#     ifdef POT_BLOCK_SIZE_Z
      CompareVar( "POT_BLOCK_SIZE_Z",        pot_block_size_z,       POT_BLOCK_SIZE_Z,          NonFatal );
#     endif

#     ifdef GRA_BLOCK_SIZE_Z
      CompareVar( "GRA_BLOCK_SIZE_Z",        gra_block_size_z,       GRA_BLOCK_SIZE_Z,          NonFatal );
#     endif

#     if ( POT_SCHEME == SOR )
#     ifdef USE_PSOLVER_10TO14
      if ( !use_psolver_10to14 )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", 
                      "USE_PSOLVER_10TO14", "OFF", "ON" );
#     else
      if (  use_psolver_10to14 )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", 
                      "USE_PSOLVER_10TO14", "ON", "OFF" );
#     endif
#     endif // if ( POT_SCHEME == SOR )

#     endif // #ifdef GRAVITY


//    check in HYDRO
//    ----------------
#     if ( MODEL == HYDRO )

      CompareVar( "FLU_SCHEME",              flu_scheme,             FLU_SCHEME,                NonFatal );

#     ifdef LR_SCHEME
      CompareVar( "LR_SCHEME",               lr_scheme,              LR_SCHEME,                 NonFatal );
#     endif

#     ifdef RSOLVER
      CompareVar( "RSOLVER",                 rsolver,                RSOLVER,                   NonFatal );
#     endif

#     ifdef CHECK_INTERMEDIATE
      CompareVar( "CHECK_INTERMEDIATE",      check_intermediate,     CHECK_INTERMEDIATE,        NonFatal );
#     endif

#     ifdef MAX_ERROR
      CompareVar( "MAX_ERROR",               max_error,              MAX_ERROR,                 NonFatal );
#     endif

#     ifdef ENFORCE_POSITIVE
      if ( !enforce_positive )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", 
                      "ENFORCE_POSITIVE", "OFF", "ON" );
#     else
      if (  enforce_positive )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", 
                      "ENFORCE_POSITIVE", "ON", "OFF" );
#     endif

#     ifdef CHAR_RECONSTRUCTION
      if ( !char_reconstruction )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", 
                      "CHAR_RECONSTRUCTION", "OFF", "ON" );
#     else
      if (  char_reconstruction )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", 
                      "CHAR_RECONSTRUCTION", "ON", "OFF" );
#     endif

#     ifdef HLL_NO_REF_STATE
      if ( !hll_no_ref_state )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", 
                      "HLL_NO_REF_STATE", "OFF", "ON" );
#     else
      if (  hll_no_ref_state )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", 
                      "HLL_NO_REF_STATE", "ON", "OFF" );
#     endif

#     ifdef HLL_INCLUDE_ALL_WAVES
      if ( !hll_include_all_waves )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", 
                      "HLL_INCLUDE_ALL_WAVES", "OFF", "ON" );
#     else
      if (  hll_include_all_waves )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", 
                      "HLL_INCLUDE_ALL_WAVES", "ON", "OFF" );
#     endif

#     ifdef WAF_DISSIPATE
      if ( !waf_dissipate )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", 
                      "WAF_DISSIPATE", "OFF", "ON" );
#     else
      if (  waf_dissipate )
         Aux_Message( stderr, "WARNING : %s : RESTART file (%s) != runtime (%s) !!\n", 
                      "WAF_DISSIPATE", "ON", "OFF" );
#     endif


//    check in MHD 
//    ----------------
#     elif ( MODEL == MHD )
#     warning : WAIT MHD !!!


//    check in ELBDM
//    ----------------
#     elif ( MODEL == ELBDM )
//    nothing to do here

#     else
#     error : ERROR : unsupported MODEL !!
#     endif // MODEL



//    d-3. check the simulation parameters recorded in the file "Input__Parameter"
//    =================================================================================================

//    errors
//    ------------------
      CompareVar( "BOX_SIZE",                box_size,                     BOX_SIZE,                     Fatal );
      CompareVar( "NX0_TOT[0]",              nx0_tot[0],                   NX0_TOT[0],                   Fatal );
      CompareVar( "NX1_TOT[1]",              nx0_tot[1],                   NX0_TOT[1],                   Fatal );
      CompareVar( "NX2_TOT[2]",              nx0_tot[2],                   NX0_TOT[2],                   Fatal );


//    warnings 
//    ------------------
      CompareVar( "DAINO_NRANK",             DAINO_nrank,                  DAINO_NRANK,               NonFatal );
      CompareVar( "DAINO_NRANK_X[0]",        DAINO_nrank_x[0],             DAINO_NRANK_X(0),          NonFatal );
      CompareVar( "DAINO_NRANK_X[1]",        DAINO_nrank_x[1],             DAINO_NRANK_X(1),          NonFatal );
      CompareVar( "DAINO_NRANK_X[2]",        DAINO_nrank_x[2],             DAINO_NRANK_X(2),          NonFatal );
      CompareVar( "OMP_NTHREAD",             omp_nthread,                  OMP_NTHREAD,               NonFatal );
      CompareVar( "END_T",                   end_t,                        END_T,                     NonFatal );
      CompareVar( "END_STEP",                end_step,                     END_STEP,                  NonFatal );
      CompareVar( "DT__FLUID",               dt__fluid,                    DT__FLUID,                 NonFatal );
      CompareVar( "OPT__ADAPTIVE_DT",        opt__adaptive_dt,             OPT__ADAPTIVE_DT,          NonFatal );
      CompareVar( "OPT__DT_USER",            opt__dt_user,                 OPT__DT_USER,              NonFatal );
      CompareVar( "REGRID_COUNT",            regrid_count,                 REGRID_COUNT,              NonFatal );
      CompareVar( "FLAG_BUFFER_SIZE",        flag_buffer_size,             FLAG_BUFFER_SIZE,          NonFatal );
      CompareVar( "MAX_LEVEL",               max_level,                    MAX_LEVEL,                 NonFatal );
      CompareVar( "OPT__FLAG_RHO",           OPT__FLAG_RHO,                OPT__FLAG_RHO,             NonFatal );
      CompareVar( "OPT__FLAG_RHO_GRADIENT",  opt__flag_rho_gradient,       OPT__FLAG_RHO_GRADIENT,    NonFatal );
      CompareVar( "OPT__FLAG_USER",          opt__flag_user,               OPT__FLAG_USER,            NonFatal );
      CompareVar( "FLU_GPU_NPGROUP",         flu_gpu_npgroup,              FLU_GPU_NPGROUP,           NonFatal );
      CompareVar( "GPU_NSTREAM",             gpu_nstream,                  GPU_NSTREAM,               NonFatal );
      CompareVar( "OPT__FIXUP_FLUX",         opt__fixup_flux,              OPT__FIXUP_FLUX,           NonFatal );
      CompareVar( "OPT__FIXUP_RESTRICT",     opt__fixup_restrict,          OPT__FIXUP_RESTRICT,       NonFatal );
      CompareVar( "OPT__OVERLAP_MPI",        opt__overlap_mpi,             OPT__OVERLAP_MPI,          NonFatal );
      CompareVar( "OPT__INT_TIME",           opt__int_time,                OPT__INT_TIME,             NonFatal );
      CompareVar( "OPT__FLU_INT_SCHEME",     opt__flu_int_scheme,     (int)OPT__FLU_INT_SCHEME,       NonFatal );
      CompareVar( "OPT__REF_FLU_INT_SCHEME", opt__ref_flu_int_scheme, (int)OPT__REF_FLU_INT_SCHEME,   NonFatal );
      CompareVar( "OPT__OUTPUT_TOTAL",       opt__output_total,            OPT__OUTPUT_TOTAL,         NonFatal );
      CompareVar( "OPT__OUTPUT_PART",        opt__output_part,        (int)OPT__OUTPUT_PART,          NonFatal );
      CompareVar( "OPT__OUTPUT_ERROR",       opt__output_error,            OPT__OUTPUT_ERROR,         NonFatal );
      if ( FormatVersion >= 1201 )
      CompareVar( "OPT__OUTPUT_BASEPS",      opt__output_baseps,           OPT__OUTPUT_BASEPS,        NonFatal );
      if ( OPT__OUTPUT_PART )
      CompareVar( "OPT__OUTPUT_BASE",        opt__output_base,             OPT__OUTPUT_BASE,          NonFatal );
      if ( OPT__OUTPUT_TOTAL  ||  OPT__OUTPUT_PART  ||  OPT__OUTPUT_ERROR  ||  OPT__OUTPUT_BASEPS ) {
      CompareVar( "OPT__OUTPUT_MODE",        opt__output_mode,        (int)OPT__OUTPUT_MODE,          NonFatal );
      CompareVar( "OUTPUT_STEP",             output_step,                  OUTPUT_STEP,               NonFatal );
      CompareVar( "OUTPUT_DT",               output_dt,                    OUTPUT_DT,                 NonFatal );}
      if ( OPT__OUTPUT_PART ) {
      CompareVar( "OUTPUT_PART_X",           output_part_x,                OUTPUT_PART_X,             NonFatal );
      CompareVar( "OUTPUT_PART_Y",           output_part_y,                OUTPUT_PART_Y,             NonFatal );
      CompareVar( "OUTPUT_PART_Z",           output_part_z,                OUTPUT_PART_Z,             NonFatal );}
      CompareVar( "OPT__TIMING_BARRIER",     opt__timing_barrier,          OPT__TIMING_BARRIER,       NonFatal );

#     ifdef OOC
      CompareVar( "OOC_NRANK",               ooc_nrank,                    ooc.NRank,                 NonFatal );
      CompareVar( "OOC_NRANK_X[0]",          ooc_nrank_x[0],               ooc.NRank_X[0],            NonFatal );
      CompareVar( "OOC_NRANK_X[1]",          ooc_nrank_x[1],               ooc.NRank_X[1],            NonFatal );
      CompareVar( "OOC_NRANK_X[2]",          ooc_nrank_x[2],               ooc.NRank_X[2],            NonFatal );
#     endif

#     ifdef COMOVING
      CompareVar( "OMEGA_M0",                omega_m0,                     OMEGA_M0,                  NonFatal );
      CompareVar( "DT__MAX_DELTA_A",         dt__max_delta_a,              DT__MAX_DELTA_A,           NonFatal );
#     endif

#     ifdef LOAD_BALANCE
      CompareVar( "LB->WLI_Max",             lb_wli_max,                   patch->LB->WLI_Max,        NonFatal );
#     endif

#     ifdef GRAVITY
      CompareVar( "DT__GRAVITY",             dt__gravity,                  DT__GRAVITY,               NonFatal );
      CompareVar( "NEWTON_G",                newton_g,                     NEWTON_G,                  NonFatal );
      CompareVar( "POT_GPU_NPGROUP",         pot_gpu_npgroup,              POT_GPU_NPGROUP,           NonFatal );
      CompareVar( "OPT__GRA_P5_GRADIENT",    opt__gra_p5_gradient,         OPT__GRA_P5_GRADIENT,      NonFatal );
      CompareVar( "OPT__POT_INT_SCHEME",     opt__pot_int_scheme,     (int)OPT__POT_INT_SCHEME,       NonFatal );
      CompareVar( "OPT__RHO_INT_SCHEME",     opt__rho_int_scheme,     (int)OPT__RHO_INT_SCHEME,       NonFatal );
      CompareVar( "OPT__GRA_INT_SCHEME",     opt__gra_int_scheme,     (int)OPT__GRA_INT_SCHEME,       NonFatal );
      CompareVar( "OPT__REF_POT_INT_SCHEME", opt__ref_pot_int_scheme, (int)OPT__REF_POT_INT_SCHEME,   NonFatal );
      CompareVar( "OPT__OUTPUT_POT",         opt__output_pot,              OPT__OUTPUT_POT,           NonFatal );
#     if   ( POT_SCHEME == SOR )
      CompareVar( "SOR_OMEGA",               sor_omega,                    SOR_OMEGA,                 NonFatal );
      CompareVar( "SOR_MAX_ITER",            sor_max_iter,                 SOR_MAX_ITER,              NonFatal );
      CompareVar( "SOR_MIN_ITER",            sor_min_iter,                 SOR_MIN_ITER,              NonFatal );
#     elif ( POT_SCHEME == MG )
      CompareVar( "MG_MAX_ITER",             mg_max_iter,                  MG_MAX_ITER,               NonFatal );
      CompareVar( "MG_NPRE_SMOOTH",          mg_npre_smooth,               MG_NPRE_SMOOTH,            NonFatal );
      CompareVar( "MG_NPOST_SMOOTH",         mg_npost_smooth,              MG_NPOST_SMOOTH,           NonFatal );
      CompareVar( "MG_TOLERATED_ERROR",      mg_tolerated_error,           MG_TOLERATED_ERROR,        NonFatal );
#     endif // POT_SCHEME
#     endif // #ifdef GRAVITY

#     if   ( MODEL == HYDRO )
      CompareVar( "OPT__FLAG_PRES_GRADIENT", opt__flag_pres_gradient,      OPT__FLAG_PRES_GRADIENT,   NonFatal );
      CompareVar( "GAMMA",                   gamma,                        GAMMA,                     NonFatal );
      CompareVar( "MINMOD_COEFF",            minmod_coeff,                 MINMOD_COEFF,              NonFatal );
      CompareVar( "EP_COEFF",                ep_coeff,                     EP_COEFF,                  NonFatal );
      CompareVar( "OPT__LR_LIMITER",         opt__lr_limiter,         (int)OPT__LR_LIMITER,           NonFatal );
      CompareVar( "OPT__WAF_LIMITER",        opt__waf_limiter,        (int)OPT__WAF_LIMITER,          NonFatal );

#     elif ( MODEL == MHD )
#     warning : WAIT MHD !!!

#     elif ( MODEL == ELBDM )
      CompareVar( "DT__PHASE",               dt__phase,                    DT__PHASE,                 NonFatal );
      CompareVar( "OPT__FLAG_ENGY_DENSITY",  opt__flag_engy_density,       OPT__FLAG_ENGY_DENSITY,    NonFatal );
      CompareVar( "OPT__INT_PHASE",          opt__int_phase,               OPT__INT_PHASE,            NonFatal );
      CompareVar( "ELBDM_MASS",              elbdm_mass,                   ELBDM_MASS,                NonFatal );
      CompareVar( "PLANCK_CONST",            planck_const,                 PLANCK_CONST,              NonFatal );

#     else
#     error : ERROR : unsupported MODEL !!
#     endif


      Aux_Message( stdout, "   Checking loaded parameters ... done\n" );

   } // if ( MPI_Rank == 0 )


// set the returned variables
   DataOrder_xyzv = ( opt__output_total == 1 ) ? true : false;
   LoadPot        = opt__output_pot;
   NLv_Restart    = nlevel;

} // FUNCTION : Load_Parameter_After_1200



//-------------------------------------------------------------------------------------------------------
// Function    :  CompareVar 
// Description :  Compare the input variables 
//
// Note        :  This function is overloaded to work with different data types
//
// Parameter   :  VarName     : Name of the targeted variable 
//                RestartVar  : Variable loaded from the RESTART file
//                RuntimeVar  : Variable loaded from the Input__Parameter
//                Fatal       : Whether or not the difference between RestartVar and RuntimeVar is fatal
//                              --> true  : terminate the program if the input variables are different
//                                  false : display warning message if the input variables are different
//-------------------------------------------------------------------------------------------------------
void CompareVar( const char *VarName, const bool RestartVar, const bool RuntimeVar, const bool Fatal )
{

   if ( RestartVar != RuntimeVar )
   {
      if ( Fatal )
         Aux_Error( ERROR_INFO, "%s : RESTART file (%d) != runtime (%d) !!\n", 
                    VarName, RestartVar, RuntimeVar );
      else
         Aux_Message( stderr, "WARNING : %s : RESTART file (%d) != runtime (%d) !!\n", 
                      VarName, RestartVar, RuntimeVar );
   }

} // FUNCTION : CompareVar (bool)



//-------------------------------------------------------------------------------------------------------
// overloaded function for type "int"
//-------------------------------------------------------------------------------------------------------
void CompareVar( const char *VarName, const int RestartVar, const int RuntimeVar, const bool Fatal )
{

   if ( RestartVar != RuntimeVar )
   {
      if ( Fatal )
         Aux_Error( ERROR_INFO, "%s : RESTART file (%d) != runtime (%d) !!\n", 
                    VarName, RestartVar, RuntimeVar );
      else
         Aux_Message( stderr, "WARNING : %s : RESTART file (%d) != runtime (%d) !!\n", 
                      VarName, RestartVar, RuntimeVar );
   }

} // FUNCTION : CompareVar (int)



//-------------------------------------------------------------------------------------------------------
// overloaded function for type "long"
//-------------------------------------------------------------------------------------------------------
void CompareVar( const char *VarName, const long RestartVar, const long RuntimeVar, const bool Fatal )
{

   if ( RestartVar != RuntimeVar )
   {
      if ( Fatal )
         Aux_Error( ERROR_INFO, "%s : RESTART file (%ld) != runtime (%ld) !!\n", 
                    VarName, RestartVar, RuntimeVar );
      else
         Aux_Message( stderr, "WARNING : %s : RESTART file (%ld) != runtime (%ld) !!\n", 
                      VarName, RestartVar, RuntimeVar );
   }

} // FUNCTION : CompareVar (long)



//-------------------------------------------------------------------------------------------------------
// overloaded function for type "float"
//-------------------------------------------------------------------------------------------------------
void CompareVar( const char *VarName, const float RestartVar, const float RuntimeVar, const bool Fatal )
{

   if ( RestartVar != RuntimeVar )
   {
      if ( Fatal )
         Aux_Error( ERROR_INFO, "%s : RESTART file (%20.14e) != runtime (%20.14e) !!\n", 
                    VarName, RestartVar, RuntimeVar );
      else
         Aux_Message( stderr, "WARNING : %s : RESTART file (%20.14e) != runtime (%20.14e) !!\n", 
                      VarName, RestartVar, RuntimeVar );
   }

} // FUNCTION : CompareVar (float)



//-------------------------------------------------------------------------------------------------------
// overloaded function for type "double"
//-------------------------------------------------------------------------------------------------------
void CompareVar( const char *VarName, const double RestartVar, const double RuntimeVar, const bool Fatal )
{

   if ( RestartVar != RuntimeVar )
   {
      if ( Fatal )
         Aux_Error( ERROR_INFO, "%s : RESTART file (%20.14e) != runtime (%20.14e) !!\n", 
                    VarName, RestartVar, RuntimeVar );
      else
         Aux_Message( stderr, "WARNING : %s : RESTART file (%20.14e) != runtime (%20.14e) !!\n", 
                      VarName, RestartVar, RuntimeVar );
   }

} // FUNCTION : CompareVar (double)
