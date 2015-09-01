########################################
# RESET
########################################
set_property LOC BUFGCTRL_X0Y8  [get_cells -hier -regexp ae3/.*cae_pers/rsttop_bufg]
set_property LOC BUFGCTRL_X0Y7  [get_cells -hier -regexp ae2/.*cae_pers/rsttop_bufg]
set_property LOC BUFGCTRL_X0Y6  [get_cells -hier -regexp ae1/.*cae_pers/rsttop_bufg]
set_property LOC BUFGCTRL_X0Y5  [get_cells -hier -regexp ae0/.*cae_pers/rsttop_bufg]

set_property LOC SLICE_X105Y247 [get_cells -hier -regexp ae3/.*cae_pers/dis/r_st_idle_reg]
set_property LOC SLICE_X105Y247 [get_cells -hier -regexp ae2/.*cae_pers/dis/r_st_idle_reg]
set_property LOC SLICE_X105Y247 [get_cells -hier -regexp ae1/.*cae_pers/dis/r_st_idle_reg]
set_property LOC SLICE_X105Y247 [get_cells -hier -regexp ae0/.*cae_pers/dis/r_st_idle_reg]
