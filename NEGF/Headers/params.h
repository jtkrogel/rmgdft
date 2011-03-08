/************************** SVN Revision Information **************************
 **    $Id$    **
******************************************************************************/
 
/****f* QMD-MGDFT/params.h *****
 * NAME
 *   Ab initio real space code with multigrid acceleration
 *   Quantum molecular dynamics package.
 *   Version: 2.1.5
 * COPYRIGHT
 *   Copyright (C) 1995  Emil Briggs
 *   Copyright (C) 1998  Emil Briggs, Charles Brabec, Mark Wensell, 
 *                       Dan Sullivan, Chris Rapcewicz, Jerzy Bernholc
 *   Copyright (C) 2001  Emil Briggs, Wenchang Lu,
 *                       Marco Buongiorno Nardelli,Charles Brabec, 
 *                       Mark Wensell,Dan Sullivan, Chris Rapcewicz,
 *                       Jerzy Bernholc
 * FUNCTION
 *   
 * INPUTS
 *
 * OUTPUT
 *  
 * PARENTS
 *
 * CHILDREN
 * 
 * SEE ALSO
 *  
 * SOURCE
 */

/*

                            params.h


    Compile time parameters for md program.


*/


/*************************************************************
 *************************************************************

    All compile time parameters are now set in the top-level
    Makefile. Don't touch anything in this file unless you
    know what you are doing.

 *************************************************************
 *************************************************************/
#include "arch.h"

#define  GAMMA_PT    1

/* This is the maximum number of non-local projectors for any species
 * just an upper limit on dynamically allocated data structures. */
#define         MAX_NL  20
#define         MAX_NB  6
#define         MAX_CHAR  255
#define         MAX_OPTS  12556


/* Maximum number of states -- just an upper limit on 
 * dynamically allocated data structures. */
#define         MAX_STATES   10000

/* Maximum  Number of processors    	*/ 
#define 	MAX_PES 	2048 

/* Maximum number of species -- just an upper limit on
 * dynamically allocated data structures. */
#define         MAX_SPECIES     6


/* Maximum number of ions -- just an upper limit on
 * dynamically allocated data structures. */
#define         MAX_IONS        1400
#define         IONS_PER_PE     MAX_IONS

/* Maximum l-value (Angular momentum channel for pseudotentials) */
#define         MAX_L              4


/* Number of k-points -- just an upper limit on
 * dynamically allocated data structures. */ 
#define         MAX_KPTS           32



/* Maximum pathname length for input file names */
#define         MAX_PATH        200


/* Maximum number of points in pseudopotential radial grid */
#define         MAX_RGRID           1300


/* Size of the linear interpolation grid for the potentials */
#define         MAX_LOCAL_LIG       (9000)

/* Size of the linear interpolation grid for the qfunction */
#define         MAX_QLIG       (9000)

/* Max. number of states localized on the same ion */
#define MAX_LOC_ST      20
#define MAX_BLOCKS      20

#define                MAX_PWRK        (50000)

/******/
