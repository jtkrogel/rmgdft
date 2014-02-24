/************************** SVN Revision Information **************************
 **    $Id$    **
******************************************************************************/
 
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "prototypes_on.h"

void assign_weight_on(SPECIES * sp, fftw_complex * weptr, rmg_double_t * rtptr)
{

    int idx, ix, iy, iz;

    idx = 0;
    for (ix = 0; ix < sp->nldim; ix++)
    {

        for (iy = 0; iy < sp->nldim; iy++)
        {

            for (iz = 0; iz < sp->nldim; iz++)
            {

                rtptr[idx] = weptr[idx].re;
                if (fabs(weptr[idx].im) > 1.0e-6)
                {
                    printf("weptr[%d].im=%e\n", idx, weptr[idx].im);
                    error_handler("something wrong with the fourier transformation");
                }

                idx++;
            }                   /* end for */
        }                       /* end for */
    }                           /* end for */
}
