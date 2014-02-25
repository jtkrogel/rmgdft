/************************** SVN Revision Information **************************
 **    $Id$    **
******************************************************************************/

/****f* QMD-MGDFT/get_vh.c *****
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
 *   void get_vh(rmg_double_t *rho, rmg_double_t *rhoc, rmg_double_t *vh_eig, int sweeps, int maxlevel)
 *   Iterative procedure to obtain the hartree potential.
 *   Uses Mehrstallen finite differencing with multigrid accelerations.
 *   The multigrid scheme uses a standard W-cycle with Jacobi relaxation.
 *   (Underrelaxed on the coarser grids.)
 * INPUTS
 *   rho: total charge density
 *   rhoc: compensating charge density
 *   sweeps:  number of iterations 
 *   maxlevel:  maximum multigrid level
 * OUTPUT
 *   vh_eig: Hartree potential
 * PARENTS
 *   init.c scf.c
 * CHILDREN
 *   getpoi_bc.c pack_vhstod.c pack_ptos.c app_cir.c 
 *   set_bc.c app_cil.c mgrid_solv.c pack_vhdtos.c
 * SOURCE
 */



#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "init_var.h"
#include "LCR.h"
#include "twoParts.h"


/* Pre and post smoothings on each level */
static int poi_pre[5] = { 0, 3, 3, 3, 3 };
static int poi_post[5] = { 0, 3, 3, 3, 3 };


