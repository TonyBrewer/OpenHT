/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

class CHttStatement;

class CHttIdent : public CHtfeIdent
{
public:
	CHtfeIdent * HandleNewIdent() { return new CHttIdent(); }

    CHttIdent *GetThis() { return this; }
	CHttStatement * GetStatementList() { return (CHttStatement *)CHtfeIdent::GetStatementList(); }
    CHttIdent * GetParamIdent(int paramId) { return (CHttIdent *)CHtfeIdent::GetParamIdent(paramId); }
    CHttIdent * GetParamType(int paramId) { return (CHttIdent *)CHtfeIdent::GetParamType(paramId); }
	CHttIdent * GetPrevHier() { return (CHttIdent *)CHtfeIdent::GetPrevHier(); }
	CHttIdent * GetFlatIdent() { return (CHttIdent *)CHtfeIdent::GetFlatIdent(); }
	CHttIdent * GetClockIdent() { return (CHttIdent *)CHtfeIdent::GetClockIdent(); }
	CHttIdent * GetPortSignal() { return (CHttIdent *)CHtfeIdent::GetPortSignal(); }
	CHttIdent * GetType() { return (CHttIdent *)CHtfeIdent::GetType(); }
	CHttIdent * GetBaseType() { return (CHttIdent *)CHtfeIdent::GetBaseType(); }
	CHttIdent * FindIdent(string const & name) { return (CHttIdent *)CHtfeIdent::FindIdent(name); }
	CHttIdent * InsertIdent(const string &name, bool bInsertFlatTbl=true) { return (CHttIdent *)CHtfeIdent::InsertIdent(name, bInsertFlatTbl); }
	CHttIdent * InsertFlatIdent(CHtfeIdent * pHierIdent, const string &flatVarName) { return (CHttIdent *)CHtfeIdent::InsertFlatIdent(pHierIdent, flatVarName); }
	CHttIdent * GetWriterList(size_t i) { return (CHttIdent *)CHtfeIdent::GetWriterList()[i]; }

private:

};

class CHttIdentTblIter : public CHtfeIdentTblIter
{
public:
	CHttIdentTblIter() : CHtfeIdentTblIter() {}
	CHttIdentTblIter(CHtfeIdentTbl &identTbl) : CHtfeIdentTblIter(identTbl) {}
    CHttIdent* operator ->() { return (CHttIdent *)CHtfeIdentTblIter::operator->(); }
    CHttIdent* operator () () { return (CHttIdent *)CHtfeIdentTblIter::operator->(); }
private:
};

class CHttArrayTblIter : public CScArrayTblIter
{
public:
	CHttArrayTblIter(CHtfeIdentTbl &identTbl) : CScArrayTblIter(identTbl) {}
    CHttIdent* operator ->() { return (CHttIdent *)CScArrayTblIter::operator->(); }
    CHttIdent* operator () () { return (CHttIdent *)CScArrayTblIter::operator->(); }

private:
};
