/*   init  processor topology for multiple images, spin/no spin, kpoint
 *   , and grid
 *   MPI_COMM_WORLD  :   first level
 *   pct.img_comm :      second level: number of proc: pct.npes_images[pct.thisimag]
 *                               it split into the following
 *                               3dimensional cartesian group
 *                       (ct.spin_flag +1) * pct.pe_kpoint * numpeforgrid = npes of pct.img_comm
 *                 
 *   pct.spin_comm:      third level:  2 proc for spin-polarized, 1 for  nonspin
 *                           this one contains pct.npes_images[pct.thisimag]/(ct.spin_flag+1)
 *                           communicators
 *                                  this will split into two car groups
 *  pct.kpsub_comm:      third level:   num_proc is pct.pe_kpoint, this comm should be used for sum over kpoint    
 *                           this one contains pct.npes_images[pct.thisimag]/(ct.spin_flag+1)/pct.pe_kpoint communicators 
 *  pct.grid_comm:       third level:   pct.npes_images[pct.thisimag]/(ct.spinflag+1)/pct.pe_kpoint     
 *                                       must be divisible 
 *                                  this is the grid communicator, size
 *                                  should be pex * pey * pez from
 *                                  regular input file
 *     Wenchang Lu, NCSU, June, 2014
 */
    


#include "const.h"
#include "params.h"
#include "rmgtypedefs.h"
#include "typedefs.h"
#include "common_prototypes.h"
#include "common_prototypes1.h"
#include "transition.h"


void InitPe4image()
{

    int image_grp_map[MAX_IMGS], range[2][3];
    MPI_Group group, world_grp, img_masters;
    int pe1, pe2, i, j, k;
    int pemin[MAX_IMGS], pemax[MAX_IMGS];

    /* Setup MPI */
    /* get world group handle */
    MPI_Comm_group (MPI_COMM_WORLD, &world_grp);

    // assign pct.thisimg:  which image on this processor
    //  and range for thisimg.
    range[0][0] = pct.total_npes;
    range[0][1] = 0;
    range[0][2] = ct.images_per_node;

    for(i = 0; i < pct.images; i++) pemax[i] = 0;

    pe2 = 0;
    for(i = 0; i < pct.images; i+= ct.images_per_node)
    {
        pe1 = 0;
        for(j = 0; j < ct.images_per_node; j++)
        {
            pe1 += pct.image_npes[i+j];
            for(k = 0; k < pct.image_npes[i + j]; k++)
            {
                if(pct.worldrank == (pe2 + k * ct.images_per_node + j) )
                {
                     pct.thisimg = i+j;
                }
            }
        }

        for(j = 0; j < ct.images_per_node; j++)
        {
            image_grp_map[i+j] = pe2+j;
            pemin[i+j] = pe2+j;
            pemax[i+j] = pemin[i+j] + pe1;
        }

        pe2 += pe1;
    } 

    range[0][0] = pemin[pct.thisimg];
    range[0][1] = pemax[pct.thisimg]-ct.images_per_node;


    MPI_Group_range_incl (world_grp, 1, range, &group);
    MPI_Comm_create (MPI_COMM_WORLD, group, &pct.img_comm);

    MPI_Barrier(MPI_COMM_WORLD);
    /* define master group and make its comm global */
    MPI_Group_incl (world_grp, pct.images, image_grp_map, &img_masters);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Comm_create (MPI_COMM_WORLD, img_masters, &pct.rmg_comm);
    MPI_Barrier(MPI_COMM_WORLD);

    /* set gridpe rank value to its image rank */
    MPI_Comm_rank (pct.img_comm, &pct.imgpe);

    // set up neighboring image's corresponding world rank for NEB communication
    pct.left_img_rank = MPI_PROC_NULL;
    pct.right_img_rank = MPI_PROC_NULL;
    if(pct.images == 1) return;
    if(pct.thisimg == 0) // leftest image
    {
        pct.left_img_rank = pemin[pct.images-1] + pct.imgpe * ct.images_per_node;
        pct.right_img_rank = pemin[pct.thisimg + 1] + pct.imgpe * ct.images_per_node;
        
    }
    else if(pct.thisimg == pct.images -1) // rightest images
    {
        pct.left_img_rank = pemin[pct.thisimg - 1] + pct.imgpe * ct.images_per_node;
        pct.right_img_rank = pemin[0] + pct.imgpe * ct.images_per_node;
    }
    else   //images in the middle
    {
        pct.left_img_rank = pemin[pct.thisimg - 1] + pct.imgpe * ct.images_per_node;
        pct.right_img_rank = pemin[pct.thisimg + 1] + pct.imgpe * ct.images_per_node;
    }

    bool eq_pe = 1;
    for (int img = 1; img < pct.images; img++)
        eq_pe = eq_pe && (pct.image_npes[img] == pct.image_npes[img-1]);
        
    if(eq_pe && (ct.images_per_node == 1))
    {
        range[0][0] = pct.imgpe;
        range[0][1] = pct.total_npes-1;
        range[0][2] = pct.image_npes[0];
        MPI_Group_range_incl (world_grp, 1, range, &group);
        MPI_Comm_create (MPI_COMM_WORLD, group, &pct.img_cross_comm);
    //    MPI_Comm_rank(pct.img_cross_comm, &pe1);
    }


}

