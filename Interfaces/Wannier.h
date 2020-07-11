/*
 *
 * Copyright 2019 The RMG Project Developers. See the COPYRIGHT file 
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


#ifndef RMG_Wannier_H
#define RMG_Wannier_H 1


#include <string>
#include <vector>
#include <set>
#include <complex>
#include <mutex>
#include "BaseGrid.h"
#include "Lattice.h"
#include "Symmetry.h"
#include "Exxbase.h"


// Screening types
#define ISOLATED_ENTANGLEMENT 0
#define GAU_ENTANGLEMENT 1
#define ERFC_ENTANGLEMENT 2


template <typename T> class Wannier {

private:
    // BaseGrid class (distributed) and half grid

    Exxbase<T> *Exx;
    BaseGrid &G;

    // Lattice object
    Lattice &L;

    // File path for wavefunction file. Spin and kpoint identifiers should be added by parent.
    const std::string &wavefile;
    // Number of occupied orbitals
    int nstates;
    int n_wannier;
    int scdm;
    double scdm_mu;
    double scdm_sigma;
    // Occupations for the orbitals

    // Base of domain distributed wavefunction array
    T *psi;


    // Grid points on this processing node
    int ngrid, ngrid_noncoll;

    // Mmapped serial wavefunction array
    T *psi_s;

    // File descriptor for serial wavefile
    int serial_fd;


public:
    Wannier (
            BaseGrid &G, 
            Lattice &L, 
            const std::string &wavefile,
            int nstates,
            int n_wannier,
            int scdm,
            double scdm_mu,
            double scdm_sigma,
            T *psi_in);

    ~Wannier(void);

    void SetEntanglement(int scdm_entan, double mu, double sigma);
    void SetAmn();
    void SetMmn();
    void WriteWinEig();

};

#endif

