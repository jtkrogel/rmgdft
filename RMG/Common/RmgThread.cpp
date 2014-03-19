#include "BaseGrid.h"
#include "BaseThread.h"
#include "RmgThread.h"
#include "rmg_error.h"
#include "rmgtypedefs.h"
#include "typedefs.h"
#include "rmgthreads.h"
#include "const.h"
#include "prototypes.h"
#if GPU_ENABLED
#include <cuda.h>
#include <cuda_runtime.h>
#endif
using namespace std;


// Main thread function specific to subprojects
void *run_threads(void *v) {

    int retval;
    BaseThread *s;
    SCF_THREAD_CONTROL *ss;
    s = (BaseThread *)v;
    BaseGrid G;    

#if GPU_ENABLED
    cudaError_t cuerr;
#endif

    s->set_cpu_affinity(s->tid);

    // Set up thread local storage
    rmg_set_tsd(s);


    // Get the control structure
    ss = (SCF_THREAD_CONTROL *)s->pptr;

#if GPU_ENABLED
    cudaSetDevice(ct.cu_dev); 
    if(cudaSuccess != (cuerr = cudaStreamCreate((cudaStream_t *)&s->cstream))) {
        fprintf (stderr, "Error: cudaStreamCreate failed for: threads setup\n");
        exit(-1);
    }
#endif

    while(1) {

        // We sleep until main wakes us up
        s->thread_sleep();

        // Get the control structure
        ss = (SCF_THREAD_CONTROL *)s->pptr;

        // Switch that controls what we do
        switch(ss->job) {
            case HYBRID_EIG:       // Performs a single multigrid sweep over an orbital
               mg_eig_state_driver(ss->sp, 0, ss->vtot, ct.mg_eig_precision);
               break;
            case HYBRID_SKIP:
               break;
#if GAMMA_PT
            case HYBRID_SUBDIAG_APP_AB:
               subdiag_app_AB_one(ss->sp, (rmg_double_t *)ss->p1, (rmg_double_t *)ss->p2, ss->vtot);
               break;
            case HYBRID_SUBDIAG_APP_A:
               subdiag_app_A_one(ss->sp, (rmg_double_t *)ss->p1, (rmg_double_t *)ss->p2, ss->vtot);
               break;
            case HYBRID_SUBDIAG_APP_B:
               subdiag_app_B_one(ss->sp, (rmg_double_t *)ss->p1);
               break;
#endif 
            case HYBRID_BETAX_PSI1_CALCULATE:
               betaxpsi1_calculate_one(ss->sp, ss->ion, ss->nion, ss->sintR, ss->sintI, ss->kpt, ss->weiptr);
               break;
            default:
               break;
        }

    }

}


