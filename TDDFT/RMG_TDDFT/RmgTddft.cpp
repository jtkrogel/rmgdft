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


#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "transition.h"
#include "const.h"
#include "State.h"
#include "Kpoint.h"
#include "TradeImages.h"
#include "RmgTimer.h"
#include "RmgThread.h"
#include "GlobalSums.h"
#include "rmgthreads.h"
#include "vhartree.h"
#include "packfuncs.h"
#include "typedefs.h"
#include "common_prototypes.h"
#include "common_prototypes1.h"
#include "rmg_error.h"
#include "Kpoint.h"
#include "Subdiag.h"
#include "Functional.h"
#include "Solvers.h"
#include "RmgParallelFft.h"

#include "blas.h"
#include "prototypes_tddft.h"
#include "RmgException.h"


void  init_point_charge_pot(double *vtot_psi, int density);

void print_matrix_d(double *matrix,  int  *nblock, int *ldim ){
    /*    
          Print the block [0:nblock-1,0:nblock-1]  of an  array of size
          ldim*ldim.  Array (matrix) is stored as a vector.

     */ 

    int  N  = *nblock ;   // block size to be printed 
    int  LD = *ldim   ;   // leaading dimension of matrix

    int i,j, ij   ;

    if (pct.gridpe ==0 ) {
        for     ( i = 0; i< N  ; i++){   // row
            //printf("\n   %d  : ", i);
            printf("\n"); 
            for  ( j = 0; j< N  ; j++){   // column
                ij = i*LD  + j  ;
                printf(" %14.6e", matrix[ij]) ;
            }
        }
        printf("\n");
    }
}

///////////////////////////////////////

void print_matrix_z(double *matrix,  int  *nblock, int *ldim ){
    /*    
          Print the block [0:nblock-1,0:nblock-1]  of a complex  array of size
          ldim*ldim.  Array (matrix) is stored as a ldim*ldim vector or real  
          values followed by ldim*ldim vector of imaginary values

     */ 

    int  N  = *nblock ;   // block size to be printed 
    int  LD = *ldim   ;   // leaading dimension of matrix

    int i,j, ij   ;
    int i0_re = 0 ; 
    int i0_im = i0_re + LD*LD  ; 

    if (pct.gridpe ==0 ) {

        // print  real part  
        printf("***  real:\n");
        for     ( i = 0; i< N  ; i++){         // row
            printf("\n");                      // printf("   %d  : ", i);
            for  ( j = 0; j< N  ; j++){        // column
                ij = i*LD  + j  ;
                printf(" %14.6e", matrix[i0_re+ ij]) ;
            }
        }
        printf("\n") ;   

        // print  imaginary  part  
        printf("***  imag:\n ") ;   
        for     ( i = 0; i< N  ; i++){         // row
            printf("\n");                       // printf("   %d  : ", i);
            for  ( j = 0; j< N  ; j++){         // column
                ij = i*LD  + j  ;
                printf(" %14.6e", matrix[ i0_im+ij ]) ;
            }
        }
        printf("\n");
    }
}


