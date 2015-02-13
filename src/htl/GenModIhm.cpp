/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "CnyHt.h"
#include "DsnInfo.h"

// Generate a module's inbound host message (ihm) interface

void CDsnInfo::InitAndValidateModIhm()
{
	// check that all host messages have valid Lsb and Width
	for (size_t modIdx = 0; modIdx < m_modList.size(); modIdx += 1) {
		CModule &mod = *m_modList[modIdx];

		if (!mod.m_bIsUsed) continue;

		for (size_t ihmIdx = 0; ihmIdx < mod.m_hostMsgList.size(); ihmIdx += 1) {
			CHostMsg &hostMsg = mod.m_hostMsgList[ihmIdx];

			if (hostMsg.m_msgDir == Outbound) continue;

			for (size_t dstIdx = 0; dstIdx < hostMsg.m_msgDstList.size(); dstIdx += 1) {
				CMsgDst & msgDst = hostMsg.m_msgDstList[dstIdx];

				// find shared variable
				size_t shIdx;
				for (shIdx = 0; shIdx < mod.m_shared.m_fieldList.size(); shIdx += 1) {
					CField & shared = mod.m_shared.m_fieldList[shIdx];

					if (shared.m_name == msgDst.m_var)
						break;
				}

				if (shIdx == mod.m_shared.m_fieldList.size()) {
					ParseMsg(Error, msgDst.m_lineInfo, "message destination must be a shared variable");
					continue;
				}
				CField & shared = mod.m_shared.m_fieldList[shIdx];

				msgDst.m_pShared = &shared;

				shared.m_bIhmReadOnly = msgDst.m_bReadOnly;

				if (msgDst.m_field.size() > 0) {
					// type of variable should be a union or struct
					bool bCStyle;
					CField const * pBaseField, * pLastField;
					FindFieldInStruct(msgDst.m_lineInfo, shared.m_type, msgDst.m_field, true, bCStyle, pBaseField, pLastField);
					msgDst.m_pField = pLastField;

					if (msgDst.m_pField == 0) {
						ParseMsg(Error, msgDst.m_lineInfo, "message field was not found");
						continue;
					}

					if (!bCStyle)
						msgDst.m_field = "m_" + msgDst.m_field;
				}

				if (msgDst.m_pShared->m_queueW.size() > 0 && msgDst.m_field.size() > 0)
					// queue can not have a field
					ParseMsg(Error, msgDst.m_lineInfo, "message destination queue can not use field");

				uint64_t mask = 0;

				// verify appropriate addresses and indexes are specified
				if ((shared.m_addr1W.size() != 0) != (msgDst.m_addr1Lsb.size() != 0)) {
					if (shared.m_addr1W.size() != 0)
						ParseMsg(Error, msgDst.m_lineInfo, "message variable requires addr1Lsb");
					else
						ParseMsg(Error, msgDst.m_lineInfo, "message variable does not require addr1Lsb");

				} else if (msgDst.m_addr1Lsb.size() > 0) {

					if (shared.m_addr1W.AsInt() + msgDst.m_addr1Lsb.AsInt() > 56)
						ParseMsg(Error, msgDst.m_lineInfo, "shared variable width (%d) plus addr1Lsb (%d) is greater than 56",
							shared.m_addr1W.AsInt(), msgDst.m_addr1Lsb.AsInt());
					else {
						uint64_t addr1Bits = ((1u << shared.m_addr1W.AsInt())-1) << msgDst.m_addr1Lsb.AsInt();
						if ((mask & addr1Bits) != 0)
							ParseMsg(Error, msgDst.m_lineInfo, "addr1Lsb overlaps with other message fields");
						mask |= addr1Bits;
					}
				}

				if ((shared.m_addr2W.size() != 0) != (msgDst.m_addr2Lsb.size() != 0)) {
					if (shared.m_addr2W.size() != 0)
						ParseMsg(Error, msgDst.m_lineInfo, "message variable requires addr2Lsb");
					else
						ParseMsg(Error, msgDst.m_lineInfo, "message variable does not require addr2Lsb");

				} else if (msgDst.m_addr2Lsb.size() > 0) {

					if (shared.m_addr2W.AsInt() + msgDst.m_addr2Lsb.AsInt() > 56)
						ParseMsg(Error, msgDst.m_lineInfo, "shared variable width (%d) plus addr2Lsb (%d) is greater than 56",
							shared.m_addr2W.AsInt(), msgDst.m_addr2Lsb.AsInt());
					else {
						uint64_t addr2Bits = ((1u << shared.m_addr2W.AsInt())-1) << msgDst.m_addr2Lsb.AsInt();
						if ((mask & addr2Bits) != 0)
							ParseMsg(Error, msgDst.m_lineInfo, "addr2Lsb overlaps with other message fields");
						mask |= addr2Bits;
					}
				}

				if ((shared.m_dimenList.size() >= 1) != (msgDst.m_idx1Lsb.size() != 0)) {
					if (shared.m_dimenList.size() != 0)
						ParseMsg(Error, msgDst.m_lineInfo, "message variable requires idx1Lsb");
					else
						ParseMsg(Error, msgDst.m_lineInfo, "message variable does not require idx1Lsb");

				} else if (msgDst.m_idx1Lsb.size() > 0) {
					
					if (FindLg2(shared.m_dimenList[0].AsInt()) + msgDst.m_idx1Lsb.AsInt() > 56)
						ParseMsg(Error, msgDst.m_lineInfo, "shared variable dimen1W (%d) plus idx1Lsb (%d) is greater than 56",
							FindLg2(shared.m_dimenList[0].AsInt()), msgDst.m_idx1Lsb.AsInt());
					else {
						uint64_t idx1Bits = ((1u << FindLg2(shared.m_dimenList[0].AsInt()))-1) << msgDst.m_idx1Lsb.AsInt();
						if ((mask & idx1Bits) != 0)
							ParseMsg(Error, msgDst.m_lineInfo, "idx1Lsb overlaps with other message fields");
						mask |= idx1Bits;
					}
				}

				if ((shared.m_dimenList.size() >= 2) != (msgDst.m_idx2Lsb.size() != 0)) {
					if (shared.m_dimenList.size() >= 2)
						ParseMsg(Error, msgDst.m_lineInfo, "message variable requires idx2Lsb");
					else
						ParseMsg(Error, msgDst.m_lineInfo, "message variable does not require idx2Lsb");

				} else if (msgDst.m_idx2Lsb.size() > 0) {
					
					if (FindLg2(shared.m_dimenList[1].AsInt()) + msgDst.m_idx2Lsb.AsInt() > 56)
						ParseMsg(Error, msgDst.m_lineInfo, "shared variable dimen2W (%d) plus idx2Lsb (%d) is greater than 56",
							FindLg2(shared.m_dimenList[1].AsInt()), msgDst.m_idx2Lsb.AsInt());
					else {
						uint64_t idx2Bits = ((1u << FindLg2(shared.m_dimenList[1].AsInt()))-1) << msgDst.m_idx2Lsb.AsInt();
						if ((mask & idx2Bits) != 0)
							ParseMsg(Error, msgDst.m_lineInfo, "idx2Lsb overlaps with other message fields");
						mask |= idx2Bits;
					}
				}

				if (shared.m_dimenList.size() > 2)
					ParseMsg(Error, msgDst.m_lineInfo, "expected message variable to have two or less dimensions");

				if (msgDst.m_field.size() == 0 && msgDst.m_fldIdx1Lsb.size() > 0)
					ParseMsg(Error, msgDst.m_lineInfo, "message variable does not require fldIdx1Lsb");

				if (msgDst.m_field.size() == 0 && msgDst.m_fldIdx2Lsb.size() > 0)
					ParseMsg(Error, msgDst.m_lineInfo, "message variable does not require fldIdx2Lsb");

				if (msgDst.m_pField) {
					if ((msgDst.m_pField->m_dimenList.size() >= 1) != (msgDst.m_idx1Lsb.size() != 0)) {
						if (msgDst.m_pField->m_dimenList.size() >= 1)
							ParseMsg(Error, msgDst.m_lineInfo, "message field requires fldIdx1Lsb");
						else
							ParseMsg(Error, msgDst.m_lineInfo, "message field does not require fldIdx1Lsb");

					} else if (msgDst.m_idx1Lsb.size() > 0) {
						
						if (FindLg2(msgDst.m_pField->m_dimenList[0].AsInt()) + msgDst.m_fldIdx1Lsb.AsInt() > 56)
							ParseMsg(Error, msgDst.m_lineInfo, "message field dimen1W (%d) plus fldIdx1Lsb (%d) is greater than 56",
								FindLg2(msgDst.m_pField->m_dimenList[0].AsInt()), msgDst.m_fldIdx1Lsb.AsInt());
						else {
							uint64_t fldIdx1Bits = ((1u << FindLg2(msgDst.m_pField->m_dimenList[0].AsInt()))-1) << msgDst.m_fldIdx1Lsb.AsInt();
							if ((mask & fldIdx1Bits) != 0)
								ParseMsg(Error, msgDst.m_lineInfo, "fldIdx1Lsb overlaps with other message fields");
							mask |= fldIdx1Bits;
						}
					}

					if ((msgDst.m_pField->m_dimenList.size() >= 2) != (msgDst.m_idx2Lsb.size() != 0)) {
						if (msgDst.m_pField->m_dimenList.size() >= 2)
							ParseMsg(Error, msgDst.m_lineInfo, "message field requires fldIdx2Lsb");
						else
							ParseMsg(Error, msgDst.m_lineInfo, "message field does not require fldIdx2Lsb");

					} else if (msgDst.m_idx2Lsb.size() > 0) {
						
						if (FindLg2(msgDst.m_pField->m_dimenList[1].AsInt()) + msgDst.m_fldIdx2Lsb.AsInt() > 56)
							ParseMsg(Error, msgDst.m_lineInfo, "message field dimen2W (%d) plus fldIdx2Lsb (%d) is greater than 56",
								FindLg2(msgDst.m_pField->m_dimenList[1].AsInt()), msgDst.m_fldIdx2Lsb.AsInt());
						else {
							uint64_t fldIdx2Bits = ((1u << FindLg2(msgDst.m_pField->m_dimenList[1].AsInt()))-1) << msgDst.m_fldIdx2Lsb.AsInt();
							if ((mask & fldIdx2Bits) != 0)
								ParseMsg(Error, msgDst.m_lineInfo, "fldIdx2Lsb overlaps with other message fields");
							mask |= fldIdx2Bits;
						}
					}

					if (msgDst.m_pField->m_dimenList.size() > 2)
						ParseMsg(Error, msgDst.m_lineInfo, "expected message field to have two or less dimensions");
				}
			}
		}
	}
}

