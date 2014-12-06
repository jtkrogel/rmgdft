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


#ifndef RMG_Scalapack_H
#define RMG_Scalapack_H 1

#include <mpi.h>


/* Blacs dimension */
#define DLEN    9

// npes is the total number of pes, ngroups is the number of
// scalapack groups that should be created out of npes where
// npes <= group_pes * ngroups * images_per_node

class Scalapack {

public:

    Scalapack(int ngroups, int thisimg, int images_per_node, int M, int N, int MB, int NB, MPI_Comm rootcomm);
    void DistributeMatrix(double *A, double *A_dist, int m, int n);
    void GatherMatrix(double *A, double *A_dist, int m, int n);

    void DistributeMatrix(std::complex<double> *A, std::complex<double> *A_dist, int m, int n);
    void GatherMatrix(std::complex<double> *A, std::complex<double> *A_dist, int m, int n);

    int GetRows(void);
    int GetCols(void);
    int GetDistMdim(void);
    int GetDistNdim(void);
    int GetCommRank(void);
    int GetRootRank(void);
    int *GetDistDesca(void);
    int GetIpivSize(void);
    bool Participates(void);
    MPI_Comm GetComm(void);

    void Allreduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op);

    void Pgemm (char *transa, char *transb, int *M, int *N, int *K, double *alpha,
                       double *A, int *IA, int *JA, int *desca,
                       double *B, int *IB, int *JB, int *descb,
                       double *beta, double *C, int *IC, int *JC, int *descc);

    void Pgemm (char *transa, char *transb, int *M, int *N, int *K, std::complex<double> *alpha,
                       std::complex<double> *A, int *IA, int *JA, int*desca,
                       std::complex<double> *B, int *IB, int *JB, int *descb,
                       std::complex<double> *beta, std::complex<double> *C, int *IC, int *JC, int *descc);

    void Pgesv (int *N, int *NRHS, double *A, int *IA, int *JA, int *desca, int *ipiv, double *B, int *IB,
                            int *JB, int *descb, int *info);

    void Pgesv (int *N, int *NRHS, std::complex<double> *A, int *IA, int *JA, int *desca, int *ipiv, std::complex<double> *B, int *IB,
                            int *JB, int *descb, int *info);

    void CopySquareMatrixToDistArray(double *A, double *A_dist, int n, int *desca);
    void CopySquareMatrixToDistArray(std::complex<double> *A, std::complex<double> *A_dist, int n, int *desca);

    ~Scalapack(void);

private:

    void matinit (double *globmat, double *dismat, int size, int *desca, bool isreal);

    int M;              // Operates on matrices of size (M,N)
    int N;              // Operates on matrices of size (M,N)
    int MB;             // Blocking factors
    int NB;
    int context;        // blacs context of this group of pes
    int npes;           // total number of pes
    int ngroups;        // total number of groups
    int group_pes;      // number of pes in this group
    int group_index;    // index of this group
    int group_rows;     // rows in this group
    int group_cols;     // cols in this group
    int my_row;         // blacs row of this PE
    int my_col;         // blacs col of this PE
    int root_rank;      // rank in rootcomm
    int comm_rank;      // rank in this comm
    int *local_desca;   // desca for local matrix
    int *dist_desca;    // desca for distributed matrix
    bool participates;  // whether or not this PE participates in scalapack calculations
    int m_dist;         // rows of distributed matrix
    int n_dist;         // cols of distributed matrix
    MPI_Comm comm;      // communicator for this object
    MPI_Comm root_comm; // parent communicator

};

extern "C" {

int numroc_ (int *, int *, int *, int *, int *);
int INDXG2P (int *, int *, int *, int *, int *);
void descinit_ (int[], int *, int *, int *, int *, int *, int *, int *, int *,
               int *);
void pdgesv_ (int *, int *, double *, int * , int *, int *, int *, double *,
        int *, int *, int *, int *);
void pzgesv_ (int *, int *, double *, int * , int *, int *, int *, double *,
        int *, int *, int *, int *);
void pdgemm_ (char *, char *, int *, int *, int *, double *, double *, int *,
             int *, int *, double *, int *, int *, int *, double *, double *,
             int *, int *, int *);
void pzgemm_ (char *, char *, int *, int *, int *, double *, double *, int *,
             int *, int *, double *, int *, int *, int *, double *, double *,
             int *, int *, int *);
void pdsyev_ (char *, char *, int *, double *, int *, int *, int *, double *,
             double *, int *, int *, int *, double *, int *, int *);
void pcheev_ (char *, char *, int *, double *, int *, int *, int *, double *,
             double *, int *, int *, int *, double *, int *, double *, int *, int *);
void PSPOCON (char *, int *, double *, int *, int *, int *, double *, double *,
              double *, int *, int *, int *, int *);
void PSPOTRF (char *, int *, double *, int *, int *, int *, int *);
void PSPOTRI (char *, int *, double *, int *, int *, int *, int *);
void PSSYGST (int *, char *, int *, double *, int *, int *, int *, double *,
              int *, int *, int *, double *, int *);
void PSTRTRS (char *, char *, char *, int *, int *, double *, int *, int *, int *,
              double *, int *, int *, int *, int *);
void PSSYMM (char *, char *, int *, int *, double *, double *, int *, int *,
             int *, double *, int *, int *, int *, double *, double *, int *,
             int *, int *);

void PSUBDIAG (char *, char *, int, double *, int, double *, int *);
void PDSYGVX(int *, char*, char*, char*, int*, double *, int*, int*, int*, double*, int*, int*,
       int*, double*, double *, int*, int*, double*, int*, int*, double*, double*, double*, int*,
       int*, int*, double*, int*, int*, int*, int*, int*, double*, int*);
void PZHEGVX(int *, char*, char*, char*, int*, double *, int*, int*, int*, double*, int*, int*,
       int*, double*, double *, int*, int*, double*, int*, int*, double*, double*, double*, int*,
       int*, int*, double*, int *, double *, int*, int*, int*, int*, int*, double*, int*);
void PDSYEVX(char*, char*, char*, int*, double *, int*, int*, int*, double*, double*, int*,
       int*, double*, int*, int*, double*, double*, double*, int*,
       int*, int*, double*, int*, int*, int*, int*, int*, double*, int*);
void pdgeadd_(char *, int *, int *, double *, double *, int *, int *, int *, double *,
       double *, int *, int *, int *);               
void pzgeadd_(char *, int *, int *, double *, double *, int *, int *, int *, double *,
       double *, int *, int *, int *);               

}


#endif