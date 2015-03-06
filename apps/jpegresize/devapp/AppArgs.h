/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include <string>
#include <vector>
using namespace std;

#define MODE_INFILE 1 << 0
#define MODE_INBUFF 1 << 1
#define MODE_OUTFILE 1 << 2
#define MODE_OUTBUFF 1 << 3
#define MODE_MAGICK 1 << 4
#define MODE_KEEPASPECT 1 << 5

struct AppJob {
	string m_inputFile;
	uint8_t * m_inputBuffer;
	int m_inputBufferSize;
	string m_outputFile;
	uint8_t * m_outputBuffer;
	int m_outputBufferSize;
	int m_width;
	int m_height;
	int m_scale;
	uint32_t m_mode;
	int m_quality;
	AppJob() {
		m_width = 0;
		m_height = 0;
		m_scale = 0;
		m_inputBuffer=NULL;
		m_inputBufferSize=0;
		m_outputBuffer=NULL;
		m_outputBufferSize=0;
		m_mode=0;
		m_quality=0;
	}
};

class AppArgs {
public:
	AppArgs(int argc, char **argv);

	uint32_t GetThreads() { return m_threads; }

	int GetJobs() { return (int)m_appJobs.size(); }

	AppJob & GetAppJob(int idx) { return m_appJobs[idx]; }
	int GetWidth(int idx) { return m_appJobs[idx].m_width; }
	int GetHeight(int idx) { return m_appJobs[idx].m_height; }
	int GetScale(int idx) { return m_appJobs[idx].m_scale; }
	string GetInputFile(int idx) { return m_appJobs[idx].m_inputFile; }
	string GetOutputFile(int idx) { return m_appJobs[idx].m_outputFile; }

	int GetDuration() { return m_duration; }

	string GetServer() { return m_server; }
	int GetPort() { return m_port; }

	bool IsReadOnce() { return m_bReadOnce; }
	bool IsNoWrite() { return m_bNoWrite; }
	bool IsLoopTest() { return m_bLoopTest; }
	bool IsAspectRatio() { return m_bAspectRatio; }
	bool IsBenchmark() { return m_bBenchmark; }
	bool IsCheck() { return m_bCheck; }
	bool IsHostOnly() { return m_bHostOnly; }
	bool IsHostMem() { return m_bHostMem; }
	bool IsDaemon() { return m_bDaemon; }
	bool IsRemote() { return m_bRemote; }
	bool IsDebugInfo() { return m_bDebugInfo; }
	bool IsAddRestarts() { return m_bAddRestarts; }
	bool IsMagickMode() { return m_bMagickMode; }
	bool isClientWrite() { return m_bClientWrite; }
	bool isClientRead() { return m_bClientRead; }
	uint8_t GetPersMode() { return m_persMode; }

private:
	void Help();
	void ParseJobsFile( string jobsFile );
	void ReadAppJobPics();

private:
	uint32_t m_threads;
	int m_duration;
	bool m_bReadOnce;
	bool m_bNoWrite;
	bool m_bLoopTest;
	bool m_bAspectRatio;
	bool m_bBenchmark;
	bool m_bCheck;
	bool m_bHostOnly;
	bool m_bHostMem;
	bool m_bDaemon;
	bool m_bRemote;
	bool m_bAddRestarts;
	int m_port;
	string m_server;
	bool m_bDebugInfo;
	bool m_bMagickMode;
	uint8_t m_persMode;
	int m_quality;	
	int m_bKeepAspect;
	bool m_bClientRead;
	bool m_bClientWrite;

	vector<AppJob>	m_appJobs;
};

extern AppArgs * g_pAppArgs;
extern bool g_IsCheck;