void CDsnInfo::GenModIhmStatements(CModule &mod)
{
	if (!mod.m_bInHostMsg)
		return;

	CHtCode	& ihmPostInstr = mod.m_clkRate == eClk1x ? m_ihmPostInstr1x : m_ihmPostInstr2x;

	// control message interface
	m_ihmIoDecl.Append("\t// Inbound host message interface\n");
	m_ihmIoDecl.Append("\tsc_in<CHostCtrlMsgIntf> i_hifTo%s_ihm;\n", m_unitName.Uc().c_str());
	m_ihmIoDecl.Append("\n");

	if (mod.m_clkRate == eClk2x) {
		m_ihmRegDecl.Append("\tsc_signal<CHostCtrlMsgIntf> r_hifTo%s_ihm_1x;\n", m_unitName.Uc().c_str());
		m_ihmRegDecl.NewLine();
		m_ihmReg1x.Append("\tr_hifTo%s_ihm_1x = i_hifTo%s_ihm;\n", m_unitName.Uc().c_str(), m_unitName.Uc().c_str());
		m_ihmReg1x.NewLine();
	}

	for (size_t msgIdx = 0; msgIdx < mod.m_hostMsgList.size(); msgIdx += 1) {
		CHostMsg & hostMsg = mod.m_hostMsgList[msgIdx];
		if (hostMsg.m_msgDir == Outbound) continue;

		for (size_t dstIdx = 0; dstIdx < hostMsg.m_msgDstList.size(); dstIdx += 1) {
			CMsgDst & msgDst = hostMsg.m_msgDstList[dstIdx];

			if (msgDst.m_addr1Lsb.size() == 0) continue;

			ihmPostInstr.Append("\t{\n");

			string indexes;
			if (msgDst.m_idx1Lsb.size() > 0) {
				ihmPostInstr.Append("\t\tht_uint%d idx1 = (i_hifTo%s_ihm.read().m_msgData >> (%s)) & ((1ul << (%d))-1);\n",
					FindLg2(msgDst.m_pShared->m_dimenList[0].AsInt()-1), m_unitName.Uc().c_str(),
					msgDst.m_idx1Lsb.c_str(), FindLg2(msgDst.m_pShared->m_dimenList[0].AsInt()-1));
				indexes += "[idx1]";
			}
			if (msgDst.m_idx2Lsb.size() > 0) {
				ihmPostInstr.Append("\t\tht_uint%d idx2 = (i_hifTo%s_ihm.read().m_msgData >> (%s)) & ((1ul << (%d))-1);\n",
					FindLg2(msgDst.m_pShared->m_dimenList[1].AsInt()-1), m_unitName.Uc().c_str(),
					msgDst.m_idx2Lsb.c_str(), FindLg2(msgDst.m_pShared->m_dimenList[1].AsInt()-1));
				indexes += "[idx2]";
			}
			string addrs;
			if (msgDst.m_addr1Lsb.size() > 0) {
				ihmPostInstr.Append("\t\tht_uint%d addr1 = (i_hifTo%s_ihm.read().m_msgData >> (%s)) & ((1ul << (%d))-1);\n",
					msgDst.m_pShared->m_addr1W.AsInt(), m_unitName.Uc().c_str(),
					msgDst.m_addr1Lsb.c_str(), msgDst.m_pShared->m_addr1W.AsInt());
				addrs += "addr1";
			}
			if (msgDst.m_addr2Lsb.size() > 0) {
				ihmPostInstr.Append("\t\tht_uint%d addr2 = (i_hifTo%s_ihm.read().m_msgData >> (%s)) & ((1ul << (%d))-1);\n",
					msgDst.m_pShared->m_addr2W.AsInt(), m_unitName.Uc().c_str(),
					msgDst.m_addr2Lsb.c_str(), msgDst.m_pShared->m_addr2W.AsInt());
				addrs += "addr2";
			}

			ihmPostInstr.Append("\t\tm_%s%s.write_addr(%s);\n",
				msgDst.m_var.c_str(), indexes.c_str(), addrs.c_str());

			ihmPostInstr.Append("\t}\n");
			ihmPostInstr.Append("\n");
		}
	}

	char ihmSignal[64];
	if (mod.m_clkRate == eClk1x)
		sprintf(ihmSignal, "i_hifTo%s_ihm", m_unitName.Uc().c_str());
	else
		sprintf(ihmSignal, "r_hifTo%s_ihm_1x", m_unitName.Uc().c_str());

	ihmPostInstr.Append("\tif (%s.read().m_bValid%s) {\n",
		ihmSignal, mod.m_clkRate == eClk1x ? "" : " && r_phase");
	ihmPostInstr.Append("\t\tswitch (%s.read().m_msgType) {\n", ihmSignal);

	for (size_t msgIdx = 0; msgIdx < mod.m_hostMsgList.size(); msgIdx += 1) {
		CHostMsg & hostMsg = mod.m_hostMsgList[msgIdx];
		if (hostMsg.m_msgDir == Outbound) continue;

		ihmPostInstr.Append("\t\t\tcase %s:\n", hostMsg.m_msgName.c_str());

		for (size_t dstIdx = 0; dstIdx < hostMsg.m_msgDstList.size(); dstIdx += 1) {
			CMsgDst & msgDst = hostMsg.m_msgDstList[dstIdx];

			string indexes;
			string fldIndexes;
			string extraTab;
			if (msgDst.m_idx1Lsb.size() > 0 || msgDst.m_fldIdx1Lsb.size() > 0) {
				ihmPostInstr.Append("\t\t\t\t{\n");

				if (msgDst.m_idx1Lsb.size() > 0) {
					ihmPostInstr.Append("\t\t\t\t\tht_uint%d idx1 = (%s.read().m_msgData >> (%s)) & ((1ul << (%d))-1);\n",
						FindLg2(msgDst.m_pShared->m_dimenList[0].AsInt()-1), ihmSignal,
						msgDst.m_idx1Lsb.c_str(), FindLg2(msgDst.m_pShared->m_dimenList[0].AsInt()-1));
					indexes += "[idx1]";
				}
				if (msgDst.m_idx2Lsb.size() > 0) {
					ihmPostInstr.Append("\t\t\t\t\tht_uint%d idx2 = (%s.read().m_msgData >> (%s)) & ((1ul << (%d))-1);\n",
						FindLg2(msgDst.m_pShared->m_dimenList[1].AsInt()-1), ihmSignal,
						msgDst.m_idx2Lsb.c_str(), FindLg2(msgDst.m_pShared->m_dimenList[1].AsInt()-1));
					indexes += "[idx2]";
				}

				if (msgDst.m_fldIdx1Lsb.size() > 0) {
					ihmPostInstr.Append("\t\t\t\t\tht_uint%d fldIdx1 = (%s.read().m_msgData >> (%s)) & ((1ul << (%d))-1);\n",
						FindLg2(msgDst.m_pField->m_dimenList[0].AsInt()-1), ihmSignal,
						msgDst.m_fldIdx1Lsb.c_str(), FindLg2(msgDst.m_pField->m_dimenList[0].AsInt()-1));
					indexes += "[fldIdx1]";
				}
				if (msgDst.m_fldIdx2Lsb.size() > 0) {
					ihmPostInstr.Append("\t\t\t\t\tht_uint%d fldIdx2 = (%s.read().m_msgData >> (%s)) & ((1ul << (%d))-1);\n",
						FindLg2(msgDst.m_pField->m_dimenList[1].AsInt()-1), ihmSignal,
						msgDst.m_fldIdx2Lsb.c_str(), FindLg2(msgDst.m_pField->m_dimenList[1].AsInt()-1));
					indexes += "[fldIdx2]";
				}
				extraTab = "\t";
			}


			if (msgDst.m_pShared->m_queueW.size() > 0) {
				ihmPostInstr.Append("%s\t\t\t\tm_%s%s.push( (%s)(%s.read().m_msgData >> %s) );\n", extraTab.c_str(),
					msgDst.m_var.c_str(), indexes.c_str(), msgDst.m_pShared->m_type.c_str(), ihmSignal, msgDst.m_dataLsb.c_str());
			} else if (msgDst.m_addr1Lsb.size() > 0) {
				ihmPostInstr.Append("%s\t\t\t\tm_%s%s.write_mem((%s)(%s.read().m_msgData >> %s));\n", extraTab.c_str(),
					msgDst.m_var.c_str(), indexes.c_str(), msgDst.m_pShared->m_type.c_str(), ihmSignal, msgDst.m_dataLsb.c_str());
			} else if (msgDst.m_field.size() > 0) {
				string castStr;
				if (msgDst.m_pField->m_bitWidth.size() == 0)
					castStr = msgDst.m_pField->m_type;
				else
					castStr = VA("ht_uint%d", msgDst.m_pField->m_bitWidth.AsInt());

				ihmPostInstr.Append("%s\t\t\t\tc_%s%s.%s%s = (%s)(%s.read().m_msgData >> %s);\n", extraTab.c_str(),
					msgDst.m_var.c_str(), indexes.c_str(), msgDst.m_field.c_str(), fldIndexes.c_str(),
						castStr.c_str(), ihmSignal, msgDst.m_dataLsb.c_str());
			} else
				ihmPostInstr.Append("%s\t\t\t\tc_%s%s = (%s)(%s.read().m_msgData >> %s);\n", extraTab.c_str(),
					msgDst.m_var.c_str(), indexes.c_str(), msgDst.m_pShared->m_type.c_str(), ihmSignal, msgDst.m_dataLsb.c_str());

			if (msgDst.m_idx1Lsb.size() > 0)
				ihmPostInstr.Append("\t\t\t\t}\n");
		}

		ihmPostInstr.Append("\t\t\t\tbreak;\n");
	}

	ihmPostInstr.Append("\t\t\tdefault:\n");
	ihmPostInstr.Append("\t\t\t\tbreak;\n");
	ihmPostInstr.Append("\t\t}\n");
	ihmPostInstr.Append("\t}\n");
	ihmPostInstr.Append("\n");
}
