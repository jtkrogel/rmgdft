/************************** SVN Revision Information **************************
 **    $Id$    **
******************************************************************************/
 
/*

	Requires the predefined parameters:
		NN:	size of the global matrix
		NPES:	number of processors (CFLAGS)

        desca and ictxt are initialized in init_pe.c

Documentation:

	LAPACK Working Note 94, A User's Guide to the BLACS v1.1

	Manpages: DESCINIT

	WWW: http://www.netlib.org/scalapack/slug/scalapack_slug.html
	(ScaLapack user's guide)



			Fattebert J.-L., November 98 
			fatteber@nemo.physics.ncsu.edu                  

*/


#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <assert.h>
#include "main.h"
#include "prototypes_on.h"
#include "blacs.h"


#define globalexit  exit


/*
*  Purpose
*  =======
*
*  SL_INIT initializes an pct.scalapack_nprow x pct.scalapack_npcol process grid using a row-major
*  ordering  of  the  processes. This routine retrieves a default system
*  context  which  will  include all available processes. In addition it
*  spawns the processes if needed.
*
*  Arguments
*  =========
*
*  ictxt   (global output) int
*          ictxt specifies the BLACS context handle identifying 
*          created process grid.  The context itself is global.
*
*  pct.scalapack_nprow   (global input) int
*          pct.scalapack_nprow specifies the number of process rows in the grid
*          to be created.
*
*  pct.scalapack_npcol   (global input) int
*          pct.scalapack_npcol specifies the number of process columns in the grid
*          to be created.
*
*  ============================================================
*  =====================================================================
*/
void sl_init_on(int *ictxt, int nprow, int npcol)
{
    int i;
    /*char    order='R'; */
    int *pmap;

    assert(nprow > 0);
    assert(npcol > 0);


/*
 *  Define process grid
 */

    Cblacs_get(0, 0, ictxt);


    my_malloc_init( pmap, pct.scalapack_nprow * pct.scalapack_npcol, int );
    for (i = 0; i < nprow * npcol; i++)
        pmap[i] = i;

    Cblacs_gridmap(ictxt, pmap, nprow, nprow, npcol);
    my_free(pmap);

}




void diaginit(double *aa, int *desca, double *a, int lda)
{
    int i, ii, iii, li, maxli, iistart;
    int mycol, myrow, nprow, npcol;
    int ictxt = desca[1], mb = desca[4], mxllda = desca[8];



    Cblacs_gridinfo(ictxt, &nprow, &npcol, &myrow, &mycol);

    for (i = 0; i < mxllda * mxllda; i++)
        aa[i] = 0.;

    maxli = (lda / (nprow * mb)) + 1;

    if (myrow == mycol)
        for (li = 0; li < maxli; li++)
        {

            iistart = (li * nprow + myrow) * mb;

            for (i = 0; i < mb; i++)
            {

                ii = iistart + i;
                iii = i + li * mb;

                if (iii < mxllda && ii < lda)
                {

                    aa[iii * (mxllda + 1)] = a[ii];
                }

            }
        }

}


/*
*
*     MATGATHER generates and distributes matrice a
*
*/




/********************************************************************/


void dsymm_dis(char *side, char *uplo, int *nn, double *aa, double *bb, double *cc)
{
    char *char_fcd1;
    char *char_fcd2;
    double zero = 0., one = 1.;
    int ione = 1;


    /* If I'm in the process grid, execute the program */
    if (pct.scalapack_myrow != -1)
    {

        char_fcd1 = side;
        char_fcd2 = uplo;

        pdsymm(char_fcd1, char_fcd2, nn, nn,
               &one, aa, &ione, &ione, pct.desca,
               bb, &ione, &ione, pct.desca, &zero, cc, &ione, &ione, pct.desca);

    }


}


