/************************** SVN Revision Information **************************
 **    $Id$    **
******************************************************************************/
 
/*
 * 
 *
 *
 */

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "md.h"


void update_orbit_centers(STATE * states)
{

    int st;
    int size;
    int shift_x, shift_y, shift_z;
    int size_x, size_y, size_z;
    int i, j, k;
    int new_i, new_j, new_k;
    int new_idx, old_idx;
    int level;
    REAL x, y, z;
    REAL *new_centers;
    REAL *temp;



    if (pct.thispe == 0)
        printf("\n Update the localization centers.. \n");

    size = ct.num_states * 3;
    my_malloc_init( new_centers, size, REAL );
    my_malloc_init( temp, ct.max_orbit_size, REAL );



    for (st = ct.state_begin; st < ct.state_end; st++)
    {
        get_orbit_center(&states[st], &x, &y, &z);
        new_centers[st * 3 + 0] = x;
        new_centers[st * 3 + 1] = y;
        new_centers[st * 3 + 2] = z;
    }

    global_sums(new_centers, &size);

    for (st = 0; st < ct.num_states; st++)
    {
        if (states[st].movable == 1)
        {
            x = new_centers[st * 3 + 0];
            y = new_centers[st * 3 + 1];
            z = new_centers[st * 3 + 2];
            shift_x = (x - states[st].crds[0]) / (ct.hxgrid * ct.xside);
            shift_y = (y - states[st].crds[1]) / (ct.hygrid * ct.yside);
            shift_z = (z - states[st].crds[2]) / (ct.hzgrid * ct.zside);

            if (pct.thispe == 0)
                printf
                    ("\n State_%d: %f, %f, %f, ---->  %f, %f, %f, Shift: %d, %d, %d  ",
                     st, states[st].crds[0], states[st].crds[1],
                     states[st].crds[2], x, y, z, shift_x, shift_y, shift_z);


            states[st].crds[0] += shift_x * (ct.hxgrid * ct.xside);
            states[st].crds[1] += shift_y * (ct.hygrid * ct.yside);
            states[st].crds[2] += shift_z * (ct.hzgrid * ct.zside);

            states[st].ixmin = states[st].ixmin + shift_x;
            states[st].iymin = states[st].iymin + shift_y;
            states[st].izmin = states[st].izmin + shift_z;
            states[st].ixmax = states[st].ixmax + shift_x;
            states[st].iymax = states[st].iymax + shift_y;
            states[st].izmax = states[st].izmax + shift_z;

            for (i = 0; i < ct.max_orbit_size; i++)
                temp[i] = 0.0;

            size_x = states[st].orbit_nx;
            size_y = states[st].orbit_ny;
            size_z = states[st].orbit_nz;

            if ((st >= ct.state_begin) && (st < ct.state_end))
            {
                for (i = 0; i < size_x; i++)
                {
                    new_i = i - shift_x;
                    if (new_i >= 0 && (new_i < size_x))
                    {
                        for (j = 0; j < size_y; j++)
                        {
                            new_j = j - shift_y;
                            if (new_j >= 0 && (new_j < size_y))
                            {
                                for (k = 0; k < size_z; k++)
                                {
                                    new_k = k - shift_z;
                                    if (new_k >= 0 && (new_k < size_z))
                                    {
                                        new_idx = new_i * size_y * size_z + new_j * size_z + new_k;
                                        old_idx = i * size_y * size_z + j * size_z + k;
                                        temp[new_idx] = states[st].psiR[old_idx];
                                        /*
                                           printf("\n idx %d, %d, %f, %f ", 
                                           new_idx, old_idx, temp[new_idx], states[st].psiR[ old_idx]);  
                                         */
                                    }
                                }
                            }
                        }
                    }
                }
                for (i = 0; i < states[st].size; i++)
                    states[st].psiR[i] = temp[i];
            }

        }
    }


    my_barrier();

    normalize_orbits(states);

    is_state_overlap(states);

    get_orbit_overlap_region(states);

    init_comm(states);

    init_comm_res(states);

    for (level = 0; level < ct.eig_parm.levels + 1; level++)
        make_mask_grid_state(level, states);

    for (st = ct.state_begin; st < ct.state_end; st++)
    {
        app_mask(st, states[st].psiR, 0);
    }

    /* Initialize Non-local operators */
    get_ion_orbit_overlap_nl(states);
    get_nlop();
    init_nonlocal_comm();

    my_free(new_centers);
    my_free(temp);

#if 	DEBUG
    print_orbit_centers(states);
#endif

}


int if_update_centers(STATE * states)
{

    int st;
    int size;
    int shift_x, shift_y, shift_z;
    int i, j, k;
    REAL x, y, z;
    REAL *new_centers;
    int N_moving_orbits;



    size = ct.num_states * 3;
    my_malloc_init( new_centers, size, REAL );

    for (st = ct.state_begin; st < ct.state_end; st++)
    {
        get_orbit_center(&states[st], &x, &y, &z);
        new_centers[st * 3 + 0] = x;
        new_centers[st * 3 + 1] = y;
        new_centers[st * 3 + 2] = z;
    }

    global_sums(new_centers, &size);

    N_moving_orbits = 0;
    for (st = ct.state_begin; st < ct.state_end; st++)
    {
        if (states[st].movable == 1)
        {
            x = new_centers[st * 3 + 0];
            y = new_centers[st * 3 + 1];
            z = new_centers[st * 3 + 2];
            shift_x = (x - states[st].crds[0]) / (ct.hxgrid * ct.xside);
            shift_y = (y - states[st].crds[1]) / (ct.hygrid * ct.yside);
            shift_z = (z - states[st].crds[2]) / (ct.hzgrid * ct.zside);

            N_moving_orbits += abs(shift_x) * abs(shift_x)
                + abs(shift_y) * abs(shift_y) + abs(shift_z) * abs(shift_z);
        }
    }
    if (N_moving_orbits < 3)
    {
        return 0;
    }
    else
    {
        return 1;
    }

}
