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

class CHttInstancePort : public CHtfeInstancePort
{
public:
	CHttIdent * GetModulePort() { return (CHttIdent *)CHtfeInstancePort::GetModulePort(); }

private:
};

class CHttSignal : public CHtfeSignal
{
public:
    CHttIdent * GetType() { return (CHttIdent *)CHtfeSignal::GetType(); }
	CHttInstancePort * GetRefList(size_t i) { return (CHttInstancePort *)CHtfeSignal::GetRefList()[i]; }

private:
};

class CHttSignalTblIter : public CHtfeSignalTblIter
{
public:
    CHttSignalTblIter(CHtfeSignalTbl &signalTbl) : CHtfeSignalTblIter(signalTbl) {}

    CHttSignal * operator ->() { return (CHttSignal *)CHtfeSignalTblIter::operator->(); }
    CHttSignal * operator () () { return (CHttSignal *)CHtfeSignalTblIter::operator->(); }

private:

};
