/************************** SVN Revision Information **************************
 **    $Id$    **
******************************************************************************/

/****f* QMD-MGDFT/rft1.c *****
 * NAME
 *   Ab initio real space code with multigrid acceleration
 *   Quantum molecular dynamics package.
 *   Version: 2.1.5
 * COPYRIGHT
 *   Copyright (C) 1995  Emil Briggs
 *   Copyright (C) 1998  Emil Briggs, Charles Brabec, Mark Wensell, 
 *                       Dan Sullivan, Chris Rapcewicz, Jerzy Bernholc
 *   Copyright (C) 2001  Emil Briggs, Wenchang Lu,
 *                       Marco Buongiorno Nardelli,Charles Brabec, 
 *                       Mark Wensell,Dan Sullivan, Chris Rapcewicz,
 *                       Jerzy Bernholc
 * FUNCTION
 *   void rft1(double cparm, double *f, double *r, double *ffil, double al, int rg_points, 
  *           int lval, double dr, double width, int lrg_points)
 *   This function is used to filter the high frequencies from a radial function 
 *   defined on a logarithmic grid. It computes a DFT, applies a cutoff function
 *   defined in gcutoff to the transform and then reconstructs the filtered function
 *   on a linear grid. (The linear grid is used for fast interpolation of the
 *   radial function onto a 3-dimensional grid).
 *
 *   The radial function must be short ranged.
 * INPUTS
 *   f: a radial function to be filtered
 *   r: array stored r values
 *   al: log mesh parameter
 *   rg_points: number of radial mesh
 *   lval: momentum l (0=s, 1=p, 2=d)
 *   dr: linear grid space
 *   width: width for G-cutoff
 *   lrg_point: number of linear grid 
 * OUTPUT
 *   ffil:  filtered potential in linear grid
 * PARENTS
 *   init_kbr.c
 * CHILDREN
 *   global_sums.c radint.c
 * SOURCE
 */


#include <float.h>
#include <math.h>
#include <stdlib.h>
#include "grid.h"
#include "main.h"

#define SMALL 1.0e-8


/* G-vector cutoff function */
double gcutoff (double g1, double gcut, double width)
{

    double t1;

    if (g1 < gcut)
        return ONE;

    t1 = (g1 - gcut) / gcut;
    return exp (-width * (t1 * t1));


}