void get_vh_negf (rmg_double_t * rho, rmg_double_t * rhoc, rmg_double_t * vh_eig, int min_sweeps, int max_sweeps, int maxlevel, rmg_double_t rms_target)
{

    int idx, its, nits, sbasis, pbasis;
    rmg_double_t t1, vavgcor, diag;
    rmg_double_t *mgrhsarr, *mglhsarr, *mgresarr, *work;
    rmg_double_t *sg_rho, *sg_vh, *sg_res, *nrho,  diff, residual = 100.0;
    int incx = 1, cycles;
    double k_vh;

    rmg_double_t time1, time2, time3, time4, time5, time6;
    time1 = my_crtc ();

    time3 = my_crtc();
    /*Taken from ON code, seems to help a lot with convergence*/
    poi_pre[maxlevel] = ct.poi_parm.coarsest_steps;

    k_vh = 0.0;

    nits = ct.poi_parm.gl_pre + ct.poi_parm.gl_pst;
    sbasis = (ct.vh_pxgrid + 2) * (ct.vh_pygrid + 2) * (ct.vh_pzgrid + 2);
    pbasis = ct.vh_pxgrid * ct.vh_pygrid * ct.vh_pzgrid;


    /* Grab some memory for our multigrid structures */
    my_malloc (mgrhsarr, 12 * sbasis, rmg_double_t);
    mglhsarr = mgrhsarr + sbasis;
    mgresarr = mglhsarr + sbasis;
    work = mgresarr + sbasis;
    sg_rho = work + 4 * sbasis;
    sg_vh = sg_rho + sbasis;
    sg_res = sg_vh + sbasis;
    nrho = sg_res + sbasis;

    /* Subtract off compensating charges from rho */
    for (idx = 0; idx < pbasis; idx++)
        work[idx] = rho[idx] - rhoc[idx];

    for (idx = 0; idx < sbasis; idx++)
        nrho[idx] = 0.0;
    pack_vhstod (work, nrho, get_FPX0_GRID(), get_FPY0_GRID(), get_FPZ0_GRID());

    /* Transfer rho into smoothing grid */
    pack_ptos (sg_rho, nrho, ct.vh_pxgrid, ct.vh_pygrid, ct.vh_pzgrid);


    /* Apply CI right hand side to rho and store result in work array */
    app_cir (sg_rho, mgrhsarr, ct.vh_pxgrid, ct.vh_pygrid, ct.vh_pzgrid);


    /* Multiply through by 4PI */
    t1 = -FOUR * PI;
    QMD_dscal (pbasis, t1, mgrhsarr, incx);
    time4 = my_crtc();
    rmg_timings (VH1_TIME, (time4 - time3));


    its = 0;

    while ( ((its < max_sweeps) && (residual > rms_target))  || (its < min_sweeps))
    {


        /* Mehrstallen smoothings */
        for (cycles = 0; cycles < nits; cycles++)
        {

    time3 = my_crtc();

	    /*At the end of this force loop, laplacian operator is reapplied to evalute the residual. Therefore,
	     * when there is no need to apply it, when this loop is called second, third, etc time. */
            if ( (cycles) || (!its))
            {
                /* Transfer vh into smoothing grid */
                time5 = my_crtc();
                pack_ptos (sg_vh, ct.vh_ext, ct.vh_pxgrid, ct.vh_pygrid, ct.vh_pzgrid);
                time6 = my_crtc();
                rmg_timings (VH2a_TIME, (time6 - time5));

                /* Apply operator */
                diag = app_cil (sg_vh, mglhsarr, ct.vh_pxgrid, ct.vh_pygrid, ct.vh_pzgrid,
                            ct.hxxgrid, ct.hyygrid, ct.hzzgrid);
                diag = -1.0 / diag;

                time5 = my_crtc();
                rmg_timings (VH2b_TIME, (time6 - time5));
                /* Generate residual vector */
#pragma omp parallel for schedule(static, 1024)
                for (idx = 0; idx < pbasis; idx++)
                {

                    mgresarr[idx] = mgrhsarr[idx] - mglhsarr[idx];

                }                   /* end for */
                time6 = my_crtc();
                rmg_timings (VH2c_TIME, (time6 - time5));

            }

         /*  Fix Hartree in some region  */
                time5 = my_crtc();
            confine (mgresarr, ct.vh_pxgrid, ct.vh_pygrid, ct.vh_pzgrid, potentialCompass, 0);
                time6 = my_crtc();
                rmg_timings (VH2d_TIME, (time6 - time5));

        time4 = my_crtc();
        rmg_timings (VH2_TIME, (time4 - time3));
            /* Pre and Post smoothings and multigrid */
            if (cycles == ct.poi_parm.gl_pre)
            {

                /* Transfer res into smoothing grid */
                time3 = my_crtc();
                pack_ptos (sg_res, mgresarr, ct.vh_pxgrid, ct.vh_pygrid, ct.vh_pzgrid);

                 
                mgrid_solv_negf (mglhsarr, sg_res, work,
                            ct.vh_pxgrid, ct.vh_pygrid, ct.vh_pzgrid, ct.hxxgrid,
                            ct.hyygrid, ct.hzzgrid,
                            0, pct.neighbors, ct.poi_parm.levels, poi_pre,
                            poi_post, ct.poi_parm.mucycles, ct.poi_parm.sb_step, k_vh,
                            get_FG_NX()*get_NX_GRID(), get_FG_NY()*get_NY_GRID(), get_FG_NZ()*get_NZ_GRID(),
                            get_FPX_OFFSET(), get_FPY_OFFSET(), get_FPZ_OFFSET(),
                            get_FPX0_GRID(), get_FPY0_GRID(), get_FPZ0_GRID());

                time4 = my_crtc ();
                rmg_timings (MGRID_VH_TIME, (time4 - time3));


                /* Transfer solution back to mgresarr array */
                pack_stop (mglhsarr, mgresarr, ct.vh_pxgrid, ct.vh_pygrid, ct.vh_pzgrid);

                /*  Fix Hartree in some region  */

                confine (mgresarr, ct.vh_pxgrid, ct.vh_pygrid, ct.vh_pzgrid, potentialCompass, 0);

                /* Update vh */
                t1 = ONE;
                saxpy (&pbasis, &t1, mgresarr, &incx, ct.vh_ext, &incx);

            }
            else
            {

                /* Update vh */
                t1 = -ct.poi_parm.gl_step * diag;
                saxpy (&pbasis, &t1, mgresarr, &incx, ct.vh_ext, &incx);

            }                   /* end if */



        }                       /* end for */

        time4 = my_crtc();
        /*Get residual*/
        /* Transfer vh into smoothing grid */
        pack_ptos (sg_vh, ct.vh_ext, ct.vh_pxgrid, ct.vh_pygrid, ct.vh_pzgrid);

        /* Apply operator */
        diag = app_cil (sg_vh, mglhsarr, ct.vh_pxgrid, ct.vh_pygrid, ct.vh_pzgrid,
                ct.hxxgrid, ct.hyygrid, ct.hzzgrid);
        diag = -1.0 / diag;

        /* Generate residual vector */
        for (idx = 0; idx < pbasis; idx++)
        {

            mgresarr[idx] = mgrhsarr[idx] - mglhsarr[idx];

        }                   /* end for */

        confine (mgresarr, ct.vh_pxgrid, ct.vh_pygrid, ct.vh_pzgrid, potentialCompass, 0);

        residual = 0.0;
        for (idx = 0; idx < pbasis; idx++)
           residual += mgresarr[idx] * mgresarr[idx];


        residual = sqrt (real_sum_all(residual, pct.grid_comm) / ct.psi_fnbasis);

        time3 = my_crtc();
        rmg_timings (VH3_TIME, (time3 - time4));

        //printf("\n get_vh sweep %3d, rms residual is %10.5e", its, residual);


        its ++;
    }                           /* end for */

    printf ("\n");
    progress_tag ();
    printf ("Executed %3d sweeps, residual is %15.8e, rms is %15.8e\n", its, residual, ct.rms);


    /* Pack the portion of the hartree potential used by the wavefunctions
     * back into the wavefunction hartree array. */
    pack_vhdtos (vh_eig, ct.vh_ext, get_FPX0_GRID(), get_FPY0_GRID(), get_FPZ0_GRID());

    /* Release our memory */
    my_free (mgrhsarr);

    time2 = my_crtc ();
    rmg_timings (HARTREE_TIME, (time2 - time1));

}                               /* end get_vh */

