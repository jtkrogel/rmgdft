/************************** SVN Revision Information **************************
 **    $Id$    **
******************************************************************************/
 
#include "main.h"
#include "prototypes_on.h"
#include <float.h>
#include <math.h>



void fill_orbit_borders4(double * sg, double * pg, int dimx, int dimy, int dimz)
{

    int ix, iy, iz;
    int incx, incy, incxs, incys;
    incys = dimz;
    incxs = dimy * dimz;
    incx = (dimy + 8) * (dimz + 8);
    incy = dimz + 8;


    for (ix = 0; ix < dimx + 8; ix++)
        for (iy = 0; iy < dimy + 8; iy++)
            for (iz = 0; iz < dimz + 8; iz++)
                sg[ix * incx + iy * incy + iz] = 0.0;

/* Load up original values from pg  */

    for (ix = 0; ix < dimx; ix++)
        for (iy = 0; iy < dimy; iy++)
            for (iz = 0; iz < dimz; iz++)
                sg[(ix + 4) * incx + (iy + 4) * incy + iz + 4] = pg[ix * incxs + iy * incys + iz];


}                               /* end fill_orbit_borders3.c */

/******/
