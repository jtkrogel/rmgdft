/************************** SVN Revision Information **************************
 **    $Id$    **
******************************************************************************/
 
/*

                global_sums.c

  Sums an array over all processors. 
  For serial machines it just returns.



*/

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "main.h"
#include "prototypes_on.h"




void comm_sums (double * vect, int *length, MPI_Comm COMM_TEM)
{
    int sizr, steps, blocks, newsize;
    double *rptr, *rptr1;
    double rptr2[100];

    /* Check for small vector case and handle on stack */
    if (*length < 100)
    {
        sizr = *length;
        QMD_dcopy (sizr, vect, 1, rptr2, 1);
        MPI_Allreduce (rptr2, vect, sizr, MPI_DOUBLE, MPI_SUM, COMM_TEM);
        return;
    }


    my_malloc_init( rptr, MAX_PWRK, double );
    newsize = MAX_PWRK;
    blocks = *length / newsize;
    sizr = (*length % newsize);

    rptr1 = vect;

    for (steps = 0; steps < blocks; steps++)
    {
        QMD_dcopy (newsize, rptr1, 1, rptr, 1);
        MPI_Allreduce (rptr, rptr1, newsize, MPI_DOUBLE, MPI_SUM, COMM_TEM);

        rptr1 += newsize;
    }


    if (sizr)
    {
        QMD_dcopy (sizr, rptr1, 1, rptr, 1);
        MPI_Allreduce (rptr, rptr1, sizr, MPI_DOUBLE, MPI_SUM, COMM_TEM);
    }


    my_free(rptr);

}                               /* end global_sums */


void global_sums_X (double * vect, int *length)
{
    int sizr, steps, blocks, newsize;
    double *rptr, *rptr1;
    double rptr2[100];

    /* Check for small vector case and handle on stack */
    if (*length < 100)
    {
        sizr = *length;
        QMD_dcopy (sizr, vect, 1, rptr2, 1);
        MPI_Allreduce (rptr2, vect, sizr, MPI_DOUBLE, MPI_SUM, COMM_PEX);
        return;
    }


    my_malloc_init( rptr, MAX_PWRK, double );
    newsize = MAX_PWRK;
    blocks = *length / newsize;
    sizr = (*length % newsize);

    rptr1 = vect;

    for (steps = 0; steps < blocks; steps++)
    {
        QMD_dcopy (newsize, rptr1, 1, rptr, 1);
        MPI_Allreduce (rptr, rptr1, newsize, MPI_DOUBLE, MPI_SUM, COMM_PEX);

        rptr1 += newsize;
    }


    if (sizr)
    {
        QMD_dcopy (sizr, rptr1, 1, rptr, 1);
        MPI_Allreduce (rptr, rptr1, sizr, MPI_DOUBLE, MPI_SUM, COMM_PEX);
    }


    my_free(rptr);

}                               /* end global_sums */


void global_sums_int (int *vect, int *length)
{

    int sizr, steps, blocks, newsize;
    int idx;
    int *rptr, *rptr1;

    MPI_Barrier(pct.img_comm);
    /* Check for small vector case and handle on stack */
    if (*length < 100)
    {

        sizr = *length;
        my_malloc( rptr, sizr, int );
        for (idx = 0; idx < sizr; idx++)
            rptr[idx] = vect[idx];

        MPI_Allreduce (rptr, vect, sizr, MPI_INTEGER, MPI_SUM, pct.grid_comm);
        my_free(rptr);
        return;
    }


    my_malloc_init( rptr, MAX_PWRK, int );
    newsize = MAX_PWRK;
    blocks = *length / newsize;
    sizr = (*length % newsize);

    rptr1 = vect;

    for (steps = 0; steps < blocks; steps++)
    {

        for (idx = 0; idx < newsize; idx++)
            rptr[idx] = rptr1[idx];
        MPI_Allreduce (rptr, rptr1, newsize, MPI_INTEGER, MPI_SUM, pct.grid_comm);

        rptr1 += newsize;
    }


    if (sizr)
    {
        for (idx = 0; idx < sizr; idx++)
            rptr[idx] = rptr1[idx];

        MPI_Allreduce (rptr, rptr1, sizr, MPI_INTEGER, MPI_SUM, pct.grid_comm);
    }


    my_free(rptr);


}                               /* end global_sums_int */



