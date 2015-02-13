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

class CHtvStatement;

class CHtvIdent : public CHtfeIdent
{
public:
	CHtfeIdent * HandleNewIdent() { return new CHtvIdent(); }

    CHtvIdent *GetThis() { return this; }
	CHtvStatement * GetStatementList() { return (CHtvStatement *)CHtfeIdent::GetStatementList(); }
    CHtvIdent * GetParamIdent(int paramId) { return (CHtvIdent *)CHtfeIdent::GetParamIdent(paramId); }
    CHtvIdent * GetParamType(int paramId) { return (CHtvIdent *)CHtfeIdent::GetParamType(paramId); }
	CHtvIdent * GetPrevHier() { return (CHtvIdent *)CHtfeIdent::GetPrevHier(); }
	CHtvIdent * GetFlatIdent() { return (CHtvIdent *)CHtfeIdent::GetFlatIdent(); }
	CHtvIdent * GetClockIdent() { return (CHtvIdent *)CHtfeIdent::GetClockIdent(); }
	CHtvIdent * GetPortSignal() { return (CHtvIdent *)CHtfeIdent::GetPortSignal(); }
	CHtvIdent * GetType() { return (CHtvIdent *)CHtfeIdent::GetType(); }
	CHtvIdent * GetBaseType() { return (CHtvIdent *)CHtfeIdent::GetBaseType(); }
	CHtvIdent * FindIdent(string const & name) { return (CHtvIdent *)CHtfeIdent::FindIdent(name); }
	CHtvIdent * InsertIdent(const string &name, bool bInsertFlatTbl=true) { return (CHtvIdent *)CHtfeIdent::InsertIdent(name, bInsertFlatTbl); }
	CHtvIdent * InsertFlatIdent(CHtfeIdent * pHierIdent, const string &flatVarName) { return (CHtvIdent *)CHtfeIdent::InsertFlatIdent(pHierIdent, flatVarName); }
	CHtvIdent * GetWriterList(size_t i) { return (CHtvIdent *)CHtfeIdent::GetWriterList()[i]; }

private:

};

class CHtvIdentTblIter : public CHtfeIdentTblIter
{
public:
	CHtvIdentTblIter() : CHtfeIdentTblIter() {}
	CHtvIdentTblIter(CHtfeIdentTbl &identTbl) : CHtfeIdentTblIter(identTbl) {}
    CHtvIdent* operator ->() { return (CHtvIdent *)CHtfeIdentTblIter::operator->(); }
    CHtvIdent* operator () () { return (CHtvIdent *)CHtfeIdentTblIter::operator->(); }
private:
};

class CHtvArrayTblIter : public CScArrayTblIter
{
public:
	CHtvArrayTblIter(CHtfeIdentTbl &identTbl) : CScArrayTblIter(identTbl) {}
    CHtvIdent* operator ->() { return (CHtvIdent *)CScArrayTblIter::operator->(); }
    CHtvIdent* operator () () { return (CHtvIdent *)CScArrayTblIter::operator->(); }

private:
};
