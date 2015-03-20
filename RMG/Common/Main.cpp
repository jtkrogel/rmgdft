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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h> 
#if (defined(_WIN32) || defined(_WIN64))
    #include <io.h>
#else
    #include <unistd.h>
#endif
#include <unordered_map>
#include "grid.h"
#include "const.h"
#include "RmgTimer.h"
#include "RmgException.h"
#include "rmgtypedefs.h"
#include "params.h"
#include "typedefs.h"
#include "rmg_error.h"
#include "transition.h"
#include "Kpoint.h"
#include "InputKey.h"
#include "blas.h"


#include "../Headers/common_prototypes.h"
#include "../Headers/common_prototypes1.h"

extern "C" void lbfgs_init(int num_ions, int num_images);

void initialize (int argc, char **argv);

template <typename OrbitalType> void run (Kpoint<OrbitalType> **Kptr);

void report (void);

void finish (void);


/* Electronic charge density or charge density of own spin in polarized case */
double *rho;

/*  Electronic charge density of pposite spin density*/
double *rho_oppo;  


/* Core Charge density */
double *rhocore;


/* Compensating charge density */
double *rhoc;


/* Hartree potential */
double *vh;

/* Nuclear local potential */
double *vnuc;

/* Exchange-correlation potential */
double *vxc;

// Pointer to Kpoint class arrays for gamma and non-gamma
Kpoint<double> **Kptr_g;
Kpoint<std::complex<double> > **Kptr_c;

double *tau;
/* Main control structure which is declared extern in main.h so any module */
/* may access it.					                 */
CONTROL ct;

/* Global MPI stuff. Overridden by input params */
int NPES=1;

/* PE control structure which is also declared extern in main.h */
PE_CONTROL pct;

std::unordered_map<std::string, InputKey *> ControlMap;

int main (int argc, char **argv)
{

#if (defined(_WIN32) || defined(_WIN64))
    // make default io binary
     _set_fmode( _O_BINARY);
#endif

    RmgTimer *RT = new RmgTimer("Main");
    char *tptr;

    /* Define a default output stream, gets redefined to log file later */
    ct.logfile = stdout;

#if GPU_ENABLED
//  Hack to force initialization of libsci on Cray before we create our own threads
    char *trans = "n";
    int asize = 32, i, j;
    double alpha = 1.0;
    double beta = 0.0;
    double A[32*32], B[32*32], C[32*32];

    for(i = 0;i < asize * asize;i++) {
        A[i] = 1.0;
        B[i] = 0.0;
        C[i] = 1.0;
    }


    dgemm (trans, trans, &asize, &asize, &asize, &alpha, A, &asize,
               B, &asize, &beta, C, &asize);
#endif


    // Get RMG_MPI_THREAD_LEVEL environment variable
    ct.mpi_threadlevel = MPI_THREAD_SERIALIZED;
    if(NULL != (tptr = getenv("RMG_MPI_THREAD_LEVEL"))) {
        ct.mpi_threadlevel = atoi(tptr);
    }

    try {

        RmgTimer *RT1 =  new RmgTimer("Main: init");
        initialize (argc, argv);
        delete(RT1);


        RmgTimer *RT2 = new RmgTimer("Main: run");
        if(ct.is_gamma)
            run<double> ((Kpoint<double> **)Kptr_g);
        else
            run<std::complex<double> >((Kpoint<std::complex<double>> **)Kptr_c);
        delete(RT2);

    }

    // Catch exceptions issued by us.
    catch(RmgFatalException const &e) {
        std::cout << e.rwhat() << std::endl;
        MPI_Finalize();
        exit(0);
    }

    // By std
    catch (std::exception &e) {
        std::cout << "Caught a std exception: " << e.what () << std::endl;
        MPI_Finalize();
        exit(0);
    } 

    // Catchall
    catch (...) {
        std::cout << "Caught an unknown exception of some type." << std::endl;
        MPI_Finalize();
        exit(0);
    } 

    delete(RT);   // Destructor has to run before report
    report ();

    finish ();
    
    return 0;
}


