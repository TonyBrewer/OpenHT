	.file	"PersAsm.s"
	.ctext

	.signature	pdk=0

##############################################################################

	.globl	PersDisp
	.type	PersDisp. @function
	.InputRegisters    &a8, &a9, &a10, &a11, &a12
	.OutputRegisters   &a8

PersDisp:
#################################################
# Copcall Parameter List
#  a8   [8]     // HT_AE_CNT
#  a9   [48]    // AE0 Pointer to control structure
#  a10  [48]    // AE1 Pointer to control structure
#  a11  [48]    // AE2 Pointer to control structure
#  a12  [48]    // AE3 Pointer to control structure
#################################################

	mov.ae0	%a9,  $0, %aeg
	mov.ae1	%a10, $0, %aeg
	mov.ae2	%a11, $0, %aeg
	mov.ae3	%a12, $0, %aeg

	# clear status
	mov	%a0, $1, %aeg

	# start up units
	caep00 $0

	# report status (AE0 only for now)
	mov.ae0	%aeg, $1, %a9
	mov	%a9, %a8

	rtn

PersDispEnd:
	.globl	PersDispEnd
	.type	PersDispEnd. @function

##############################################################################

	.globl	PersDispPoll
	.type	PersDispPoll. @function
	.InputRegisters    &a8, &a9, &a10, &a11, &a12
	.OutputRegisters   &a8
	.ScratchRegisters  a13

PersDispPoll:
#################################################
# Copcall Parameter List
#  a8   [8]     // HT_AE_CNT
#  a9   [48]    // AE0 Pointer to control structure
#  a10  [48]    // AE1 Pointer to control structure
#  a11  [48]    // AE2 Pointer to control structure
#  a12  [48]    // AE3 Pointer to control structure
#################################################

	mov.ae0	%a9,  $0, %aeg
	mov.ae1	%a10, $0, %aeg
	mov.ae2	%a11, $0, %aeg
	mov.ae3	%a12, $0, %aeg

	# clear status
	mov	%a0, $1, %aeg

	# start up units
	caep01 $0

	# poll until done
loop:
	mov	%a0, %a13
	mov.ae0	%aeg, $4, %a9
	add.sq	%a9, %a13, %a13
	mov.ae1	%aeg, $4, %a9
	add.sq	%a9, %a13, %a13
	mov.ae2	%aeg, $4, %a9
	add.sq	%a9, %a13, %a13
	mov.ae3	%aeg, $4, %a9
	add.sq	%a9, %a13, %a13
	cmp.sq	%a13, %a8, %ac0
	br	%ac0.lt, loop

	# report status (AE0 only for now)
	mov.ae0	%aeg, $1, %a8

	rtn

PersDispPollEnd:
	.globl	PersDispPollEnd
	.type	PersDispPollEnd. @function

##############################################################################

	.globl	PersInfo
	.type	PersInfo. @function
	.InputRegisters    &a8, &a9
	.OutputRegisters   &a8, &a9
	.ScratchRegisters  a10

PersInfo:
#################################################
# Copcall Parameter List
#  a8   // BCD Part number
#  a9   // {32'b0, 16'freq, 16'numUnits}
#################################################

	# gather info from AEG's
	mov.ae0 %aeg, $2, %a10
	st.uq  %a10, $0 (%a8)
	mov.ae0 %aeg, $3, %a10
	st.uq  %a10, $0 (%a9)

	rtn

PersInfoEnd:
	.globl	PersInfoEnd
	.type	PersInfoEnd. @function

	.cend
