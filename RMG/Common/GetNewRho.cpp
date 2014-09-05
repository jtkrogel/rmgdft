/*
 *
 * Copyright (c) 2014, Emil Briggs
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
*/

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "transition.h"
#include "const.h"
#include "RmgTimer.h"
#include "rmgtypedefs.h"
#include "params.h"
#include "typedefs.h"
#include "common_prototypes.h"
#include "common_prototypes1.h"
#include "rmg_error.h"
#include "Kpoint.h"
#include <complex>
#include "../Headers/prototypes.h"


template void GetNewRho<double>(Kpoint<double> **, double *);
template void GetNewRho<std::complex<double> >(Kpoint<std::complex<double>> **, double *);

template <typename OrbitalType> void GetNewRho(Kpoint<OrbitalType> **Kpts, double *rho)
{

    int pbasis = Kpts[0]->pbasis;
    int nstates = Kpts[0]->nstates;
    int max_product = (ct.max_nl + 1) * ct.max_nl / 2;
    double *qtpr;

    double *work = new double[pbasis];
    double *product = new double[max_product];
    double *sintR = new double[2 * ct.max_nl];
    double *sintI = new double[2 * ct.max_nl];
    OrbitalType *sint = new OrbitalType[2 * ct.max_nl];


    for(int idx = 0;idx < pbasis;idx++)
        work[idx] = 0.0;

    for (int kpt = 0; kpt < ct.num_kpts; kpt++)
    {

        /* Loop over states and accumulate charge */
        for (int istate = 0; istate < nstates; istate++)
        {

            double scale = Kpts[kpt]->kstates[istate].occupation[0] * ct.kp[kpt].kweight;

            OrbitalType *psi = Kpts[kpt]->Kstates[istate].psi;

            for (int idx = 0; idx < pbasis; idx++)
            {
                work[idx] += scale * std::norm(psi[idx]);
            }                   /* end for */

        }                       /*end for istate */

    }                           /*end for kpt */

    /* Interpolate onto fine grid, result will be stored in rho*/

    switch (ct.interp_flag)
    {
        case 0:
            pack_rho_ctof (work, rho);
            break;
        case 1:
            bspline_interp_full (work, rho);
            break;
        case 2:
            mg_prolong_MAX10 (rho, work, get_FPX0_GRID(), get_FPY0_GRID(), get_FPZ0_GRID(), get_PX0_GRID(), get_PY0_GRID(), get_PZ0_GRID(), get_FG_RATIO(), 6);
            break;

        default:

            //Dprintf ("charge interpolation is set to %d", ct.interp_flag);
            rmg_error_handler (__FILE__, __LINE__, "ct.interp_flag is set to an invalid value.");


    }


    for (int ion = 0; ion < pct.num_nonloc_ions; ion++)
    {
        int gion = pct.nonloc_ions_list[ion];
        
        if (pct.Qidxptrlen[gion])
        {
            
            ION *iptr = &ct.ions[gion];
       
            int nh = ct.sp[iptr->species].nh;
            
            int *ivec = pct.Qindex[gion];
            int ncount = pct.Qidxptrlen[gion];
            double *qnmI = pct.augfunc[gion];

            for (int i=0; i < max_product; i++)
                product[i] = 0.0;

            for (int kpt = 0; kpt < ct.num_kpts; kpt++)
            {

                STATE *sp = ct.kp[kpt].kstate;
                /* Loop over states and accumulate charge */
                for (int istate = 0; istate < ct.num_states; istate++)
                {
                    double t1 = sp->occupation[0] * ct.kp[kpt].kweight;

                    for (int i = 0; i < ct.max_nl; i++)
                    {
                        sint[i] = Kpts[kpt]->newsint_local[ion * ct.num_states * ct.max_nl + istate * ct.max_nl + i];
//                        sintR[i] =
//                            pct.newsintR_local[kpt * pct.num_nonloc_ions * ct.num_states * ct.max_nl 
//                            + ion * ct.num_states * ct.max_nl + istate * ct.max_nl + i];

//                        if(!ct.is_gamma) {
//                            sintI[i] =
//                                pct.newsintI_local[kpt * pct.num_nonloc_ions * ct.num_states * ct.max_nl 
//                                + ion * ct.num_states * ct.max_nl + istate * ct.max_nl + i];
//                        }

                    }               /*end for i */

                    int idx = 0;
                    for (int i = 0; i < nh; i++)
                    {
                        for (int j = i; j < nh; j++)
                        {

                            if(i == j) {

                                    product[idx] += t1 * (std::real(sint[i]) * std::real(sint[j]) + std::imag(sint[i]) * std::imag(sint[j]));

                            }
                            else {

                                    product[idx] += 2.0 * t1 * (std::real(sint[i]) * std::real(sint[j]) + std::imag(sint[i]) * std::imag(sint[j]));

                            }
#if 0
                            if(ct.is_gamma) {
                                if (i == j)
                                    product[idx] += t1 * sintR[i] * sintR[j];
                                else
                                    product[idx] += 2 * t1 * sintR[i] * sintR[j];
                            }
                            else {

                                if (i == j)
                                    product[idx] += t1 * (sintR[i] * sintR[j] + sintI[i] * sintI[j]);
                                else
                                    product[idx] += 2 * t1 * (sintR[i] * sintR[j] + sintI[i] * sintI[j]);
                            }
#endif
                            idx++;
                        }           /*end for j */
                    }               /*end for i */
                    sp++;
                }                   /*end for istate */
            }                       /*end for kpt */


            int idx = 0;
            for (int i = 0; i < nh; i++)
            {
                for (int j = i; j < nh; j++)
                {
                    qtpr = qnmI + idx * ncount;
                    for (int icount = 0; icount < ncount; icount++)
                    {
                        rho[ivec[icount]] += qtpr[icount] * product[idx];
                    }           /*end for icount */
                    idx++;
                }               /*end for j */
            }                   /*end for i */


        }                       /*end if */

    }                           /*end for ion */

    if(!ct.is_gamma) {
        //symmetrize_rho (rho);
    }


    /* Check total charge. */
    ct.tcharge = ZERO;
    int FP0_BASIS = Rmg_G->get_P0_BASIS(Rmg_G->default_FG_RATIO);
    for (int idx = 0; idx < FP0_BASIS; idx++)
        ct.tcharge += rho[idx];

    /* ct.tcharge = real_sum_all (ct.tcharge); */
    ct.tcharge = real_sum_all (ct.tcharge, pct.img_comm);
    ct.tcharge = ct.tcharge * get_vel_f();

    /* Renormalize charge, there could be some discrpancy because of interpolation */
    double t1 = ct.nel / ct.tcharge;
    int incx = 1;
    if (pct.imgpe == 0)
        rmg_printf ("\n get_new_rho: Normalization constant for new charge is %f", t1);
    QMD_dscal (FP0_BASIS, t1, rho, incx);

    /*Update ct.tcharge, do not really recalculate it, just mutltiply it by normalization constant */
    ct.tcharge *= t1;



    delete [] sint;
    delete [] sintI;
    delete [] sintR;
    delete [] product;
    delete [] work;
}
