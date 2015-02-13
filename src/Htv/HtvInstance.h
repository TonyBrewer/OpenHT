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

class CHtvInstancePort : public CHtfeInstancePort
{
public:
	CHtvIdent * GetModulePort() { return (CHtvIdent *)CHtfeInstancePort::GetModulePort(); }

private:
};

class CHtvSignal : public CHtfeSignal
{
public:
    CHtvIdent * GetType() { return (CHtvIdent *)CHtfeSignal::GetType(); }
	CHtvInstancePort * GetRefList(size_t i) { return (CHtvInstancePort *)CHtfeSignal::GetRefList()[i]; }

private:
};

class CHtvSignalTblIter : public CHtfeSignalTblIter
{
public:
    CHtvSignalTblIter(CHtfeSignalTbl &signalTbl) : CHtfeSignalTblIter(signalTbl) {}

    CHtvSignal * operator ->() { return (CHtvSignal *)CHtfeSignalTblIter::operator->(); }
    CHtvSignal * operator () () { return (CHtvSignal *)CHtfeSignalTblIter::operator->(); }

private:

};
