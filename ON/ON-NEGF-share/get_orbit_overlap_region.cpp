/************************** SVN Revision Information **************************
 **    $Id$    **
******************************************************************************/
 
/*
for each orbit, its corner grid is (ixmin, iymin, izmin) , 
                                 (ixmax, iymax, izmax)
Here we determine their overlap regions.
*/

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "main.h"
#include "prototypes_on.h"
#include "init_var.h"


void get_orbit_overlap_region(STATE * states)
{
    int i1, i2, i3, i4, i5, i6;
    int size;
    int index;
    int st1, st2;
    int num_state_gridpe;


    num_state_gridpe = ct.state_end - ct.state_begin;
    size = ct.num_states * num_state_gridpe;
    my_malloc( orbit_overlap_region, size, ORBIT_ORBIT_OVERLAP );

    for (st1 = ct.state_begin; st1 < ct.state_end; st1++)
        for (st2 = 0; st2 < ct.num_states; st2++)
        {
            index = (st1 - ct.state_begin) * ct.num_states + st2;
            i1 = states[st1].ixmin;
            i2 = states[st1].ixmax;
            i3 = states[st2].ixmin;
            i4 = states[st2].ixmax;
            i5 = rmg_max(i1, i3);
            i6 = rmg_min(i2, i4);

            orbit_overlap_region[index].xlow1 = i5;
            orbit_overlap_region[index].xhigh1 = i6;

            i3 = states[st2].ixmin + get_NX_GRID();
            i4 = states[st2].ixmax + get_NX_GRID();
            i5 = rmg_max(i1, i3);
            i6 = rmg_min(i2, i4);

            if (i6 >= i5)
            {
                orbit_overlap_region[index].xlow2 = i5;
                orbit_overlap_region[index].xhigh2 = i6;
                orbit_overlap_region[index].xshift = get_NX_GRID();
            }
            else
            {
                i3 = states[st2].ixmin - get_NX_GRID();
                i4 = states[st2].ixmax - get_NX_GRID();
                i5 = rmg_max(i1, i3);
                i6 = rmg_min(i2, i4);
                orbit_overlap_region[index].xlow2 = i5;
                orbit_overlap_region[index].xhigh2 = i6;
                orbit_overlap_region[index].xshift = -get_NX_GRID();
            }

            i1 = states[st1].iymin;
            i2 = states[st1].iymax;
            i3 = states[st2].iymin;
            i4 = states[st2].iymax;
            i5 = rmg_max(i1, i3);
            i6 = rmg_min(i2, i4);

            orbit_overlap_region[index].ylow1 = i5;
            orbit_overlap_region[index].yhigh1 = i6;

            i3 = states[st2].iymin + get_NY_GRID();
            i4 = states[st2].iymax + get_NY_GRID();
            i5 = rmg_max(i1, i3);
            i6 = rmg_min(i2, i4);

            if (i6 >= i5)
            {
                orbit_overlap_region[index].ylow2 = i5;
                orbit_overlap_region[index].yhigh2 = i6;
                orbit_overlap_region[index].yshift = get_NY_GRID();
            }
            else
            {
                i3 = states[st2].iymin - get_NY_GRID();
                i4 = states[st2].iymax - get_NY_GRID();
                i5 = rmg_max(i1, i3);
                i6 = rmg_min(i2, i4);
                orbit_overlap_region[index].ylow2 = i5;
                orbit_overlap_region[index].yhigh2 = i6;
                orbit_overlap_region[index].yshift = -get_NY_GRID();
            }


            i1 = states[st1].izmin;
            i2 = states[st1].izmax;
            i3 = states[st2].izmin;
            i4 = states[st2].izmax;
            i5 = rmg_max(i1, i3);
            i6 = rmg_min(i2, i4);

            orbit_overlap_region[index].zlow1 = i5;
            orbit_overlap_region[index].zhigh1 = i6;

            i3 = states[st2].izmin + get_NZ_GRID();
            i4 = states[st2].izmax + get_NZ_GRID();
            i5 = rmg_max(i1, i3);
            i6 = rmg_min(i2, i4);

            if (i6 >= i5)
            {
                orbit_overlap_region[index].zlow2 = i5;
                orbit_overlap_region[index].zhigh2 = i6;
                orbit_overlap_region[index].zshift = get_NZ_GRID();
            }
            else
            {
                i3 = states[st2].izmin - get_NZ_GRID();
                i4 = states[st2].izmax - get_NZ_GRID();
                i5 = rmg_max(i1, i3);
                i6 = rmg_min(i2, i4);

                orbit_overlap_region[index].zlow2 = i5;
                orbit_overlap_region[index].zhigh2 = i6;
                orbit_overlap_region[index].zshift = -get_NZ_GRID();
            }
        }
}
