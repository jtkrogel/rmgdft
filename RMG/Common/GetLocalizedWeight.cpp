/*
 *
 * Copyright 2014 The RMG Project Developers. See the COPYRIGHT file 
 * at the top-level directory of this distribution or in the current
 * directory.
 * 
 * This file is part of RMG. 
 * RMG is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * any later version.
 *
 * RMG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
*/



#include "const.h"

#include "rmgtypedefs.h"
#include "typedefs.h"
#include <complex>
#include "Kpoint.h"
#include "common_prototypes.h"
#include "common_prototypes1.h"
#include "transition.h"

// Used for localizing projectors in real space
template void GetLocalizedWeight<double> (Kpoint<double> **Kptr);
template void GetLocalizedWeight<std::complex<double> >(Kpoint<std::complex<double>> **Kptr);
template <typename KpointType>
void GetLocalizedWeight (Kpoint<KpointType> **Kptr)
{

    int max_size;
    KpointType *Bweight, *Nlweight;
    std::complex<double> I_t(0.0, 1.0);

    int num_nonloc_ions = Kptr[0]->BetaProjector->get_num_nonloc_ions();
    int *nonloc_ions_list = Kptr[0]->BetaProjector->get_nonloc_ions_list();


    SPECIES *sp;
    ION *iptr;
    fftw_plan p2;
    /*Pointer to the result of forward transform on the coarse grid */
    std::complex<double> *fptr;
    std::complex<double> *beptr, *gbptr;
    std::complex<double> *in, *out;

    int P0_BASIS = get_P0_BASIS();

    /*maximum of nldim^3 for any species */
    max_size = ct.max_nldim * ct.max_nldim * ct.max_nldim;


    /*Get memory to store the phase factor applied to the forward Fourier transform 
     * and to store the backwards transform*/
    beptr = (std::complex<double> *)fftw_malloc(sizeof(std::complex<double>) * 2 * max_size);
    std::complex<double> *phase_fftw = new std::complex<double>[max_size];

    if (beptr == NULL)
        rmg_error_handler (__FILE__, __LINE__, "can't allocate memory\n");

    gbptr = beptr + max_size;

    for(int kpt =0; kpt < ct.num_kpts_pe;kpt++)
    {

        Projector<KpointType> *P = Kptr[kpt]->BetaProjector;

        /* Loop over ions */
        for (int ion1 = 0; ion1 < num_nonloc_ions; ion1++)
        {

            int ion = nonloc_ions_list[ion1];
            /* Generate ion pointer */
            iptr = &ct.ions[ion];

            /* Get species type */
            sp = &ct.sp[iptr->species];

            Bweight = &Kptr[kpt]->nl_Bweight[ion1 * ct.max_nl * P0_BASIS];
            Nlweight = &Kptr[kpt]->nl_weight[ion1 * ct.max_nl * P0_BASIS];
//            for(int idx = 0;idx < sp->num_projectors*P0_BASIS; idx++) Bweight[idx] = 0.0;
//            for(int idx = 0;idx < sp->num_projectors*P0_BASIS; idx++) Nlweight[idx] = 0.0;


            int nlxdim = P->get_nldim(iptr->species);
            int nlydim = P->get_nldim(iptr->species);
            int nlzdim = P->get_nldim(iptr->species);

            in = (std::complex<double> *)fftw_malloc(sizeof(std::complex<double>) * nlxdim * nlydim * nlzdim);
            out = (std::complex<double> *)fftw_malloc(sizeof(std::complex<double>) * nlxdim * nlydim * nlzdim);


            /*Number of grid points on which fourier transform is done (in the coarse grid) */
            int coarse_size = nlxdim * nlydim * nlzdim;


            //fftw_import_wisdom_from_string (sp->backward_wisdom);
            p2 = fftw_plan_dft_3d (nlxdim, nlydim, nlzdim, reinterpret_cast<fftw_complex*>(in), reinterpret_cast<fftw_complex*>(out), FFTW_BACKWARD,
                    FFTW_ESTIMATE);
            //fftw_forget_wisdom ();


            /*Calculate the phase factor */
            FindPhase(nlxdim, nlydim, nlzdim, P->nlcrds[ion].data(), phase_fftw);

            /*Temporary pointer to the already calculated forward transform */
            fptr = (std::complex<double> *)&sp->forward_beta[kpt*sp->num_projectors * coarse_size];


            /* Loop over radial projectors */
            for (int ip = 0; ip < sp->num_projectors; ip++)
            {


                /*Apply the phase factor */
                for (int idx = 0; idx < coarse_size; idx++)
                {
                    gbptr[idx] =fptr[idx] * std::conj(phase_fftw[idx]);
                }


                /*Do the backwards transform */
                fftw_execute_dft (p2,  reinterpret_cast<fftw_complex*>(gbptr), reinterpret_cast<fftw_complex*>(beptr));
                /*This takes and stores the part of beta that is useful for this PE */
                AssignWeight (Kptr[kpt], sp, ion, reinterpret_cast<fftw_complex*>(beptr), Bweight, Nlweight);


                /*Advance the temp pointers */
                fptr += coarse_size;
                Bweight += P0_BASIS;
                Nlweight += P0_BASIS;

            }                   /*end for(ip = 0;ip < sp->num_projectors;ip++) */


            fftw_destroy_plan (p2);
            fftw_free(out);
            fftw_free(in);


        }                           /* end for */

    } // end for(kpt)

    fftw_free (beptr);
    delete [] phase_fftw;


}                               /* end GetWeight */