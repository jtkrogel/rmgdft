#ifndef RMG_ION_H
#define RMG_ION_H 1



/* Ion structure */
class ION
{

public:

    /* Initial physical coordinates at start of run */
    double icrds[3];

    /* Actual Physical coordinates at current time step */
    double crds[3];

    /* Positions at the previous time step */
    double ocrds1[3];

    /* Positions at  2dt back */
    double ocrds2[3];

    /* Positions at  3dt back */
    double ocrds3[3];

    /* Initial crystal coordinates at start of run */
    double ixtal[3];

    /* Actual crystal coordinates at current time step */
    double xtal[3];

    /* Crystal coordinates  at the previous time step */
    double oxtal[3];

    /*Position of ion relative to the middle of non-local box around the ion 
     *          * determined in get_nlop, AIget_cindex sets this up*/
    double nlcrds[3];


    /* Coordinates of the corner of the grid that the local */
    /* difference potential is nonzero on.                  */
    double lxcstart;
    double lycstart;
    double lzcstart;


    /* Coordinates of the corner of the grid that the non-local */
    /* potential is nonzero on.                                 */
    double nlxcstart;
    double nlycstart;
    double nlzcstart;
 
    int nl_global_grid_xstart;
    int nl_global_grid_ystart;
    int nl_global_grid_zstart;


    /* Coordinates of the corner of the grid that the Qfunction */
    /* potential is nonzero on.                                 */
    double Qxcstart;
    double Qycstart;
    double Qzcstart;


    /* Integer species type when using a raw pseudopotential */
    int species;

    /* Forces on the ion */
    double force[4][3];

    /* Current velocity of the ion */
    double velocity[3];

    /* Milliken normalization coefficients */
    double mnorm[(MAX_L + 1) * (MAX_L + 1)];

    /* Total number of projectors */
    int prjcount;

    /* Movable flag */
    int movable;

    /* Force modifier parameters */
    struct {
        double setA_weight;
        double setA_coord[3];
        double setB_weight;
        double setB_coord[3];
        double forcemask[3];
    } constraint;

    /*  number of local orbitals on the ion */
    int n_loc_states;


    int ixstart;
    int iystart;
    int izstart;
    int ixend;
    int iyend;
    int izend;

    int frozen;
 
       /* Localization mask */
    char *lmask[4];

    

    int first_state_index;

    /*Stores PDB information*/
    PDB_INFO pdb;



    int ixstart_loc;
    int iystart_loc;
    int izstart_loc;
    int ixend_loc;
    int iyend_loc;
    int izend_loc;

    double xcstart_loc;
    double ycstart_loc;
    double zcstart_loc;

    int orbital_start_index;
    int num_orbitals;

    double partial_charge;
    double init_spin_rho;


};


#endif
