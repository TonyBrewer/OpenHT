
#########################################
# Personality specific placement commands
#########################################
#create_pblock AG_dec
#resize_pblock AG_dec -add {RAMB18_X0Y0:RAMB18_X12Y47 RAMB36_X0Y0:RAMB36_X12Y23 DSP48_X0Y0:DSP48_X16Y47 SLICE_X0Y0:SLICE_X191Y119}
create_pblock AG_dec0
resize_pblock AG_dec0 -add {RAMB18_X0Y0:RAMB18_X5Y47 RAMB36_X0Y0:RAMB36_X5Y23 DSP48_X0Y0:DSP48_X7Y47 SLICE_X0Y0:SLICE_X95Y119}
create_pblock AG_dec1
resize_pblock AG_dec1 -add {RAMB18_X6Y0:RAMB18_X12Y47 RAMB36_X6Y0:RAMB36_X12Y23 DSP48_X8Y0:DSP48_X16Y47 SLICE_X96Y0:SLICE_X191Y119}
add_cells_to_pblock [get_pblocks AG_dec0] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BMEM.*.* && (NAME =~  "*PersDec$0*" || NAME =~  "*PersIhuf2$0*" || NAME =~  "*PersIdct$0*") } ]
add_cells_to_pblock [get_pblocks AG_dec0] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ FLOP_LATCH.*.* && (NAME =~  "*PersDec$0*" || NAME =~  "*PersIhuf2$0*" || NAME =~  "*PersIdct$0*") } ]
add_cells_to_pblock [get_pblocks AG_dec0] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ MULT.dsp.* && (NAME =~  "*PersDec$0*" || NAME =~  "*PersIhuf2$0*" || NAME =~  "*PersIdct$0*") } ]
add_cells_to_pblock [get_pblocks AG_dec0] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BMEM.*.* && (NAME =~  "*PersDec$1*" || NAME =~  "*PersIhuf2$1*" || NAME =~  "*PersIdct$1*") } ]
add_cells_to_pblock [get_pblocks AG_dec0] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ FLOP_LATCH.*.* && (NAME =~  "*PersDec$1*" || NAME =~  "*PersIhuf2$1*" || NAME =~  "*PersIdct$1*") } ]
add_cells_to_pblock [get_pblocks AG_dec0] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ MULT.dsp.* && (NAME =~  "*PersDec$1*" || NAME =~  "*PersIhuf2$1*" || NAME =~  "*PersIdct$1*") } ]
add_cells_to_pblock [get_pblocks AG_dec1] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BMEM.*.* && (NAME =~  "*PersDec$2*" || NAME =~  "*PersIhuf2$2*" || NAME =~  "*PersIdct$2*") } ]
add_cells_to_pblock [get_pblocks AG_dec1] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ FLOP_LATCH.*.* && (NAME =~  "*PersDec$2*" || NAME =~  "*PersIhuf2$2*" || NAME =~  "*PersIdct$2*") } ]
add_cells_to_pblock [get_pblocks AG_dec1] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ MULT.dsp.* && (NAME =~  "*PersDec$2*" || NAME =~  "*PersIhuf2$2*" || NAME =~  "*PersIdct$2*") } ]
add_cells_to_pblock [get_pblocks AG_dec1] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BMEM.*.* && (NAME =~  "*PersDec$3*" || NAME =~  "*PersIhuf2$3*" || NAME =~  "*PersIdct$3*") } ]
add_cells_to_pblock [get_pblocks AG_dec1] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ FLOP_LATCH.*.* && (NAME =~  "*PersDec$3*" || NAME =~  "*PersIhuf2$3*" || NAME =~  "*PersIdct$3*") } ]
add_cells_to_pblock [get_pblocks AG_dec1] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ MULT.dsp.* && (NAME =~  "*PersDec$3*" || NAME =~  "*PersIhuf2$3*" || NAME =~  "*PersIdct$3*") } ]

