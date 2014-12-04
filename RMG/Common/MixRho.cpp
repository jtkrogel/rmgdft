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

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>
#include <string>
#include <cfloat>
#include <climits>
#include <unordered_map>
#include <typeinfo>
#include "const.h"
#include "InputKey.h"
#include "common_prototypes.h"
#include "transition.h"


void MixRho (double * new_rho, double * rho, double *rhocore, int length, int length_x, int length_y, int length_z, std::unordered_map<std::string, InputKey *>& ControlMap)
{
    double t1, nspin = (ct.spin_flag + 1.0);
    static double **rhohist=NULL, **residhist=NULL;

    if(Verify ("freeze_occupied", true, ControlMap)) return;

    /*Linear Mixing*/
    if (Verify("charge_mixing_type","Linear", ControlMap) || ct.charge_pulay_order == 1)
    {
	
	/* Scale old charge density first*/
	t1 = 1.0 - ct.mix;
        for(int ix = 0;ix < length;ix++) rho[ix] *= t1;

	/*Add the new density*/
        for(int ix = 0;ix < length;ix++) rho[ix] += ct.mix * new_rho[ix];
    }
    else {
	if (Verify("charge_mixing_type","Pulay", ControlMap))
	{
	    int step = ct.scf_steps;

	    if (ct.charge_pulay_refresh)
		step = ct.scf_steps % ct.charge_pulay_refresh;

	    /*Use pulay mixing, result will be in rho*/
	    pulay_rho(step, length, length_x, length_y, length_z, new_rho, rho, ct.charge_pulay_order, &rhohist, &residhist, ct.charge_pulay_special_metrics, ct.charge_pulay_special_metrics_weight);
	    
	}
	    
    }


    /*Find charge minimum */
    double min = ZERO;
    double min2 = ZERO;
    for (int idx = 0; idx < length; idx++)
    {
        if (rho[idx] < min)
            min = rho[idx];
        
	/*Here we check charge density with rhocore added*/
	if ((rho[idx] + rhocore[idx] / nspin) < min2)
         	min2 = rho[idx] + rhocore[idx] / nspin;
    }




    /*Find absolute minimum from all PEs */
    min = real_min_all (min, pct.img_comm);
    min2 = real_min_all (min2, pct.img_comm);

    if (min < ZERO)
    {
        printf ("\n\n Charge density is NEGATIVE after interpolation, minimum is %e", min);
        printf ("\n Minimum charge density with core charge added is %e", min2);
    }

}
