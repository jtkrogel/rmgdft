if(USE_LIBXC)
add_library (XC_useLIBXC
xclda_libxc.c
xclsda_spin_libxc.c
xcgga_libxc.c
xcgga_spin_libxc.c
get_vxc.c
get_vxc_exc.c
pbe.f
bpgcxc.f
gga91_sr.f
corlyp.c
get_mgga_vxc.c
get_mgga_exc_vxc.c
#get_new_tau.c
#get_te_tau.c
mgga_libxc.c
mix_tau.c
#quench_tau.c
#relax_tau.c
#scf_tau.c
)
endif(USE_LIBXC)