void initialize(int argc, char **argv) 
{

    int FP0_BASIS;

    /* start the benchmark clock */
    ct.time0 = my_crtc ();
    RmgTimer *RT = new RmgTimer("Pre-init");


    /* Initialize all I/O including MPI group comms */
    /* Also reads control and pseudopotential files*/
    InitIo (argc, argv, ControlMap);

    FP0_BASIS = Rmg_G->get_P0_BASIS(Rmg_G->default_FG_RATIO);

    int num_images = pct.images;
    num_images = 1;
    lbfgs_init(ct.num_ions, num_images);

    rho = new double[FP0_BASIS];
    rhocore = new double[FP0_BASIS];
    rhoc = new double[FP0_BASIS];
    vh = new double[FP0_BASIS];
    vnuc = new double[FP0_BASIS];
    vxc = new double[FP0_BASIS];
    if (ct.xctype == MGGA_TB09) 
    	tau = new double[FP0_BASIS];

    /* for spin polarized calculation, allocate memory for density of the opposite spin */
    if(ct.spin_flag)
            rho_oppo = new double[FP0_BASIS];


    /* Initialize some k-point stuff */
    Kptr_g = new Kpoint<double> * [ct.num_kpts];
    Kptr_c = new Kpoint<std::complex<double> > * [ct.num_kpts];

    ct.is_gamma = true;
    for (int kpt = 0; kpt < ct.num_kpts; kpt++) {
        double v1, v2, v3;
        v1 = twoPI * ct.kp[kpt].kpt[0] / Rmg_L.get_xside();
        v2 = twoPI * ct.kp[kpt].kpt[1] / Rmg_L.get_yside();
        v3 = twoPI * ct.kp[kpt].kpt[2] / Rmg_L.get_zside();

        ct.kp[kpt].kvec[0] = v1;
        ct.kp[kpt].kvec[1] = v2;
        ct.kp[kpt].kvec[2] = v3;
        ct.kp[kpt].kmag = v1 * v1 + v2 * v2 + v3 * v3;

        if(ct.kp[kpt].kmag != 0.0) ct.is_gamma = false;
    }

    if(!ct.is_gamma) 
    {
        ct.is_use_symmetry = 1;
        rmg_printf("\nUSING COMPLEX ORBITALS\n");
    }
    else
    {
        rmg_printf("\nUSING REAL ORBITALS\n");
    }

    for (int kpt = 0; kpt < ct.num_kpts; kpt++)
    {

        if(ct.is_gamma) {

            // Gamma point
            Kptr_g[kpt] = new Kpoint<double> (ct.kp[kpt].kpt, ct.kp[kpt].kweight, kpt, pct.grid_comm, Rmg_G, Rmg_T, &Rmg_L, ControlMap);

        }
        else {

            // General case
            Kptr_c[kpt] = new Kpoint<std::complex<double>> (ct.kp[kpt].kpt, ct.kp[kpt].kweight, kpt, pct.grid_comm, Rmg_G, Rmg_T, &Rmg_L, ControlMap);

        }
        ct.kp[kpt].kidx = kpt;
    }


    my_barrier ();

    /* Record the time it took from the start of run until we hit init */
    delete(RT);

    /* Perform any necessary initializations */
    if(ct.is_gamma) {
        Init (vh, rho, rho_oppo, rhocore, rhoc, vnuc, vxc, Kptr_g);
    }
    else {
        Init (vh, rho, rho_oppo, rhocore, rhoc, vnuc, vxc, Kptr_c);
    }


    /* Need if statement here, otherwise job output file 
     * will also show information of control file ? */
    if (pct.imgpe == 0)
    {
    
        /* Write header to stdout */
        write_header (); 

    }


    /* Write state occupations to stdout */
    if(ct.is_gamma) {
        Kptr_g[0]->write_occ(); 
    }
    else {
        Kptr_c[0]->write_occ(); 
    }

    
    /* Flush the results immediately */
    fflush (NULL);



    /* Wait until everybody gets here */
    /* MPI_Barrier(MPI_COMM_WORLD); */
    MPI_Barrier(pct.img_comm);


}

template <typename OrbitalType> void run (Kpoint<OrbitalType> **Kptr)
{


    /* Dispatch to the correct driver routine */
    switch (ct.forceflag)
    {

    case MD_QUENCH:            /* Quench the electrons */
    	if (ct.xctype == MGGA_TB09)
        	//relax_tau (0, states, vxc, vh, vnuc, rho, rho_oppo, rhocore, rhoc, tau);
                ;
	else 
        	Relax (0, vxc, vh, vnuc, rho, rho_oppo, rhocore, rhoc, Kptr);
        break;

    case MD_FASTRLX:           /* Fast relax */
        Relax (ct.max_md_steps, vxc, vh, vnuc, rho, rho_oppo, rhocore, rhoc, Kptr);
        break;

    case NEB_RELAX:           /* nudged elastic band relax */
// Fix later. Calls C version of relax which is deprecated
//        neb_relax (states, vxc, vh, vnuc, rho, rho_oppo, rhocore, rhoc);
        break;

    case MD_CVE:               /* molecular dynamics */
    case MD_CVT:
    case MD_CPT:
        Quench (vxc, vh, vnuc, rho, rho_oppo, rhocore, rhoc, Kptr);
        //moldyn (states, vxc, vh, vnuc, rho, rho_oppo, rhoc, rhocore);
        break;

    case BAND_STRUCTURE:
        BandStructure (Kptr, vxc, vh, vnuc);
        break;
    default:
        rmg_error_handler (__FILE__, __LINE__, "Undefined MD method");


    }

    if(!ct.is_gamma)
        OutputBandPlot(Kptr);

    if(Verify ("output_rho_xsf", true, Kptr[0]->ControlMap))
        Output_rho_xsf(rho, pct.grid_comm);

}                               /* end run */

void report ()
{

    /* write planar averages of quantities */
    if (ct.zaverage == 1)
    {
        /* output the average potential */
        write_avgv (vh, vnuc);
        write_avgd (rho);
    }
    else if (ct.zaverage == 2)
    {
        //write_zstates (states);
        ;
    }


    /* If milliken population info is requested then compute and output it */
    /*if (ct.domilliken)
      mulliken (states);*/


    /* Release the memory for density of opposite spin */
    if (ct.spin_flag)
        delete [] rho_oppo;
    if (ct.xctype == MGGA_TB09) 
        delete [] tau;

    /* Write timing information */
    //    write_timings ();
    if(pct.imgpe == 0) fclose(ct.logfile);
    RmgPrintTimings(Rmg_G, ct.logname, ct.scf_steps);



}                               /* end report */


void finish ()
{

#if SCALAPACK_LIBS
    /*Exit Scalapack */
    if (pct.scalapack_pe)
        sl_exit (pct.ictxt);
#endif

    MPI_Barrier(MPI_COMM_WORLD);
    /*Exit MPI */
    MPI_Finalize ();

#if GPU_ENABLED
    finalize_gpu();
#endif

}                               /* end finish */


/******/
