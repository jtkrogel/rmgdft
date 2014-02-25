/************************** SVN Revision Information **************************
 **    $Id$    **
******************************************************************************/
 
/****f* QMD-MGDFT/distri_fermi.c ************
 * NAME
 *   Ab initio real space code with multigrid acceleration
 *   Quantum molecular dynamics package.
 *   Version: 2.1.5
 * COPYRIGHT
 * FUNCTION
 *   void set_energy_weight(real eneR, real eneI, real weight, nenergy)
 *   set up the energies and weights and # of Green functions 
 * INPUTS
 *   
 * OUTPUT
 *   nothing
 * PARENTS
 *   too many
 * CHILDREN
 *   nothing
 * SOURCE
 */


#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "init_var.h"
#include "LCR.h"

/* This function returns a pointer to a block of memory of size nelem. */
void distri_fermi (complex double ene,  rmg_double_t EF, complex double *distri)
{

    rmg_double_t temR, temI, tem1, tem2;
    
    complex double ctem;
    rmg_double_t KT;

    KT = cei.KT;
    ctem = (ene - EF) / KT;
    *distri = 1.0/( 1.0 + cexp (ctem));

}
