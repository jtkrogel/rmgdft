/************************** SVN Revision Information **************************
 **    $Id$    **
******************************************************************************/
 

/*
 *
 */

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "main.h"
#include "init_var.h"
#include "LCR.h"
#include "pmo.h"


void get_cond_dos_test (STATE * states)
{
    int iprobe, jprobe;
    int iene;
    int st1;
    double eneR;
    double eneI;
    double *tot;
    double *tott;
    double *green_tem;
    double *g;
    double *sigma, *green_C;
    complex double *sigma_all;
    double *temp_matrix_tri, *temp_matrix, *matrix_product;
    double de, emin, emax;

    int nC;
    double alpha[2], beta[2];
    int i, j, *sigma_idx, idx_C, llda, locc;
    char fcd_n = 'N', fcd_c = 'C';
    FILE *file;

    double *ener1, *dos;
    int ntot, ndim;
    int ii, jj, kk, xoff;
    double *Green_store, *rho_energy;
    int root_pe, idx, ix;

    int E_POINTS, kpoint;
    double E_imag, KT;


    read_cond_input (&emin, &emax, &E_POINTS, &E_imag, &KT, &kpoint);
    de = (emax - emin) / (E_POINTS - 1);

    my_malloc_init( ener1, E_POINTS, double );
    my_malloc_init( dos, E_POINTS, double );


    alpha[0] = 1.0;
    alpha[1] = 0.0;
    beta[0] = 0.0;
    beta[1] = 0.0;

    /* store the imaginary part of Green function Gc on each processor
     * then calculate the energy dependent charge density 
     * and stored in rho_energy after average in the yz plane
     */

    nC = ct.num_states;

    ntot = 0;
    ndim = 0;
    for (i = 0; i < ct.num_blocks; i++)
    {
        ntot += ct.block_dim[i] * ct.block_dim[i];
        ndim += ct.block_dim[i];
    }

    for (i = 1; i < ct.num_blocks; i++)
    {
        ntot += ct.block_dim[i - 1] * ct.block_dim[i];
    }
    if (ndim != ct.num_states)
    {
        printf ("\n %d %d ndim  not equaol to nC in get_cond_frommatrix", ndim, ct.num_states);
        exit (0);
    }

/*========================== Reading Matrices =======================*/

    my_malloc_init( lcr[0].Htri, ntot, double );
    my_malloc_init( lcr[0].Stri, ntot, double );

    for (iprobe = 1; iprobe <= cei.num_probe; iprobe++)
    {
        idx = pmo.mxllda_lead[iprobe-1] * pmo.mxlocc_lead[iprobe-1];
        my_malloc_init( lcr[iprobe].H00, idx, double );
        my_malloc_init( lcr[iprobe].S00, idx, double );
        my_malloc_init( lcr[iprobe].H01, idx, double );
        my_malloc_init( lcr[iprobe].S01, idx, double );
    }


    for (iprobe = 1; iprobe <= cei.num_probe; iprobe++)
    {
        i = cei.probe_in_block[iprobe - 1];
        idx = pmo.mxllda_cond[i] * pmo.mxlocc_lead[iprobe-1];
        my_malloc_init( lcr[iprobe].HCL, idx, double );
        my_malloc_init( lcr[iprobe].SCL, idx, double );
    }


    read_matrix_pp();


/*======================== Allocate memory for sigma =================*/
                                                                                                
    idx = 0;
    for (iprobe = 1; iprobe <= cei.num_probe; iprobe++)
    {
        idx_C = cei.probe_in_block[iprobe - 1];  /* block index */
        idx = max(idx, pmo.mxllda_cond[idx_C] * pmo.mxlocc_cond[idx_C]);
    }
    my_malloc_init( sigma, 2 * idx, double );
                                                                                             
                                                                  
    my_malloc_init( sigma_idx, cei.num_probe, int );

    idx = 0;
    for (iprobe = 1; iprobe <= cei.num_probe; iprobe++)
    {
        sigma_idx[iprobe - 1] = idx;
        idx_C = cei.probe_in_block[iprobe - 1];  /* block index */
        idx += pmo.mxllda_cond[idx_C] * pmo.mxlocc_cond[idx_C];
    }
                                                                                               
    my_malloc_init( sigma_all, idx, complex double );


/*============== Allocate memory for tot, tott, g ====================*/

    idx = 0;
    for (iprobe = 1; iprobe <= cei.num_probe; iprobe++)
    {
        idx = max(idx, pmo.mxllda_lead[iprobe-1] * pmo.mxlocc_lead[iprobe-1]);
    }

    my_malloc_init( tot,  2 * idx, double );
    my_malloc_init( tott, 2 * idx, double );
    my_malloc_init( g,    2 * idx, double );

    my_malloc_init( green_tem, 2 * idx, double );
    my_malloc_init( green_C, 2 * ntot, double );
    st1 = (E_POINTS + pct.grid_npes - 1) / pct.grid_npes;
    my_malloc_init( Green_store, st1 * ntot, double );

/*===================================================================*/

    my_malloc_init( rho_energy, E_POINTS * get_FNX_GRID(), double );


    for (iene = 0; iene < E_POINTS; iene++)
    {
        ener1[iene] = emin + iene * de;
        dos[iene] = 0.0;
    }


    idx = 0;
    for (iene = pct.gridpe; iene < E_POINTS; iene += pct.grid_npes)
    {
        eneR = emin + iene * de;
        eneI = 0.0005;
        printf ("\n energy point %d %f\n", iene, eneR);


        /* sigma is a complex matrix with dimension ct.num_states * ct.num_states 
         * it sums over all probes
         * tot, tott, green_tem is also a complex matrix, 
         * their memory should be the maximum of probe dimensions, lcr[1,...].num_states
         */
        for (jprobe = 1; jprobe <= cei.num_probe; jprobe++)
        {
            Stransfer (tot, tott, lcr[jprobe].H00, lcr[jprobe].H01,
                       lcr[jprobe].S00, lcr[jprobe].S01, eneR, eneI, lcr[jprobe].num_states);
            Sgreen (tot, tott, lcr[jprobe].H00, lcr[jprobe].H01, lcr[jprobe].S00,
                    lcr[jprobe].S01, eneR, eneI, green_tem, g, lcr[jprobe].num_states, 0);

            Sgreen (tot, tott, lcr[jprobe].H00, lcr[jprobe].H01, lcr[jprobe].S00,
                    lcr[jprobe].S01, eneR, eneI, green_tem, g, lcr[jprobe].num_states, jprobe);


            Sigma (sigma, lcr[jprobe].HCL, lcr[jprobe].SCL, eneR, eneI, g, jprobe);


            idx_C = cei.probe_in_block[jprobe - 1];  /* block index */
            j = 0;
            for (i = 0; i < pmo.mxllda_cond[idx_C] * pmo.mxlocc_cond[idx_C]; i++)
            {
                    sigma_all[sigma_idx[jprobe - 1] + i].r = sigma[j];
                    j++; 
                    sigma_all[sigma_idx[jprobe - 1] + i].i = sigma[j];
                    j++; 
            }


        }                       /*  end for jprobe */




        Sgreen_c_wang (lcr[0].Htri, lcr[0].Stri, sigma_all, sigma_idx, 
                      eneR, eneI, (complex double *) green_C, nC);

        ener1[iene] = eneR;

        for (st1 = 0; st1 < ntot; st1++)
        {
            Green_store[idx + st1] = -green_C[st1 * 2 + 1];
        }
        idx += ntot;

    }                           /*  end for iene */



    MPI_Barrier (pct.grid_comm);

    pe2xyz (pct.gridpe, &ii, &jj, &kk);


    xoff = ii * get_FPX0_GRID();
    for (iene = 0; iene < E_POINTS; iene++)
    {
        root_pe = iene % pct.grid_npes;
        idx = iene / pct.grid_npes;
        if (pct.gridpe == 0)
        {
            printf ("root_pe, iene idx %d %d %d\n", root_pe, iene, idx);
            fflush (NULL);
        }

        for (st1 = 0; st1 < ntot; st1++)
        {
            lcr[0].density_matrix_tri[st1] = Green_store[idx * ntot + st1];
        }

        idx = ntot;
        MPI_Bcast (lcr[0].density_matrix_tri, idx, MPI_DOUBLE, root_pe, pct.grid_comm);

        get_new_rho_soft (states, rho);

        for (ix = 0; ix < get_FPX0_GRID(); ix++)
            for (i = 0; i < get_FPY0_GRID() * get_FPZ0_GRID(); i++)
                rho_energy[iene * get_FNX_GRID() + ix + xoff] += rho[ix * get_FPY0_GRID() * get_FPZ0_GRID() + i];


        MPI_Barrier(pct.img_comm);
    }

    iene = E_POINTS * get_FNX_GRID();
    global_sums (rho_energy, &iene);
    if (pct.gridpe == 0)
    {
        double dx = get_celldm(0] / get_NX_GRID();
        double x0 = 0.5 * get_celldm(0];

        file = fopen ("dos.dat", "w");
        fprintf (file, "#     x[a0]      E[eV]          dos\n\n");
        for (iene = 0; iene < E_POINTS; iene++)
        {
            eneR = emin + iene * de;

            for (ix = 0; ix < get_NX_GRID(); ix++)
            {

                fprintf (file, " %10.6f %10.6f %12.6e\n",
                         ix * dx - x0, eneR, rho_energy[iene * get_FNX_GRID() + ix * get_FG_NX()]);
            }
            fprintf (file, "\n");
        }

        fclose (file);
    }



    MPI_Barrier(pct.img_comm);
    fflush (NULL);

    my_free(ener1);
    my_free(dos);

    st1 = (E_POINTS + pct.grid_npes - 1) / pct.grid_npes;
    my_free(tot);
    my_free(tott);
    my_free(g);
    my_free(green_tem);
    my_free(sigma_all);
    my_free(sigma_idx);
    my_free(green_C);
    my_free(rho_energy);
    my_free(Green_store);


    my_free(lcr[0].Htri);
    my_free(lcr[0].Stri);
    for (iprobe = 1; iprobe <= cei.num_probe; iprobe++)
    {
        my_free(lcr[iprobe].H00);
        my_free(lcr[iprobe].S00);
        my_free(lcr[iprobe].H01);
        my_free(lcr[iprobe].S01);
    }

}