#create_pblock AG_hor
#resize_pblock AG_hor -add {RAMB18_X0Y48:RAMB18_X12Y99 RAMB36_X0Y24:RAMB36_X12Y49 DSP48_X0Y50:DSP48_X16Y99 SLICE_X0Y120:SLICE_X191Y249}
create_pblock AG_hor0
resize_pblock AG_hor0 -add {RAMB18_X0Y48:RAMB18_X5Y73 RAMB36_X0Y24:RAMB36_X5Y36 DSP48_X0Y50:DSP48_X8Y73 SLICE_X0Y120:SLICE_X95Y184}
create_pblock AG_hor1
resize_pblock AG_hor1 -add {RAMB18_X6Y48:RAMB18_X12Y73 RAMB36_X6Y24:RAMB36_X12Y36 DSP48_X9Y50:DSP48_X16Y73 SLICE_X96Y120:SLICE_X191Y184}
create_pblock AG_hor2
resize_pblock AG_hor2 -add {RAMB18_X0Y74:RAMB18_X5Y799 RAMB36_X0Y37:RAMB36_X5Y49 DSP48_X0Y74:DSP48_X8Y99 SLICE_X0Y185:SLICE_X95Y249}
create_pblock AG_hor3
resize_pblock AG_hor3 -add {RAMB18_X6Y74:RAMB18_X12Y99 RAMB36_X6Y37:RAMB36_X12Y49 DSP48_X9Y74:DSP48_X16Y99 SLICE_X96Y185:SLICE_X191Y249}
add_cells_to_pblock [get_pblocks AG_hor0] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BMEM.*.* && (NAME =~  "*PersHwrk$0*") } ]
add_cells_to_pblock [get_pblocks AG_hor0] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ MULT.dsp.* && (NAME =~  "*PersHwrk$0*") } ]
add_cells_to_pblock [get_pblocks AG_hor0] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ FLOP_LATCH.*.* && (NAME =~  "*PersHwrk$0*") } ]
add_cells_to_pblock [get_pblocks AG_hor1] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BMEM.*.* && (NAME =~  "*PersHwrk$1*") } ]
add_cells_to_pblock [get_pblocks AG_hor1] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ MULT.dsp.* && (NAME =~  "*PersHwrk$1*") } ]
add_cells_to_pblock [get_pblocks AG_hor1] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ FLOP_LATCH.*.* && (NAME =~  "*PersHwrk$1*") } ]
add_cells_to_pblock [get_pblocks AG_hor2] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BMEM.*.* && (NAME =~  "*PersHwrk$2*") } ]
add_cells_to_pblock [get_pblocks AG_hor2] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ MULT.dsp.* && (NAME =~  "*PersHwrk$2*") } ]
add_cells_to_pblock [get_pblocks AG_hor2] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ FLOP_LATCH.*.* && (NAME =~  "*PersHwrk$2*") } ]
add_cells_to_pblock [get_pblocks AG_hor3] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BMEM.*.* && (NAME =~  "*PersHwrk$3*") } ]
add_cells_to_pblock [get_pblocks AG_hor3] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ MULT.dsp.* && (NAME =~  "*PersHwrk$3*") } ]
add_cells_to_pblock [get_pblocks AG_hor3] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ FLOP_LATCH.*.* && (NAME =~  "*PersHwrk$3*") } ]

create_pblock AG_ver
resize_pblock AG_ver -add {DSP48_X0Y100:DSP48_X16Y145 RAMB18_X0Y100:RAMB18_X12Y145 RAMB36_X0Y50:RAMB36_X12Y72 SLICE_X0Y250:SLICE_X191Y364}
create_pblock AG_ver0
resize_pblock AG_ver0 -add {DSP48_X0Y100:DSP48_X7Y145 RAMB18_X0Y100:RAMB18_X5Y145 RAMB36_X0Y50:RAMB36_X5Y72 SLICE_X0Y250:SLICE_X95Y364}
create_pblock AG_ver1
resize_pblock AG_ver1 -add {DSP48_X8Y100:DSP48_X16Y145 RAMB18_X6Y100:RAMB18_X12Y145 RAMB36_X6Y50:RAMB36_X12Y72 SLICE_X96Y250:SLICE_X191Y364}
add_cells_to_pblock [get_pblocks AG_ver] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BMEM.*.* && (NAME =~  "*PersVctl*") } ]
add_cells_to_pblock [get_pblocks AG_ver] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ FLOP_LATCH.*.* && (NAME =~  "*PersVctl*") } ]
add_cells_to_pblock [get_pblocks AG_ver0] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BMEM.*.* && (NAME =~  "*PersVwrk$0*") } ]
add_cells_to_pblock [get_pblocks AG_ver0] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ MULT.dsp.* && (NAME =~  "*PersVwrk$0*") } ]
add_cells_to_pblock [get_pblocks AG_ver0] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ FLOP_LATCH.*.* && (NAME =~ "*PersVwrk$0*") } ]
add_cells_to_pblock [get_pblocks AG_ver1] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BMEM.*.* && (NAME =~  "*PersVwrk$1*") } ]
add_cells_to_pblock [get_pblocks AG_ver1] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ MULT.dsp.* && (NAME =~  "*PersVwrk$1*") } ]
add_cells_to_pblock [get_pblocks AG_ver1] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ FLOP_LATCH.*.* && (NAME =~ "*PersVwrk$1*") } ]