///////////////////////////////////////
void  magnus( double *H0, double *H1, double *p_time_step , double *Hdt, int *ldim){
    /*  
        This is a first order Magnus  expansion, wchih is equivalent to   mid-point propagator
        F*dt =   Integrate_0^t   H(t) dt   
        ~= 1/2*(H0 +H1) *dt

        It works for real  Hamiltonian matrices only.  Complex are required for bybrid functions with exact exchange

     */  

    double  dt     =    *p_time_step  ;   // time step
    int     Nbasis =    *ldim         ;     // leaading dimension of matrix

    int     Nsq =    Nbasis*Nbasis    ; 
    //double  dminus_one  = -1.0e0      ; 
    double  one         =  1.0e0      ; 
    double  half_dt     =  0.5e0*dt   ;
    int    ione = 1 ;
    //printf("magnus,  dt, ldim =  %f   %d  \n", dt,Nbasis ) ;

    dcopy_ ( &Nsq ,   H0      ,     &ione , Hdt ,  &ione) ;
    daxpy_ ( &Nsq , &one      , H1, &ione , Hdt ,  &ione) ;   
    dscal_ ( &Nsq , &half_dt  ,             Hdt ,  &ione) ;


}
///////////////////////////////////////
void tst_conv_matrix  (double * p_err , int * p_ij_err ,   double *H0, double *H1,  int *ldim) {  
    /* 
       Test convergemce: calculate  infty error for convergence:   || H1 - H0||_infty
       Here error is  L_infty norm of a difference matrix dH =H1-H0:
     *p_err    :  [out]  returns absolute value of the largest matrix element of  dH  where dH = H1-H0 
     *p_ij_err :  [out]  returns position  of err in the  matrix dH
     H0, H1    :  [in]   tested matrices
ldim      :  [in]   leading dimension of  H0, H1  (= Nbasis)
     */


    int    Nbasis     =   *ldim         ;  
    int    Nsq        =   Nbasis*Nbasis ;  

    double err        =  fabs( H1[0] - H0[0]) ; 
    int    ij_err     =  0                    ;  // position/ location  in matrix

    for (int i =1 ;  i < Nsq ; i++ )  {
        double  tst = fabs(H1[i]-H0[i])  ;
        if (  tst  > err)   { err = tst ; ij_err = i ; } 
    } 

    //  pass back the max  error and its location:
    * p_err    =  err   ;
    * p_ij_err = ij_err ; 

} 
///////////////////////////////////////
void extrapolate_Hmatrix   (double  *Hm1, double *H0, double *H1,  int *ldim) {  
    /* 
       Linear extrapolation of  H(1) from H(0) and  H(-1):
       H(1) =   H(0) + dH   =  H(0) + ( H(0)-H(-1) )  =  2*H(0) - H(-1) 
       H1   =  2*H0 - Hm1

       Hm1,H0  : [in]
H1      : [out]
     */

    int    Nbasis     =   *ldim         ;  
    int    Nsq        =   Nbasis*Nbasis ;  

    int    ione       = 1 ;
    double  neg_one   = -1.0e0   ;
    double  two       =  2.0e0   ;

    dcopy_ ( &Nsq ,            Hm1 , &ione , H1 ,  &ione) ;    //  
    dscal_ ( &Nsq , &neg_one , H1  , &ione )              ;    //  H1 = -H(-1)  
    daxpy_ ( &Nsq , &two     , H0  , &ione , H1  , &ione) ;    //  H1 =  2*H0 + H1 =  2*H0 -Hm1 

}
///////////////////////////////////////

/////////////////////
template void RmgTddft<double> (double *, double *, double *,
        double *, double *, double *, double *, Kpoint<double> **);
template void RmgTddft<std::complex<double> > (double *, double *, double *,
        double *, double *, double *, double *, Kpoint<std::complex<double>> **);
