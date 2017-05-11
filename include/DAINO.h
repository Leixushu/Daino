
#ifndef __DAINO_H__
#define __DAINO_H__



#ifndef DAINO_DEBUG
#  define NDEBUG
#endif

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <cmath>
#include <malloc.h>
#include <stdio.h>
#include <unistd.h>

#ifndef SERIAL
#  include <mpi.h>
#endif

#ifdef OPENMP
#  include <omp.h>
#endif

#ifdef GRAVITY
#  ifdef FLOAT8
#     ifdef SERIAL
#        include <drfftw.h>
#     else
#        include <drfftw_mpi.h>
#     endif
#  else
#     ifdef SERIAL
#        include <srfftw.h>
#     else
#        include <srfftw_mpi.h>
#     endif
#  endif
#endif

using namespace std;

#include "Macro.h"
#include "Typedef.h"
#include "AMR.h"
#include "Timer.h"
#include "Global.h"
#include "Prototype.h"

#ifdef SERIAL
#  include "Serial.h"
#endif

#ifdef OOC
#  include "OOC.h"
#endif

#ifdef __CUDACC__
#  include "CUAPI.h"
#endif



#endif // #ifndef __DAINO_H__