#create_pblock AG_enc
#resize_pblock AG_enc -add {RAMB18_X0Y146:RAMB18_X12Y169 RAMB36_X0Y73:RAMB36_X12Y84 SLICE_X0Y365:SLICE_X191Y424}
create_pblock AG_enc0
resize_pblock AG_enc0 -add {DSP48_X0Y146:DSP48_X7Y169 RAMB18_X0Y146:RAMB18_X5Y169 RAMB36_X0Y73:RAMB36_X5Y84 SLICE_X0Y365:SLICE_X95Y424}
create_pblock AG_enc1
resize_pblock AG_enc1 -add {DSP48_X8Y146:DSP48_X16Y169 RAMB18_X6Y146:RAMB18_X12Y169 RAMB36_X6Y73:RAMB36_X12Y84 SLICE_X96Y365:SLICE_X191Y424}
add_cells_to_pblock [get_pblocks AG_enc0] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BMEM.*.* && (NAME =~  "*PersEnc$0*") } ]
add_cells_to_pblock [get_pblocks AG_enc0] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ DMEM.*.* && (NAME =~  "*PersEnc$0*") } ]
add_cells_to_pblock [get_pblocks AG_enc0] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ MULT.dsp.* && (NAME =~  "*PersEnc$0*") } ]
add_cells_to_pblock [get_pblocks AG_enc0] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ FLOP_LATCH.*.* && (NAME =~  "*PersEnc$0*") } ]
add_cells_to_pblock [get_pblocks AG_enc1] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BMEM.*.* && (NAME =~  "*PersEnc$1*") } ]
add_cells_to_pblock [get_pblocks AG_enc1] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ DMEM.*.* && (NAME =~  "*PersEnc$1*") } ]
add_cells_to_pblock [get_pblocks AG_enc1] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ MULT.dsp.* && (NAME =~  "*PersEnc$1*") } ]
add_cells_to_pblock [get_pblocks AG_enc1] \
[get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ FLOP_LATCH.*.* && (NAME =~  "*PersEnc$1*") } ]

set_property LOC RAMB36_X4Y61 [get_cells ae0/cae0.cae_pers/top/pPersAuTop\$0/pPersVwrk\$0/m_pntWghtStart/mem_reg_0]
set_property LOC RAMB36_X4Y62 [get_cells ae0/cae0.cae_pers/top/pPersAuTop\$0/pPersVwrk\$0/m_pntWghtStart/mem_reg_1]
set_property LOC RAMB36_X4Y63 [get_cells ae0/cae0.cae_pers/top/pPersAuTop\$0/pPersVwrk\$0/m_pntWghtStart/mem_reg_2]
set_property LOC RAMB18_X4Y128 [get_cells ae0/cae0.cae_pers/top/pPersAuTop\$0/pPersVwrk\$0/m_pntWghtStart/mem_reg_3]
set_property LOC RAMB36_X5Y61 [get_cells ae0/cae0.cae_pers/top/pPersAuTop\$0/pPersVwrk\$0/m_pntWghtIdx/mem_reg_0]
set_property LOC RAMB36_X5Y62 [get_cells ae0/cae0.cae_pers/top/pPersAuTop\$0/pPersVwrk\$0/m_pntWghtIdx/mem_reg_1]
set_property LOC RAMB36_X5Y63 [get_cells ae0/cae0.cae_pers/top/pPersAuTop\$0/pPersVwrk\$0/m_pntWghtIdx/mem_reg_2]

set_property LOC RAMB36_X6Y61 [get_cells ae0/cae0.cae_pers/top/pPersAuTop\$0/pPersVwrk\$0/m_pntWghtStart/mem_reg_0]
set_property LOC RAMB36_X6Y62 [get_cells ae0/cae0.cae_pers/top/pPersAuTop\$0/pPersVwrk\$0/m_pntWghtStart/mem_reg_1]
set_property LOC RAMB36_X6Y63 [get_cells ae0/cae0.cae_pers/top/pPersAuTop\$0/pPersVwrk\$0/m_pntWghtStart/mem_reg_2]
set_property LOC RAMB18_X6Y128 [get_cells ae0/cae0.cae_pers/top/pPersAuTop\$0/pPersVwrk\$0/m_pntWghtStart/mem_reg_3]
set_property LOC RAMB36_X7Y61 [get_cells ae0/cae0.cae_pers/top/pPersAuTop\$0/pPersVwrk\$0/m_pntWghtIdx/mem_reg_0]
set_property LOC RAMB36_X7Y62 [get_cells ae0/cae0.cae_pers/top/pPersAuTop\$0/pPersVwrk\$0/m_pntWghtIdx/mem_reg_1]
set_property LOC RAMB36_X7Y63 [get_cells ae0/cae0.cae_pers/top/pPersAuTop\$0/pPersVwrk\$0/m_pntWghtIdx/mem_reg_2]
