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

#include <complex>
#include <omp.h>
#include "FiniteDiff.h"
#include "const.h"
#include "rmgtypedefs.h"
#include "typedefs.h"
#include "rmg_error.h"
#include "RmgTimer.h"
#include "GlobalSums.h"
#include "Kpoint.h"
#include "Subdiag.h"
#include "RmgGemm.h"
#include "GpuAlloc.h"
#include "ErrorFuncs.h"
#include "RmgException.h"
#include "Scalapack.h"
#include "blas.h"

#include "prototypes.h"
#include "common_prototypes.h"
#include "common_prototypes1.h"
#include "transition.h"
#if GPU_ENABLED
    #include <cuda.h>
    #include <cuda_runtime_api.h>
    #include <cublas_v2.h>
    #if MAGMA_LIBS
        #include <magma.h>
    #endif

#endif


#define FOLDED_GSE 1



// Array storage for folded spectrum diagonalization communications
static int *fs_eigstart = NULL;
static int *fs_eigstop = NULL;
static int *fs_eigcounts = NULL;


// I have not finished updating this to work with complex orbitals yet. Given that the folded spectrum method is only
// useful for large systems which are almost always run at gamma with real orbitals it's not a high priority but should
// be straightforward enough to finish.
template int FoldedSpectrumScalapack<double> (Kpoint<double> *, int, double *, int, double *, int, double *, double *, Scalapack*, int, int);