void rft1 (double cparm, double * f, double * r, double * ffil, double * rab, int rg_points,
           int lval, double dr, double width, int lrg_points)
{

    int idx, ift, gnum, istep, alloc;
    double gmesh, gmax, gcut, t1, t2, *work1, *work2, *gcof, *gvec;
    double rfil, rstep;
    int npes = get_PE_X() * get_PE_Y() * get_PE_Z();


    /* Get some temporary memory */
    alloc = rg_points;
    if (alloc < lrg_points)
        alloc = lrg_points;
    my_malloc (work1, alloc, double);
    my_malloc (work2, alloc, double);
    my_malloc (gcof, alloc, double);
    my_malloc (gvec, alloc, double);

    for (idx = 0; idx < alloc; idx++)
    {
        work1[idx] = 0.0;
        work2[idx] = 0.0;
        gcof[idx] = 0.0;
        gvec[idx] = 0.0;
    }
    gnum = 1500;

    /* G-vectors are defined on a log grid with the smallest value set */
    /* by the largest real-space value of r.                           */
    gvec[0] = PI / (r[rg_points - 1]);

    /* The largest g-vector that we generate is defined by the global */
    /* grid spacing.                                                  */
    gcut = PI / (cparm * ct.hmaxgrid);
    gmax = PI / r[0];

    gmesh = (log (gmax) - log (gvec[0])) / gnum;
    t1 = exp (gmesh);


    /* Generate g-vectors */
    for (idx = 1; idx < gnum; idx++)
    {

        gvec[idx] = gvec[0] * pow (t1, (double) idx);

    }                           /* end for */




    /* Loop over frequency components */
    for (ift = 0; ift < gnum; ift++)
    {


        switch (lval)
        {

        case S_STATE:

            for (idx = 0; idx < rg_points; idx++)
            {

                t1 = r[idx] * gvec[ift];

                if (t1 > SMALL)
                    work1[idx] = f[idx] * sin (t1) / t1;
                else
                    work1[idx] = f[idx];
                /*t2 = sin(t1) / t1;
                   if(t1 < 1.0e-8) t2 = 1.0;
                   work1[idx] = f[idx] * t2; */

            }                   /* end for */

            break;

        case P_STATE:

            for (idx = 0; idx < rg_points; idx++)
            {

                t1 = r[idx] * gvec[ift];
                if (t1 > SMALL)
                    t2 = cos (t1) / t1 - sin (t1) / (t1 * t1);
                else
                    t2 = 0.0;
                /*if(t1 < 1.0e-8) t2 = 0.0; */
                work1[idx] = f[idx] * t2;

            }                   /* end for */

            break;

        case D_STATE:

            for (idx = 0; idx < rg_points; idx++)
            {

                t1 = r[idx] * gvec[ift];
                if (t1 > SMALL)
                {
                    t2 = (THREE / (t1 * t1) - ONE) * sin (t1) / t1;
                    t2 -= THREE * cos (t1) / (t1 * t1);
                }
                else
                    t2 = 0.0;
                /*if(t1 < 1.0e-8) t2 = 0.0; */
                work1[idx] = f[idx] * t2;

            }                   /* end for */

            break;

        case F_STATE:

            for (idx = 0; idx < rg_points; idx++)
            {
                t1 = r[idx] * gvec[ift];
                if (t1 > SMALL)
                {
                    t2 = (15.0 / (t1 * t1) - 6.0) * sin (t1) / (t1 * t1);
                    t2 += (1.0 - 15.0 / (t1 * t1)) * cos (t1) / t1;
                }
                else
                    t2 = 0.0;
                /*if(t1 < 1.0e-8) t2 = 0.0; */
                work1[idx] = f[idx] * t2;
            }                   /* end for */

            break;

        case G_STATE:

            for (idx = 0; idx < rg_points; idx++)
            {
                t1 = r[idx] * gvec[ift];
                if (t1 > SMALL)
                {
                    t2 = (105.0 / (t1 * t1 * t1 * t1) - 45.0 / (t1 * t1) + 1.0) * sin (t1) / t1;
                    t2 += (10.0 - 105.0 / (t1 * t1)) * cos (t1) / (t1 * t1);
                }
                else
                    t2 = 0.0;
                /*if(t1 < 1.0e-8) t2 = 0.0; */
                work1[idx] = f[idx] * t2;
            }                   /*end for */

            break;

        default:

            error_handler ("angular momentum l=%d is not programmed", lval);


        }                       /* end switch */


        /* Get coefficients */
        gcof[ift] = radint1 (work1, r, rab, rg_points);

    }                           /* end for */

    /* Fouerier Filter the transform and store in work2 */
    for (idx = 0; idx < gnum; idx++)
    {

        work2[idx] = gcof[idx] * gcutoff (gvec[idx], gcut, width);

    }                           /* end for */


    /* Zero out the array in which the filtered function is to be returned */
    for (idx = 0; idx < lrg_points; idx++)
    {

        ffil[idx] = ZERO;

    }                           /* end for */



    /* Now we reconstruct the filtered function */
    istep = lrg_points / npes;
    t1 = (double) istep;
    rstep = t1 * dr;
    t1 = (double) pct.gridpe;
    rfil = t1 * rstep + 1.0e-10;

    switch (lval)
    {

    case S_STATE:

        for (idx = istep * pct.gridpe; idx < istep * pct.gridpe + istep; idx++)
        {


            for (ift = 0; ift < gnum; ift++)
            {

                t1 = rfil * gvec[ift];
                if (t1 > SMALL)
                    t2 = sin (t1) / t1;
                else
                    t2 = 1.0;
                /*if(t1 < 1.0e-8) t2 = 1.0; */

                /* G-space cutoff */
                work1[ift] = work2[ift] * t2;

            }                   /* end for */


            /* Integrate it */
            t1 = radint (work1, gvec, gnum, gmesh);
            ffil[idx] = t1 * TWO / PI;

            rfil += dr;

        }                       /* end for */


        istep = npes * istep;
        global_sums (ffil, &istep, pct.grid_comm);

        t1 = (double) npes;
        rfil = t1 * rstep + 1.0e-10;

        for (idx = istep; idx < lrg_points; idx++)
        {


            for (ift = 0; ift < gnum; ift++)
            {

                t1 = rfil * gvec[ift];
                if (t1 > SMALL)
                    t2 = sin (t1) / t1;
                else
                    t2 = 1.0;
                /*if(t1 < 1.0e-8) t2 = 1.0; */

                /* G-space cutoff */
                work1[ift] = work2[ift] * t2;

            }                   /* end for */


            /* Integrate it */
            t1 = radint (work1, gvec, gnum, gmesh);
            ffil[idx] = t1 * TWO / PI;

            rfil += dr;

        }                       /* end for */




        break;

    case P_STATE:

        for (idx = istep * pct.gridpe; idx < istep * pct.gridpe + istep; idx++)
        {


            for (ift = 0; ift < gnum; ift++)
            {

                t1 = rfil * gvec[ift];
                if (t1 > SMALL)
                    t2 = cos (t1) / t1 - sin (t1) / (t1 * t1);
                else
                    t2 = 0.0;
                /*if(t1 < 1.0e-8) t2 = 0.0; */

                /* G-space cutoff */
                work1[ift] = work2[ift] * t2;

            }                   /* end for */


            /* Integrate it */
            t1 = radint (work1, gvec, gnum, gmesh);
            ffil[idx] = t1 * TWO / PI;

            rfil += dr;

        }                       /* end for */


        istep = npes * istep;
        global_sums (ffil, &istep, pct.grid_comm);

        t1 = (double) npes;
        rfil = t1 * rstep + 1.0e-10;

        for (idx = istep; idx < lrg_points; idx++)
        {

            for (ift = 0; ift < gnum; ift++)
            {

                t1 = rfil * gvec[ift];
                if (t1 > SMALL)
                    t2 = cos (t1) / t1 - sin (t1) / (t1 * t1);
                else
                    t2 = 0.0;
                /*if(t1 < 1.0e-8) t2 = 0.0; */

                /* G-space cutoff */
                work1[ift] = work2[ift] * t2;

            }                   /* end for */


            /* Integrate it */
            t1 = radint (work1, gvec, gnum, gmesh);
            ffil[idx] = t1 * TWO / PI;
            rfil += dr;

        }                       /* end for */

        break;

    case D_STATE:

        for (idx = istep * pct.gridpe; idx < istep * pct.gridpe + istep; idx++)
        {


            for (ift = 0; ift < gnum; ift++)
            {

                t1 = rfil * gvec[ift];
                if (t1 > SMALL)
                {
                    t2 = (THREE / (t1 * t1) - ONE) * sin (t1) / t1;
                    t2 -= THREE * cos (t1) / (t1 * t1);
                }
                else
                    t2 = 0.0;
                /*if(t1 < 1.0e-8) t2 = 0.0; */

                /* G-space cutoff */
                work1[ift] = work2[ift] * t2;

            }                   /* end for */


            /* Integrate it */
            t1 = radint (work1, gvec, gnum, gmesh);
            ffil[idx] = t1 * TWO / PI;

            rfil += dr;

        }                       /* end for */


        istep = npes * istep;
        global_sums (ffil, &istep, pct.grid_comm);

        t1 = (double) npes;
        rfil = t1 * rstep + 1.0e-10;

        for (idx = istep; idx < lrg_points; idx++)
        {

            for (ift = 0; ift < gnum; ift++)
            {

                t1 = rfil * gvec[ift];
                if (t1 > SMALL)
                {
                    t2 = (THREE / (t1 * t1) - ONE) * sin (t1) / t1;
                    t2 -= THREE * cos (t1) / (t1 * t1);
                }
                else
                    t2 = 0.0;
                /*if(t1 < 1.0e-8) t2 = 0.0; */

                /* G-space cutoff */
                work1[ift] = work2[ift] * t2;

            }                   /* end for */

            /* Integrate it */
            t1 = radint (work1, gvec, gnum, gmesh);
            ffil[idx] = t1 * TWO / PI;
            rfil += dr;

        }                       /* end for */

        break;

    case F_STATE:

        for (idx = istep * pct.gridpe; idx < istep * pct.gridpe + istep; idx++)
        {

            for (ift = 0; ift < gnum; ift++)
            {

                t1 = rfil * gvec[ift];
                if (t1 > SMALL)
                {
                    t2 = (15.0 / (t1 * t1) - 6.0) * sin (t1) / (t1 * t1);
                    t2 += (1.0 - 15.0 / (t1 * t1)) * cos (t1) / t1;
                }
                else
                    t2 = 0.0;
                /*if(t1 < 1.0e-8) t2 = 0.0; */

                /* G-space cutoff */
                work1[ift] = work2[ift] * t2;

            }                   /* end for */

            /* Integrate it */
            t1 = radint (work1, gvec, gnum, gmesh);
            ffil[idx] = t1 * TWO / PI;

            rfil += dr;

        }                       /* end for */

        istep = npes * istep;
        global_sums (ffil, &istep, pct.grid_comm);

        t1 = (double) npes;
        rfil = t1 * rstep + 1.0e-10;

        for (idx = istep; idx < lrg_points; idx++)
        {

            for (ift = 0; ift < gnum; ift++)
            {

                t1 = rfil * gvec[ift];
                if (t1 > SMALL)
                {
                    t2 = (15.0 / (t1 * t1) - 6.0) * sin (t1) / (t1 * t1);
                    t2 += (1.0 - 15.0 / (t1 * t1)) * cos (t1) / t1;
                }
                else
                    t2 = 0.0;
                /*if(t1 < 1.0e-8) t2 = 0.0; */

                /* G-space cutoff */
                work1[ift] = work2[ift] * t2;

            }                   /* end for */

            /* Integrate it */
            t1 = radint (work1, gvec, gnum, gmesh);
            ffil[idx] = t1 * TWO / PI;
            rfil += dr;

        }                       /* end for */

        break;

    case G_STATE:

        for (idx = istep * pct.gridpe; idx < istep * pct.gridpe + istep; idx++)
        {

            for (ift = 0; ift < gnum; ift++)
            {

                t1 = rfil * gvec[ift];
                if (t1 > SMALL)
                {
                    t2 = (105.0 / (t1 * t1 * t1 * t1) - 45.0 / (t1 * t1) + 1.0) * sin (t1) / t1;
                    t2 += (10.0 - 105.0 / (t1 * t1)) * cos (t1) / (t1 * t1);
                }
                else
                    t2 = 0.0;
                /*if(t1 < 1.0e-8) t2 = 0.0; */

                /* G-space cutoff */
                work1[ift] = work2[ift] * t2;

            }                   /* end for */

            /* Integrate it */
            t1 = radint (work1, gvec, gnum, gmesh);
            ffil[idx] = t1 * TWO / PI;

            rfil += dr;

        }                       /* end for */

        istep = npes * istep;
        global_sums (ffil, &istep, pct.grid_comm);

        t1 = (double) npes;
        rfil = t1 * rstep + 1.0e-10;

        for (idx = istep; idx < lrg_points; idx++)
        {

            for (ift = 0; ift < gnum; ift++)
            {

                t1 = rfil * gvec[ift];
                if (t1 > SMALL)
                {
                    t2 = (105.0 / (t1 * t1 * t1 * t1) - 45.0 / (t1 * t1) + 1.0) * sin (t1) / t1;
                    t2 += (10.0 - 105.0 / (t1 * t1)) * cos (t1) / (t1 * t1);
                }
                else
                    t2 = 0.0;
                /*if(t1 < 1.0e-8) t2 = 0.0; */

                /* G-space cutoff */
                work1[ift] = work2[ift] * t2;

            }                   /* end for */

            /* Integrate it */
            t1 = radint (work1, gvec, gnum, gmesh);
            ffil[idx] = t1 * TWO / PI;
            rfil += dr;

        }                       /* end for */

        break;

    default:

	error_handler ("angular momentum l=%d is not programmed", lval);


    }                           /* end switch */


    /* Release memory */
    my_free (gvec);
    my_free (gcof);
    my_free (work2);
    my_free (work1);

}                               /* end rft1 */
