########################################
# RESET
########################################
set_property LOC BUFGCTRL_X0Y127 [get_cells -hier -regexp ae3/.*cae_pers/rsttop_bufg]
set_property LOC BUFGCTRL_X0Y95  [get_cells -hier -regexp ae2/.*cae_pers/rsttop_bufg]
set_property LOC BUFGCTRL_X0Y63  [get_cells -hier -regexp ae1/.*cae_pers/rsttop_bufg]
set_property LOC BUFGCTRL_X0Y31  [get_cells -hier -regexp ae0/.*cae_pers/rsttop_bufg]

set_property LOC SLICE_X251Y549 [get_cells -hier -regexp ae3/.*cae_pers/dis/r_st_idle_reg]
set_property LOC SLICE_X251Y399 [get_cells -hier -regexp ae2/.*cae_pers/dis/r_st_idle_reg]
set_property LOC SLICE_X251Y249 [get_cells -hier -regexp ae1/.*cae_pers/dis/r_st_idle_reg]
set_property LOC SLICE_X251Y99  [get_cells -hier -regexp ae0/.*cae_pers/dis/r_st_idle_reg]