// Just to note here the inputs A,B and C are full matrices in the root communicator
template <typename KpointType>
int FoldedSpectrumScalapack(Kpoint<KpointType> *kptr, int n, KpointType *A, int lda, KpointType *B, int ldb, 
		double *eigs, KpointType *C, Scalapack* MainSp, int driver, int blocksize)
{

    RmgTimer RT0("Diagonalization: fs:");
    RmgTimer *RT1, *RT2;

    KpointType ZERO_t(0.0);
    KpointType ONE_t(1.0);
    KpointType *NULLptr = NULL;

    char *trans_t="t", *trans_n="n";
    char *cuplo = "L", *jobz="V";

    int ione=1, itype=1, info=0, izero=0;
    double rone = 1.0, rzero = 0.0;

    BaseGrid *Grid = kptr->G;
    Lattice *L = kptr->L;


    // For mpi routines. Transfer twice as much data for complex orbitals
    int factor = 2;
    if(ct.is_gamma) factor = 1;

    // Folded spectrum scalapacks
    Scalapack *FSp = MainSp->GetNextScalapack();
    int FSp_my_row = FSp->GetRow();
    int FSp_my_col = FSp->GetCol();
    int FSp_rows = FSp->GetRows();
    int FSp_cols = FSp->GetCols();
    int FSp_nodes = FSp_rows * FSp_cols;
    int FSp_context = FSp->GetContext();
    int FSp_my_index = FSp_my_row*FSp_cols + FSp_my_col;

    // descriptor for full matrix in main scalapack instance
    int *m_f_desca = MainSp->GetDistDesca();
    int m_f_dist_length = MainSp->ComputeMdim(n) *  MainSp->ComputeNdim(n);
    

    // Number of groups at the sub level
    int FS_NPES = MainSp->GetNumGroupsNext();
    if(FS_NPES < 12) // Not much point in trying to use folded spectrum in this case
        throw RmgFatalException() << "Insufficient PE's to use folded spectrum in " << __FILE__ << " at line " << __LINE__ << ". Terminating.\n";


    // Allocate some memory for our communication book keeping arrays
    if(!fs_eigstart) {
        fs_eigstart = new int[FS_NPES];
        fs_eigstop = new int[FS_NPES];
        fs_eigcounts = new int[FS_NPES];
    }


    // Set up partition indices and bookeeping arrays
    int eig_start, eig_stop, eig_step;
    int n_start, n_win;
blocksize = 1;
    FoldedSpectrumSetup(n, FS_NPES, FSp->GetGroupIndex(), eig_start, eig_stop, eig_step,
                        n_start, n_win, fs_eigstart, fs_eigstop, fs_eigcounts, blocksize);

    // A PE is actually a scalapack instance
    int THIS_PE = FSp->GetGroupIndex();

    // Get dist length and desca for the submatrices
    int s_s_desca[DLEN];
    int s_s_dist_length = FSp->ComputeDesca(n_win, n_win, s_s_desca);
    int s_f_desca[DLEN];
    int s_f_dist_length = FSp->ComputeDesca(n, n, s_f_desca);


    MPI_Comm scalapack_comm = FSp->GetComm();

    static double *distAsave;
    static double *distBsave;
    static double *distA;
    static double *distB;
    static double *m_distA;
    static double *m_distB;
    static double *distV;
    static double *Asave;
    static double *Bsave;
    static double *n_eigs;
    if(!distAsave) {
         int retval1 = MPI_Alloc_mem(s_f_dist_length * sizeof(double) , MPI_INFO_NULL, &distAsave);
         int retval2 = MPI_Alloc_mem(s_f_dist_length * sizeof(double) , MPI_INFO_NULL, &distBsave);
         int retval3 = MPI_Alloc_mem(s_f_dist_length * sizeof(double) , MPI_INFO_NULL, &distA);
         int retval4 = MPI_Alloc_mem(s_f_dist_length * sizeof(double) , MPI_INFO_NULL, &distB);
         int retval5 = MPI_Alloc_mem(m_f_dist_length * sizeof(double) , MPI_INFO_NULL, &m_distA);
         int retval6 = MPI_Alloc_mem(m_f_dist_length * sizeof(double) , MPI_INFO_NULL, &m_distB);
         int retval7 = MPI_Alloc_mem(s_f_dist_length * sizeof(double) , MPI_INFO_NULL, &distV);
         int retval8 = MPI_Alloc_mem(n * n * sizeof(double) , MPI_INFO_NULL, &Asave);
         int retval9 = MPI_Alloc_mem(n * n * sizeof(double) , MPI_INFO_NULL, &Bsave);
         int retval10 = MPI_Alloc_mem(n * sizeof(double) , MPI_INFO_NULL, &n_eigs);
         if((retval1 != MPI_SUCCESS) || (retval2 != MPI_SUCCESS) || (retval3 != MPI_SUCCESS) || (retval4 != MPI_SUCCESS) || 
            (retval5 != MPI_SUCCESS) || (retval6 != MPI_SUCCESS) || (retval7 != MPI_SUCCESS) || (retval8 != MPI_SUCCESS) ||
            (retval9 != MPI_SUCCESS) || (retval10 != MPI_SUCCESS)) {
            rmg_error_handler (__FILE__, __LINE__, "Memory allocation failure in FoldedSpectrum_Scalapack");
         }
    }

    //  Transform problem to standard eigenvalue form. For now this is done in the Main sp using
    // pdsygst which is equivalent to FOLDED_GSE=1 in the non-scalapack case.
    {
        RT2 = new RmgTimer("Diagonalization: fs: transform");

        // Copy A and B into m_distA, m_distB
        MainSp->CopySquareMatrixToDistArray(A, m_distA, n, m_f_desca);
        MainSp->CopySquareMatrixToDistArray(B, m_distB, n, m_f_desca);

        double scale = 1.0;
        pdpotrf_( cuplo, &n, m_distB, &ione, &ione, m_f_desca, &info );
        if(info != 0) throw RmgFatalException() << "pdpotrf failed in " << __FILE__ << " at line " << __LINE__ << ".\n";
        pdsygst_(&itype, cuplo, &n, m_distA, &ione, &ione, m_f_desca, m_distB, &ione, &ione, m_f_desca, &scale, &info);

        delete(RT2);
    }


    // Copy transformed matrix back to local matrix and then broadcast to other nodes
    // Possible optimization is to only broadcast to root of each subscalapack
    MainSp->GatherMatrix(A, m_distA);
    MainSp->Bcast(A, factor * n * n, MPI_DOUBLE);


    KpointType *V = new KpointType[n*n]();
    KpointType *G = new KpointType[n*n]();
    for(int idx = 0;idx < n*n;idx++) Asave[idx] = A[idx];

    // Zero out matrix of eigenvectors (V)
    for(int ix = 0;ix < s_f_dist_length;ix++) distV[ix] = ZERO_t;

    // AX=lambdaX  store a copy of distA in distAsave
    for(int ix = 0;ix < s_f_dist_length;ix++) distAsave[ix] = distA[ix];

     
    // Do the submatrix along the diagonal to get starting values for folded spectrum
    //--------------------------------------------------------------------
    RT2 = new RmgTimer("Diagonalization: fs: submatrix");
    {

        for(int ix = 0;ix < n_win;ix++){
            for(int iy = 0;iy < n_win;iy++){
                G[ix*n_win + iy] = Asave[(n_start+ix)*n + n_start + iy];
            }
        }
        for(int idx = 0;idx < n_win * n_win;idx++) A[idx] = G[idx];

        // Set up dist arrays
        FSp->CopySquareMatrixToDistArray(A, distA, n_win, s_s_desca);

        int lwork=-1, liwork=-1, liwork_tmp;
        double lwork_tmp;
        lwork = -1;
        pdsyevd_(jobz, cuplo, &n_win, distA, &ione, &ione, s_s_desca, &eigs[n_start],
                distV, &ione, &ione, s_s_desca, &lwork_tmp, &lwork, &liwork_tmp, &liwork, &info);
        lwork = 8*(int)lwork_tmp;
lwork = 6*n*n;
        liwork = 16*n;
        double *work = new double[lwork];
        int *iwork = new int[liwork];

        pdsyevd_(jobz, cuplo, &n_win, distA, &ione, &ione, s_s_desca, &eigs[n_start],
                 distV, &ione, &ione, s_s_desca, work, &lwork, iwork, &liwork, &info);

        delete [] iwork;
        delete [] work;
        if( info != 0 ) 
                rmg_error_handler(__FILE__, __LINE__, "pdsyevd failure");

    }

    // Copy distributed matrix into local. All nodes in the local scalapack will get
    // a copy
    int lld = std::max( numroc_( &n_win, &n_win, &FSp_my_row, &izero, &FSp_rows ), 1 );
    int local_desca[DLEN];
    descinit_( local_desca, &n_win, &n_win, &n_win, &n_win, &izero, &izero, &FSp_context, &lld, &info );
    pdgeadd_( "N", &n_win, &n_win, &rone, distV, &ione, &ione, s_s_desca, &rzero, G, &ione, &ione, local_desca );

    // Make sure the sign is the same for all groups when copied back to the main array
    double *Vdiag = new double[n];

    for(int ix = 0;ix < n_win;ix++) {
        Vdiag[ix] = ONE_t;
        if(G[ix*n_win + ix] < 0.0) Vdiag[ix] = -ONE_t;
    }

    // Store the eigen vectors from the submatrix
    if(FSp_my_index == 0) {
        for(int ix = 0;ix < n_win;ix++) {

            if(((n_start+ix) >= eig_start) && ((n_start+ix) < eig_stop)) {

                for(int iy = 0;iy < n_win;iy++) {
                      V[(ix + n_start)*n + n_start + iy] = Vdiag[ix] * G[ix * n_win + iy];
                }

            }

        }
    }

    delete [] Vdiag;
    delete(RT2);  // submatrix part done

    // Every node in a scalapack group has a copy of it's set of eigenvalues
    // which we have to get to all nodes so use an Allreduce but make sure
    // only the first node in each grup is non-zero
    for(int idx = 0;idx < n;idx++) n_eigs[idx] = 0.0;
    if(FSp_my_index == 0) {
        for(int eig_index = eig_start;eig_index < eig_stop;eig_index++) {
            n_eigs[eig_index] = eigs[eig_index];
        }
    }

    // Use async MPI to get a copy of the eigs to everyone. Will overlap with the iterator
    MPI_Barrier(MainSp->GetComm());
    MPI_Request MPI_reqeigs;
    MPI_Iallreduce(MPI_IN_PLACE, n_eigs, n, MPI_DOUBLE, MPI_SUM, MainSp->GetComm(), &MPI_reqeigs);


    // Since we have the full Asave present on every physical node we can parallelize the iteration over
    // all of them without communication but we have to recompute our starting and stopping points since
    // each scalapack instance may consist of many physical nodes
    RT2 = new RmgTimer("Diagonalization: fs: iteration");
    int chunksize = (eig_stop - eig_start) / FSp_nodes;
    int offset = chunksize * FSp_my_index;
    int rem = (eig_stop - eig_start) % FSp_nodes;
    if(FSp_my_index < rem) {
        chunksize++;
        offset += FSp_my_index;
    }
    else {
        offset += rem;
    }

    if(chunksize)
        FoldedSpectrumIterator(Asave, n, &eigs[eig_start + offset], chunksize, &V[(eig_start + offset) * n], -0.5, 10, driver);
    delete(RT2);
     
    // Wait for eig request to finish and copy summed eigs from n_eigs back to eigs
    MPI_Wait(&MPI_reqeigs, MPI_STATUS_IGNORE);
    for(int idx = 0;idx < n;idx++) eigs[idx] = n_eigs[idx];

    // Make sure all nodes in this scalapack have copies of the eigenvectors
//    MPI_Allreduce(MPI_IN_PLACE, &V[eig_start*n], n_win, MPI_DOUBLE, MPI_SUM, FSp->GetComm());
#if 0
        // Make sure all PE's have all eigenvectors.
        RT2 = new RmgTimer("Diagonalization: fs: allreduce1");
        MPI_Allgatherv(MPI_IN_PLACE, eig_step * n * factor, MPI_DOUBLE, V, fs_eigcounts, fs_eigstart, MPI_DOUBLE, pct.grid_comm);
        delete(RT2);
#endif
    MPI_Allreduce(MPI_IN_PLACE, V, n*n, MPI_DOUBLE, MPI_SUM, MainSp->GetComm());

    // Gram-Schmidt ortho for eigenvectors.
    RT2 = new RmgTimer("Diagonalization: fs: Gram-Schmidt");
    MainSp->CopySquareMatrixToDistArray(V, m_distA, n, m_f_desca);
    FoldedSpectrumScalapackOrtho(n, eig_start, eig_stop, fs_eigcounts, fs_eigstart, m_distA, V, NULLptr, MainSp);
    delete(RT2);


    // Back transform eigenvectors
    {
        RT2 = new RmgTimer("Diagonalization: fs: back transform");
        pdtrsm_("Left", cuplo, "T", "N", &n, &n, &rone, (double *)m_distB, &ione, &ione, m_f_desca,
                        (double *)m_distA, &ione, &ione, m_f_desca);
        delete(RT2);
    }
    MainSp->GatherMatrix(A, m_distA);
    MainSp->Bcast(A, factor * n * n, MPI_DOUBLE);


    delete [] G;
    delete [] V;

    return 0;
} 