template <typename OrbitalType> void RmgTddft (double * vxc, double * vh, double * vnuc, 
        double * rho, double * rho_oppo, double * rhocore, double * rhoc, Kpoint<OrbitalType> **Kptr)
{

    double *vtot, *vtot_psi, *vxc_psi=NULL;

    int dimx = Rmg_G->get_PX0_GRID(Rmg_G->get_default_FG_RATIO());
    int dimy = Rmg_G->get_PY0_GRID(Rmg_G->get_default_FG_RATIO());
    int dimz = Rmg_G->get_PZ0_GRID(Rmg_G->get_default_FG_RATIO());
    int FP0_BASIS = dimx * dimy * dimz;

    FILE *dfi = NULL;
    std::string filename;
    int n2,n22, numst, P0_BASIS,i, ione =1;
    int tot_steps = 0, pre_steps, tddft_steps;
    int Ieldyn = 1, iprint = 0;


    P0_BASIS =  Rmg_G->get_P0_BASIS(1);
    FP0_BASIS = Rmg_G->get_P0_BASIS(Rmg_G->default_FG_RATIO);

    numst = ct.num_states; 
    n2 = numst * numst;
    n22 = 2* n2;

    double *Hmatrix     = new double[n2];
    double *Hmatrix_old = new double[n2];
    double *Smatrix     = new double[n2];
    double *Akick       = new double[n2];
    double *Pn0         = new double[2*n2];
    double *Pn1         = new double[2*n2];
    double *vh_old      = new double[FP0_BASIS];
    double *vxc_old     = new double[FP0_BASIS];
    double *vh_corr_old = new double[FP0_BASIS];
    double *vh_corr     = new double[FP0_BASIS];
    // Jacek: 
    //double *dHmatrix    = new double[n2];   // storage for  H1 -H1_old 
    double *Hmatrix_m1  = new double[n2];
    double *Hmatrix_0   = new double[n2];
    double *Hmatrix_1   = new double[n2];
    double *Hmatrix_dt  = new double[n2];   
    double    err        ;
    int       ij_err     ;
    double   thrs_dHmat =1.0e-5 ;

    if(pct.gridpe == 0) {
        printf("\n Number of states used for TDDFT: Nbasis =  %d \n",numst);
    }

    //    double *vh_x = new double[FP0_BASIS];
    //    double *vh_y = new double[FP0_BASIS];
    //    double *vh_z = new double[FP0_BASIS];
    double *xpsi = new double[P0_BASIS * numst];
    double dipole_ele[3];


    if(0)
    {
        XyzMatrix(Kptr[0], (OrbitalType *)Hmatrix_0, 1, 0, 0);
        if(pct.gridpe == 0)
            for(int i = 0; i < 5; i++) 
            { printf("xyz\n");
                for(int j = 0; j < 5; j++) printf(" %10.4e", 0.001* Hmatrix_0[i*numst + j]);
            }
    }

    RmgTimer *RT0 = new RmgTimer("2-TDDFT");

    // Loop over k-points
    if(ct.num_kpts != 1) 
    {
        rmg_printf(" \n  TDDFT does not support multiple k-points \n");

        fflush(NULL);
        throw RmgFatalException() << " TDDFT does not support multiple k-points in "<< __FILE__ << " at line " << __LINE__ << "\n";
        exit(0);
    }
    if(!ct.norm_conserving_pp)
    {
        rmg_printf(" \n  TDDFT support NCPP only \n");

        fflush(NULL);
        throw RmgFatalException() << " TDDFT support NCPP only in "<< __FILE__ << " at line " << __LINE__ << "\n";
        exit(0);
    }

    double efield[3];
    efield[0] = ct.x_field_0 * ct.e_field;
    efield[1] = ct.y_field_0 * ct.e_field;
    efield[2] = ct.z_field_0 * ct.e_field;
    if(pct.gridpe == 0)
    {
        filename = std::string(pct.image_path[pct.thisimg]) +"dipole.dat_"+ std::string(ct.basename);

        dfi = fopen(filename.c_str(), "w");

        fprintf(dfi, "\n  &&electric field:  %f  %f  %f ",efield[0], efield[1], efield[2]);

    }



    //    VhcorrDipoleInit(vh_x, vh_y, vh_z, rhoc);

    /* allocate memory for eigenvalue send array and receive array */

    vtot = new double[FP0_BASIS];
    vtot_psi = new double[P0_BASIS];
    double time_step = ct.tddft_time_step;

    if(ct.restart_tddft)
    {

        ReadData_rmgtddft(ct.outfile_tddft, vh, vxc, vh_corr, Pn0, Hmatrix, Smatrix,Smatrix, &pre_steps);
        dcopy(&n2, Hmatrix, &ione, Hmatrix_old, &ione);
        ReadData (ct.infile, vh, rho, vxc, Kptr);

    }
    else
    {
        for (int idx = 0; idx < FP0_BASIS; idx++) vtot[idx] = 0.0;
        if(ct.tddft_mode == EFIELD)
        {
            init_efield(vtot);
            GetVtotPsi (vtot_psi, vtot, Rmg_G->default_FG_RATIO);
        }
        else if(ct.tddft_mode == POINT_CHARGE)
        {
            init_point_charge_pot(vtot_psi, 1);
        }
        else
        {
            throw RmgFatalException() << " TDDFT mode not defined: "<< ct.tddft_mode<< __FILE__ << " at line " << __LINE__ << "\n";
        }

        HmatrixUpdate(Kptr[0], vtot_psi, (OrbitalType *)Akick);

        /* save old vhxc + vnuc */
        for (int idx = 0; idx < FP0_BASIS; idx++) {
            vtot[idx] = vxc[idx] + vh[idx] + vnuc[idx];
        }

        // Transfer vtot from the fine grid to the wavefunction grid
        GetVtotPsi (vtot_psi, vtot, Rmg_G->default_FG_RATIO);

        /*Generate the Dnm_I */
        get_ddd (vtot, vxc, true);

        HSmatrix (Kptr[0], vtot_psi, vxc_psi, (OrbitalType *)Hmatrix, (OrbitalType *)Smatrix);

        dcopy(&n2, Hmatrix, &ione, Hmatrix_old, &ione);

        if(pct.gridpe == 0 && ct.verbose)
        { 
            printf("\nHMa\n");
            for(i = 0; i < 10; i++) 
            {

                printf("\n");
                for(int j = 0; j < 10; j++) printf(" %8.1e",  Hmatrix[i*numst + j]);
            }

            printf("\nSMa\n");
            for(i = 0; i < 10; i++) 
            {

                printf("\n");
                for(int j = 0; j < 10; j++) printf(" %8.1e",  Smatrix[i*numst + j]);
            }
        }


        pre_steps = 0;

        for(i = 0; i < n2; i++) Hmatrix[i] += Akick[i]/time_step;

        for(i = 0; i < 2* n2; i++) Pn0[i] = 0.0;

        for(i = 0; i < ct.nel/2; i++) Pn0[i * numst + i] = 2.0;


        get_dipole(rho, dipole_ele);

        rmg_printf("\n  x dipolll  %f ", dipole_ele[0]);
        rmg_printf("\n  y dipolll  %f ", dipole_ele[1]);
        rmg_printf("\n  z dipolll  %f ", dipole_ele[2]);

        //   if(pct.gridpe == 0)
        //   for(int i = 0; i < 5; i++) 
        //   { printf("Akick\n");
        //       for(int j = 0; j < 5; j++) printf(" %10.4e", Akick[i*numst + j]);
        //   }

    }

    //  initialize   data for rt-td-dft
    //int nblock = 10 ;   //  size of tthe block for printing (debug!)
    dcopy(&n2, Hmatrix, &ione, Hmatrix_m1, &ione);
    dcopy(&n2, Hmatrix, &ione, Hmatrix_0 , &ione);

    /*
       if(pct.gridpe == 0) { printf("**** Smat  : \n");  print_matrix_d(Smatrix,   &nblock, &numst)   ; }
       if(pct.gridpe == 0) { printf("**** Hmat  : \n");  print_matrix_d(Hmatrix,   &nblock, &numst)   ; }
       if(pct.gridpe == 0) { printf("**** Hmat0 : \n");  print_matrix_d(Hmatrix_0, &nblock, &numst)   ; }
       if(pct.gridpe == 0) { printf("**** Hmat1 : \n");  print_matrix_d(Hmatrix_1, &nblock, &numst)   ; }
     */

    //  run rt-td-dft
    for(tddft_steps = 0; tddft_steps < ct.tddft_steps; tddft_steps++)
    {
        //if(pct.gridpe == 0) printf("=========================================================================\n   step:  %d\n", tddft_steps);

        tot_steps = pre_steps + tddft_steps;

        /* 
        //Wenchang: 
        dscal(&n2, &time_step, Hmatrix, &ione);   
        eldyn_(&numst, Smatrix, Hmatrix, Pn0, Pn1, &Ieldyn, &iprint);
         */
        //  guess H1 from  H(0) and H(-1):
        extrapolate_Hmatrix  (Hmatrix_m1,  Hmatrix_0, Hmatrix_1  , &numst) ; //   (*Hm1, double *H0, double *H1,  int *ldim)

        //  SCF loop 
        int  Max_iter_scf = 10 ; int  iter_scf =0 ;
        err =1.0e0   ;  thrs_dHmat  = 1e-7  ;

        RmgTimer *RT2a ;    // timer type  declaration

        //-----   SCF loop  starts here: 
        while (err > thrs_dHmat &&  iter_scf <  Max_iter_scf)  {

            //RmgTimer *RT2a = new RmgTimer("2-TDDFT: ELDYN");
            RT2a = new RmgTimer("2-TDDFT: ELDYN");
            magnus (Hmatrix_0,    Hmatrix_1 , &time_step, Hmatrix_dt , &numst) ; 
            eldyn_(&numst, Smatrix, Hmatrix_dt, Pn0, Pn1, &Ieldyn, &iprint);
            delete(RT2a);

            // if(pct.gridpe == 0) { printf("**** Pn1 : \n");   print_matrix_z(Pn1,  &nblock, &numst)  ; }

            //            for(i = 0; i < 10; i++) 
            //            { printf("Pn\n");
            //           for(int j = 0; j < 10; j++) printf(" %8.1e", i, Pn1[i*numst + j]);
            //          }


            /////// <----- update Hamiltonian from  Pn1
            RT2a = new RmgTimer("2-TDDFT: Rho");
            GetNewRho_rmgtddft((double *)Kptr[0]->orbital_storage, xpsi, rho, Pn1, numst);
            delete(RT2a);

            dcopy(&FP0_BASIS, vh_corr, &ione, vh_corr_old, &ione);
            dcopy(&FP0_BASIS, vh, &ione, vh_old, &ione);
            dcopy(&FP0_BASIS, vxc, &ione, vxc_old, &ione);

            //get_vxc(rho, rho_oppo, rhocore, vxc);
            double vtxc, etxc;
            RmgTimer *RT1 = new RmgTimer("2-TDDFT: exchange/correlation");
            Functional *F = new Functional ( *Rmg_G, Rmg_L, *Rmg_T, ct.is_gamma);
            F->v_xc(rho, rhocore, etxc, vtxc, vxc, ct.nspin);
            delete F;
            delete RT1;

            RT1 = new RmgTimer("2-TDDFT: Vh");
            VhDriver(rho, rhoc, vh, ct.vh_ext, 1.0-12);
            delete RT1;
            for (int idx = 0; idx < FP0_BASIS; idx++) {
                vtot[idx] = vxc[idx] + vh[idx]
                    -vxc_old[idx] -vh_old[idx];
            }

            GetVtotPsi (vtot_psi, vtot, Rmg_G->default_FG_RATIO);
            RT2a = new RmgTimer("2-TDDFT: Hupdate");
            HmatrixUpdate(Kptr[0], vtot_psi, (OrbitalType *)Hmatrix);                                     
            delete(RT2a);

            for(i = 0; i < n2; i++) Hmatrix[i] += Hmatrix_old[i];   // final update of  Hmatrix
            dcopy(&n2, Hmatrix, &ione, Hmatrix_old, &ione);         // saves Hmatrix to Hmatrix_old   

            //////////  < ---  end of Hamiltonian update

            // check error and update Hmatrix_1:
            tst_conv_matrix (&err, &ij_err ,  Hmatrix,  Hmatrix_1 ,  &numst) ;  //  check error  how close  H and H_old are
            dcopy(&n2, Hmatrix  , &ione, Hmatrix_1, &ione);

            if(pct.gridpe == 0) { printf("step: %5d  iteration: %d  thrs= %12.5e err=  %12.5e at element: %5d \n", 
                    tddft_steps, iter_scf,    thrs_dHmat,  err,         ij_err); } 
            //err= -1.0e0 ;  
            iter_scf ++ ;
        } //---- end of  SCF/while loop 


        /*  done with propagation,  save Pn1 ->  Pn0 */
        dcopy(&n22, Pn1, &ione, Pn0, &ione);

        //  extract dipole from rho(Pn1)
        get_dipole(rho, dipole_ele);
        if(pct.gridpe == 0)fprintf(dfi, "\n  %f  %18.10f  %18.10f  %18.10f ",
                tot_steps*time_step, dipole_ele[0], dipole_ele[1], dipole_ele[2]);




        if((tddft_steps +1) % ct.checkpoint == 0)
        {   
            RT2a = new RmgTimer("2-TDDFT: Write");
            WriteData_rmgtddft(ct.outfile_tddft, vh, vxc, vh_corr, Pn0, Hmatrix, Smatrix, Smatrix, tot_steps);
            delete RT2a;
            // fflush(NULL);
        }

        // save current  H0, H1 for the  next step extrapolatiion
        dcopy  (&n2, Hmatrix_0, &ione, Hmatrix_m1 , &ione);         
        //dcopy(&n2, Hmatrix  , &ione, Hmatrix_1  , &ione);         // this update is already done right after scf loop 
        dcopy  (&n2, Hmatrix_1, &ione, Hmatrix_0  , &ione);        

    }

    if(pct.gridpe == 0) fclose(dfi);


    RmgTimer *RT2a = new RmgTimer("2-TDDFT: Write");
    WriteData_rmgtddft(ct.outfile_tddft, vh, vxc, vh_corr, Pn0, Hmatrix, Smatrix, Smatrix, tot_steps+1);
    delete RT2a;
    delete RT0;
}



