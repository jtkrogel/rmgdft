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

#ifndef RMG_Kpoint_H
#define RMG_Kpoint_H 1

#include "BaseGrid.h"
#include "Lattice.h"
#include "TradeImages.h"
#include "State.h"
#include <mpi.h>

template <typename KpointType> class Kpoint {

public:

    Kpoint(double *kpt, double kweight, int nstates, int index, MPI_Comm newcomm, BaseGrid *newG, TradeImages *newT, Lattice *newL );

    void set_pool(KpointType *pool);
    void sort_orbitals(void);
    void random_init(void);
    int get_nstates(void);
    int get_index(void);
    void orthogonalize(double *storage);
    void orthogonalize(std::complex<double> *storage);
    void mix_betaxpsi(int mix);
    void mix_betaxpsi1(int istate);


    // BaseGrid class
    BaseGrid *G;

    // TradeImages object to use
    TradeImages *T;

    // Lattice object
    Lattice *L;

    // MPI communicator to use for trade images and reduction operations
    MPI_Comm comm;

    // The index of the k-point for backreferencing
    int kidx;

    // The k-point
    double kpt[3];

    // The corresponding vector
    double kvec[3];

    // The weight associated with the k-point
    double kweight;

    // The magnitude of the k-vector
    double kmag;

    // Number of orbitals
    int nstates;

    // Block of contiguous storage for the orbitals
    KpointType *orbital_storage;

    // The orbital structure for this k-point
    State<KpointType> *Kstates;

    // Legacy structure will be removed later
    STATE *kstates;

    // Pointer to sint arrays (Betaxpsi)
    KpointType *newsint_local;
    KpointType *oldsint_local;

    // Size of the sint arrays
    int sint_size;

    // Pointers to nv, ns, and Bns
    KpointType *nv;
    KpointType *ns;
    KpointType *Bns;

    // Pointers to weight and Bweight
    KpointType *nl_weight;
    KpointType *nl_Bweight;

    // Number of points in orbital basis
    int pbasis;

    // Mean min, and max wavefunction residuals for occupied space
    double meanres;
    double minres;
    double maxres;

    // Total energies */
    double ES;
    double NUC;
    double KE;
    double XC;
    double NL;
    double II;
    double TOTAL;


};

#endif
