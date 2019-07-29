/************************** SVN Revision Information **************************
 **    $Id$    **
 ******************************************************************************/

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "main.h"
#include "init_var.h"

void update_pot(double *, double *, double *, double *, double *, double *,
        double *, double *, int *, STATE * states);
static double t[2];
extern int it_scf;
double tem1;

void update_TDDFT(double *mat_X)
{
    int outcount = 0;
    int state_plot, i;
    static int CONVERGENCE = FALSE;
    double tem, time1, time2;
    int idx, ione =1;
    int n = get_FP0_BASIS();

   

    void *RT = BeginRmgTimer("update");

    dcopy(&n, rho, &ione, rho_old, &ione);

    void *RT1 = BeginRmgTimer("Main: update: get_rho");
    get_new_rho(states, rho);
    EndRmgTimer(RT1);


    /* Update potential */
    update_pot(vxc, vh, vxc_old, vh_old, vnuc, rho, rhoc, rhocore, &CONVERGENCE, states);


    EndRmgTimer(RT);
//    get_te(rho, rhoc, rhocore, vh, vxc, states);



}

/*
   Function to update potentials vh and vxc:

   The new potentials are computed as a linear combination 
   of the old ones (input "vh" and "vxc") and the ones 
   corresponding to the input "rho".
   */
void update_pot(double *vxc, double *vh, double * vxc_old, double * vh_old,
        double *vnuc, double *rho, double *rhoc, double *rhocore,
        int *CONVERGENCE, STATE * states)
{
    int n = get_FP0_BASIS(), idx, ione = 1;
    double tem;

    /* save old vtot, vxc, vh */
    dcopy(&n, vxc, &ione, vxc_old, &ione);
    dcopy(&n, vh, &ione, vh_old, &ione);

    for (idx = 0; idx < get_FP0_BASIS(); idx++)
        vtot[idx] = vxc[idx] + vh[idx];

    /* Generate exchange-correlation potential */
    get_vxc(rho, rho, rhocore, vxc);


   // for(idx = 0; idx < n; idx++) printf("\n %d %f %f adad", idx,
//rho[idx], vxc[idx]); 

    pack_vhstod(vh, ct.vh_ext, get_FPX0_GRID(), get_FPY0_GRID(), get_PZ0_GRID(), ct.boundaryflag);

    /* Keep in memory vh*rho_new before updating vh */
    tem = ddot(&n, rho, &ione, vh, &ione);
    ct.Evhold_rho = 0.5 * get_vel_f() * real_sum_all(tem, pct.grid_comm);


    /* Generate hartree potential */
    //    get_vh1(rho, rhoc, vh, 15, ct.poi_parm.levels);
    void *RT1 = BeginRmgTimer("Main: update: get_vh");
    get_vh (rho, rhoc, vh, ct.hartree_min_sweeps, ct.hartree_max_sweeps,
            ct.poi_parm.levels, ct.rms/ct.hartree_rms_ratio, ct.boundaryflag);

    EndRmgTimer(RT1);


    /* Compute quantities function of rho only */
    tem = ddot(&n, rho, &ione, vh, &ione);
    ct.Evh_rho = 0.5 * get_vel_f() * real_sum_all(tem, pct.grid_comm);

    tem = ddot(&n, rhoc, &ione, vh, &ione);
    ct.Evh_rhoc = 0.5 * get_vel_f() * real_sum_all(tem, pct.grid_comm);



    /* evaluate correction vh+vxc */
    for (idx = 0; idx < get_FP0_BASIS(); idx++)
        vtot[idx] = vxc[idx] + vh[idx] - vtot[idx];

    /* evaluate SC correction */
    t[0] = t[1] = 0.;

    for (idx = 0; idx < get_FP0_BASIS(); idx++)
    {
        t[0] += rho[idx] * vtot[idx];
        t[1] += vtot[idx] * vtot[idx];
    }
    idx = 2;
    global_sums(t, &idx, pct.grid_comm);
    t[0] *= get_vel_f();
    t[0] /= (double) ct.num_ions;
    t[1] = sqrt(t[1] / ((double) (ct.vh_nbasis)));

    ct.rms = t[1];
    if (pct.gridpe == 0)
        printf(" SCF CHECKS: RMS[dv] = %15.10e RMS[drho] = %15.10e \n", t[1], tem1);

    fflush(NULL);
    MPI_Barrier(pct.img_comm);

    if (ct.scf_steps < 4 && ct.runflag == 0)
    {
        for (idx = 0; idx < get_FP0_BASIS(); idx++)
        {
            vxc[idx] = vxc_old[idx];
            vh[idx] = vh_old[idx];
        }
    }

    for (idx = 0; idx < get_FP0_BASIS(); idx++)
        vtot[idx] = vxc[idx] + vh[idx] + vnuc[idx];


    get_vtot_psi(vtot_c, vtot, get_FG_RATIO());

    if (t[1] < ct.thr_rms)
        *CONVERGENCE = TRUE;
}
