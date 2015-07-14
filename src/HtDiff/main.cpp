/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <stdio.h>
#include <Windows.h>
#include <vector>
#include <string>

#include "dirent.h"

using namespace std;

// file status
//   missing - not in golden files
//   extra - in golden but not in project
//   diff - in both but different
//   match - in both and identical

enum EFileStatus { eMissing, eExtra, eDiff, eMatch, eOpenError };

struct CHtFile {
	EFileStatus		m_fileStatus;
	string			m_fileName;
	string			m_projPath;
	string			m_goldPath;
};

struct CHtProj {
	string			m_projName;
	string			m_projStatus;
	string			m_projDir;
	string			m_goldDir;
	string			m_projSystemcDir;
	string			m_projVerilogDir;
	string			m_goldSystemcDir;
	string			m_goldVerilogDir;
	vector<CHtFile>	m_systemcFiles;
	vector<CHtFile>	m_verilogFiles;
};

void ScanProject(CHtProj & htProj);
bool CreateSubDir(string path, char * pSub1, char * pSub2);
EFileStatus FileCompare(string &file1, string &file2);
void FileCopy(string &srcFile, string &dstFile);
void FileDelete(string &file);
char GetStatusChar(EFileStatus status);
void IgnoreWhiteSpace(char * pBuf);

bool bIgnoreLineOrder = false;
bool bIgnoreWhiteSpace = true;

int main(int argc, char **argv)
{
	printf("Version 1.05\n");

	string goldDir = "C:/HtGolden";

	// first get list of projects to compare, list is saved in file in C:\HtGolden\ProjList
	FILE *projListFp = fopen("C:/HtGolden/ProjList.txt", "r");

	if (projListFp == 0) {
		fprintf(stderr, "Could not open C:/HtGolden/ProjList.txt\n");
		exit(0);
	}

	vector<CHtProj> projList;

	char projDir[128];
	while (fgets(projDir, 128, projListFp) != 0) {
		CHtProj htProj;
		htProj.m_projDir = projDir;

		int len = htProj.m_projDir.size();
		if (htProj.m_projDir[len-1] == '\n' || htProj.m_projDir[len-1] == '\r')
			htProj.m_projDir.erase(len-1, len);

		htProj.m_projName = htProj.m_projDir;
		int n = htProj.m_projName.find_last_of("/\\");
		if (n == string::npos) {
			fprintf(stderr, "expected project path name in C:/HtGolden/ProjList.txt, found '%s'\n", projDir);
			continue;
		}

		htProj.m_projName.erase(0, n+1);
		htProj.m_projSystemcDir = htProj.m_projDir + "/ht/sysc";
		htProj.m_projVerilogDir = htProj.m_projDir + "/ht/verilog";

		htProj.m_goldDir = goldDir + "/" + htProj.m_projName;
		htProj.m_goldSystemcDir = htProj.m_goldDir + "/ht/sysc";
		htProj.m_goldVerilogDir = htProj.m_goldDir + "/ht/verilog";

		if (CreateSubDir(htProj.m_goldDir, "/ht", "/sysc") && CreateSubDir(htProj.m_goldDir, "/ht", "/verilog"))
			projList.push_back(htProj);
	}

	fclose(projListFp);

	// for each project, check if ht/sysc and ht/verilog files are different
	// if one or more files are missing from a project, then list missing and extra files
	//   and ask to update golden file set

	printf("Creating project and file lists ...\n");
	printf("  %d projects\n", (int)projList.size());

	for (size_t projIdx = 0; projIdx < projList.size(); projIdx += 1) {
		CHtProj & htProj = projList[projIdx];

		ScanProject(htProj);
		printf(".");
	}
	printf("\n");

	CHtProj * pCurHtProj = 0;
	bool bInit = true;

	char line[128];
	char cmd[64];
	int num;
	int args;

	for (;;) {

		if (bInit) {
			strcpy(cmd, "la");
			bInit = false;
		} else {
			printf("> ");
			gets(line);
			args = sscanf(line, "%s %d", cmd, &num);
			if (args == 0)
				continue;
		}

		if (strcmp(cmd, "q") == 0 || strcmp(cmd, "quit") == 0) {
			exit(0);

		} else if (strcmp(cmd, "la") == 0) {
			printf("List all projects\n");
			for (size_t i = 0; i < projList.size(); i += 1)
				printf("  %2d   %s  %s\n", (int)i, projList[i].m_projStatus.c_str(), projList[i].m_projName.c_str());

		} else if (strcmp(cmd, "lad") == 0) {
			printf("List projects with differences\n");
			for (size_t i = 0; i < projList.size(); i += 1) {
				if (projList[i].m_projStatus == "   ") continue;
				printf("  %2d   %s  %s\n", (int)i, projList[i].m_projStatus.c_str(), projList[i].m_projName.c_str());
			}

		} else if (strcmp(cmd, "lad") == 0) {
			for (size_t pi = 0; pi < projList.size(); pi += 1) {
				CHtProj & htProj = projList[pi];

				if (htProj.m_projStatus == "   ")
					continue;

				printf("  %2d   %s  %s\n", (int)pi, htProj.m_projStatus.c_str(), htProj.m_projName.c_str());

				for (size_t i = 0; i < htProj.m_systemcFiles.size(); i += 1) {
					if (GetStatusChar(htProj.m_systemcFiles[i].m_fileStatus) == ' ')
						continue;

					printf("  %2d   %c      %s\n", (int)i,
						GetStatusChar(htProj.m_systemcFiles[i].m_fileStatus), htProj.m_systemcFiles[i].m_fileName.c_str());
				}

				for (size_t i = 0; i < htProj.m_verilogFiles.size(); i += 1) {
					if (GetStatusChar(htProj.m_verilogFiles[i].m_fileStatus) == ' ')
						continue;

					printf("  %2d   %c      %s\n", (int)(htProj.m_systemcFiles.size() + i),
						GetStatusChar(htProj.m_verilogFiles[i].m_fileStatus), htProj.m_verilogFiles[i].m_fileName.c_str());
				}
			}

		} else if (strcmp(cmd, "lp") == 0) {
			if (args == 1) {
				printf("  project number missing\n");
				continue;
			}
			if (num >= (int)projList.size()) {
				printf("  project number out of range (0-%d)\n", (int)projList.size() - 1);
				continue;
			}

			printf("List files of project %s\n", projList[num].m_projName.c_str());
			for (size_t i = 0; i < projList[num].m_systemcFiles.size(); i += 1)
				printf("  %2d   %c %s\n", (int)i,
				GetStatusChar(projList[num].m_systemcFiles[i].m_fileStatus), projList[num].m_systemcFiles[i].m_fileName.c_str());

			for (size_t i = 0; i < projList[num].m_verilogFiles.size(); i += 1)
				printf("  %2d   %c %s\n", (int)(projList[num].m_systemcFiles.size() + i),
				GetStatusChar(projList[num].m_verilogFiles[i].m_fileStatus), projList[num].m_verilogFiles[i].m_fileName.c_str());

			pCurHtProj = &projList[num];

		} else if (strcmp(cmd, "lpd") == 0) {
			if (args == 1) {
				printf("  project number missing\n");
				continue;
			}
			if (num >= (int)projList.size()) {
				printf("  project number out of range (0-%d)\n", (int)projList.size() - 1);
				continue;
			}

			printf("List files of project %s with differences\n", projList[num].m_projName.c_str());
			for (size_t i = 0; i < projList[num].m_systemcFiles.size(); i += 1) {
				char c = GetStatusChar(projList[num].m_systemcFiles[i].m_fileStatus);
				if (c == ' ') continue;
				printf("  %2d   %c %s\n", (int)i, c, projList[num].m_systemcFiles[i].m_fileName.c_str());
			}

			for (size_t i = 0; i < projList[num].m_verilogFiles.size(); i += 1) {
				char c = GetStatusChar(projList[num].m_verilogFiles[i].m_fileStatus);
				if (c == ' ') continue;
				printf("  %2d   %c %s\n", (int)(projList[num].m_systemcFiles.size() + i), c, projList[num].m_verilogFiles[i].m_fileName.c_str());
			}

			pCurHtProj = &projList[num];

		} else if (strcmp(cmd, "lps") == 0) {
			if (args == 1) {
				printf("  project number missing\n");
				continue;
			}
			if (num >= (int)projList.size()) {
				printf("  project number out of range (0-%d)\n", (int)projList.size()-1);
				continue;
			}

			printf("List files of project %s\n", projList[num].m_projName.c_str());
			for (size_t i = 0; i < projList[num].m_systemcFiles.size(); i += 1)
				printf("  %2d   %c %s\n", (int)i,
					GetStatusChar(projList[num].m_systemcFiles[i].m_fileStatus), projList[num].m_systemcFiles[i].m_fileName.c_str());

			pCurHtProj = &projList[num];

		} else if (strcmp(cmd, "lpv") == 0) {
			if (args == 1) {
				printf("  project number missing\n");
				continue;
			}
			if (num >= (int)projList.size()) {
				printf("  project number out of range (0-%d)\n", (int)projList.size()-1);
				continue;
			}

			printf("List files of project %s\n", projList[num].m_projName.c_str());
			for (size_t i = 0; i < projList[num].m_verilogFiles.size(); i += 1)
				printf("  %2d   %c %s\n", (int)(projList[num].m_systemcFiles.size() + i),
					GetStatusChar(projList[num].m_verilogFiles[i].m_fileStatus), projList[num].m_verilogFiles[i].m_fileName.c_str());

			pCurHtProj = &projList[num];

		} else if (strcmp(cmd, "uf") == 0) {
			if (args == 1) {
				printf("  file number missing\n");
				continue;
			}
			int fileCnt = (int)(pCurHtProj->m_systemcFiles.size() + pCurHtProj->m_verilogFiles.size());
			if (num >= (int)fileCnt) {
				printf("  file number out of range (0-%d)\n", fileCnt-1);
				continue;
			}

			bool bSystemc = num < (int)pCurHtProj->m_systemcFiles.size();
			CHtFile & htFile = bSystemc ? pCurHtProj->m_systemcFiles[num] : pCurHtProj->m_verilogFiles[num - pCurHtProj->m_systemcFiles.size()];

			switch (htFile.m_fileStatus) {
			case eMissing: FileCopy(htFile.m_projPath, htFile.m_goldPath); break;
			case eExtra: FileDelete(htFile.m_goldPath); break;
			case eMatch: break;
			case eDiff: FileCopy(htFile.m_projPath, htFile.m_goldPath); break;
			case eOpenError: FileCopy(htFile.m_projPath, htFile.m_goldPath); break;
			default: break;
			}

			ScanProject(*pCurHtProj);

		} else if (strcmp(cmd, "up") == 0) {
			if (args == 1) {
				printf("  project number missing\n");
				continue;
			}
			if (num >= (int)projList.size()) {
				printf("  project number out of range (0-%d)\n", (int)projList.size()-1);
				continue;
			}

			CHtProj & htProj = projList[num];

			for (size_t fi = 0; fi < htProj.m_systemcFiles.size(); fi += 1) {
				CHtFile & htFile = htProj.m_systemcFiles[fi];

				switch (htFile.m_fileStatus) {
				case eMissing: FileCopy(htFile.m_projPath, htFile.m_goldPath); break;
				case eExtra: FileDelete(htFile.m_goldPath); break;
				case eMatch: break;
				case eDiff: FileCopy(htFile.m_projPath, htFile.m_goldPath); break;
				case eOpenError: FileCopy(htFile.m_projPath, htFile.m_goldPath); break;
				default: break;
				}
			}

			for (size_t fi = 0; fi < htProj.m_verilogFiles.size(); fi += 1) {
				CHtFile & htFile = htProj.m_verilogFiles[fi];

				switch (htFile.m_fileStatus) {
				case eMissing: FileCopy(htFile.m_projPath, htFile.m_goldPath); break;
				case eExtra: FileDelete(htFile.m_goldPath); break;
				case eMatch: break;
				case eDiff: FileCopy(htFile.m_projPath, htFile.m_goldPath); break;
				case eOpenError: FileCopy(htFile.m_projPath, htFile.m_goldPath); break;
				default: break;
				}
			}

			ScanProject(htProj);

		} else if (strcmp(cmd, "ups") == 0) {
			if (args == 1) {
				printf("  project number missing\n");
				continue;
			}
			if (num >= (int)projList.size()) {
				printf("  project number out of range (0-%d)\n", (int)projList.size()-1);
				continue;
			}

			CHtProj & htProj = projList[num];

			for (size_t fi = 0; fi < htProj.m_systemcFiles.size(); fi += 1) {
				CHtFile & htFile = htProj.m_systemcFiles[fi];

				switch (htFile.m_fileStatus) {
				case eMissing: FileCopy(htFile.m_projPath, htFile.m_goldPath); break;
				case eExtra: FileDelete(htFile.m_goldPath); break;
				case eMatch: break;
				case eDiff: FileCopy(htFile.m_projPath, htFile.m_goldPath); break;
				case eOpenError: FileCopy(htFile.m_projPath, htFile.m_goldPath); break;
				default: break;
				}
			}

			ScanProject(htProj);

		} else if (strcmp(cmd, "upv") == 0) {
			if (args == 1) {
				printf("  project number missing\n");
				continue;
			}
			if (num >= (int)projList.size()) {
				printf("  project number out of range (0-%d)\n", (int)projList.size()-1);
				continue;
			}

			CHtProj & htProj = projList[num];

			for (size_t fi = 0; fi < htProj.m_verilogFiles.size(); fi += 1) {
				CHtFile & htFile = htProj.m_verilogFiles[fi];

				switch (htFile.m_fileStatus) {
				case eMissing: FileCopy(htFile.m_projPath, htFile.m_goldPath); break;
				case eExtra: FileDelete(htFile.m_goldPath); break;
				case eMatch: break;
				case eDiff: FileCopy(htFile.m_projPath, htFile.m_goldPath); break;
				case eOpenError: FileCopy(htFile.m_projPath, htFile.m_goldPath); break;
				default: break;
				}
			}

			ScanProject(htProj);

		} else if (strcmp(cmd, "ua") == 0) {
			for (size_t pi = 0; pi < projList.size(); pi += 1) {
				CHtProj & htProj = projList[pi];

				for (size_t fi = 0; fi < htProj.m_systemcFiles.size(); fi += 1) {
					CHtFile & htFile = htProj.m_systemcFiles[fi];

					switch (htFile.m_fileStatus) {
					case eMissing: FileCopy(htFile.m_projPath, htFile.m_goldPath); break;
					case eExtra: FileDelete(htFile.m_goldPath); break;
					case eMatch: break;
					case eDiff: FileCopy(htFile.m_projPath, htFile.m_goldPath); break;
					case eOpenError: FileCopy(htFile.m_projPath, htFile.m_goldPath); break;
					default: break;
					}
				}

				for (size_t fi = 0; fi < htProj.m_verilogFiles.size(); fi += 1) {
					CHtFile & htFile = htProj.m_verilogFiles[fi];

					switch (htFile.m_fileStatus) {
					case eMissing: FileCopy(htFile.m_projPath, htFile.m_goldPath); break;
					case eExtra: FileDelete(htFile.m_goldPath); break;
					case eMatch: break;
					case eDiff: FileCopy(htFile.m_projPath, htFile.m_goldPath); break;
					case eOpenError: FileCopy(htFile.m_projPath, htFile.m_goldPath); break;
					default: break;
					}
				}
			
				ScanProject(htProj);
			}

		} else if (strcmp(cmd, "sf") == 0) {
			if (args == 1) {
				printf("  file number missing\n");
				continue;
			}
			int fileCnt = (int)(pCurHtProj->m_systemcFiles.size() + pCurHtProj->m_verilogFiles.size());
			if (num >= (int)fileCnt) {
				printf("  file number out of range (0-%d)\n", fileCnt-1);
				continue;
			}

			bool bSystemc = num < (int)pCurHtProj->m_systemcFiles.size();
			CHtFile & htFile = bSystemc ? pCurHtProj->m_systemcFiles[num] : pCurHtProj->m_verilogFiles[num - pCurHtProj->m_systemcFiles.size()];

			ScanProject(*pCurHtProj);

		} else if (strcmp(cmd, "sp") == 0) {
			if (args == 1) {
				printf("  project number missing\n");
				continue;
			}
			if (num >= (int)projList.size()) {
				printf("  project number out of range (0-%d)\n", (int)projList.size()-1);
				continue;
			}

			CHtProj & htProj = projList[num];

			ScanProject(htProj);

			printf("Scan project %s and list files with differences\n", projList[num].m_projName.c_str());

			for (size_t i = 0; i < projList[num].m_systemcFiles.size(); i += 1) {
				char c = GetStatusChar(projList[num].m_systemcFiles[i].m_fileStatus);
				if (c == ' ') continue;
				printf("  %2d   %c %s\n", (int)i, c, projList[num].m_systemcFiles[i].m_fileName.c_str());
			}

			for (size_t i = 0; i < projList[num].m_verilogFiles.size(); i += 1) {
				char c = GetStatusChar(projList[num].m_verilogFiles[i].m_fileStatus);
				if (c == ' ') continue;
				printf("  %2d   %c %s\n", (int)(projList[num].m_systemcFiles.size() + i), c, projList[num].m_verilogFiles[i].m_fileName.c_str());
			}

			pCurHtProj = &projList[num];

		} else if (strcmp(cmd, "sa") == 0) {
			for (size_t pi = 0; pi < projList.size(); pi += 1) {
				CHtProj & htProj = projList[pi];

				ScanProject(htProj);
			}

		} else if (strcmp(cmd, "df") == 0) {
			if (args == 1) {
				printf("  file number missing\n");
				continue;
			}

			if (pCurHtProj == 0) {
				printf("  must use lp command to select current project\n");
				continue;
			}

			int fileCnt = (int)(pCurHtProj->m_systemcFiles.size() + pCurHtProj->m_verilogFiles.size());
			if (num >= (int)fileCnt) {
				printf("  file number out of range (0-%d)\n", fileCnt - 1);
				continue;
			}

			bool bSystemc = num < (int)pCurHtProj->m_systemcFiles.size();
			CHtFile & htFile = bSystemc ? pCurHtProj->m_systemcFiles[num] : pCurHtProj->m_verilogFiles[num - pCurHtProj->m_systemcFiles.size()];

			printf("Diff file %s\n", htFile.m_fileName.c_str());

			char cmdLine[256];
			sprintf(cmdLine, "C:/HtGolden/WinDiff.exe \"%s\" \"%s\"",
				htFile.m_projPath.c_str(), htFile.m_goldPath.c_str());

			system(cmdLine);

			ScanProject(*pCurHtProj);

		} else if (strcmp(cmd, "dp") == 0) {
			if (args == 1) {
				printf("  project number missing\n");
				continue;
			}
			if (num >= (int)projList.size()) {
				printf("  project number out of range (0-%d)\n", (int)projList.size() - 1);
				continue;
			}

			pCurHtProj = &projList[num];

			printf("Diff files of project %s\n", pCurHtProj->m_projName.c_str());
			for (size_t i = 0; i < pCurHtProj->m_systemcFiles.size(); i += 1) {
				CHtFile & htFile = pCurHtProj->m_systemcFiles[i];

				if (GetStatusChar(htFile.m_fileStatus) == ' ')
					continue;

				if (GetStatusChar(htFile.m_fileStatus) != 'd') {
					printf("Unable to diff file %s\n", htFile.m_fileName.c_str());
					continue;
				}

				printf("Diff file %s\n", htFile.m_fileName.c_str());

				char cmdLine[256];
				sprintf(cmdLine, "C:/HtGolden/WinDiff.exe \"%s\" \"%s\"",
					htFile.m_projPath.c_str(), htFile.m_goldPath.c_str());

				system(cmdLine);
			}

			for (size_t i = 0; i < projList[num].m_verilogFiles.size(); i += 1) {
				CHtFile & htFile = pCurHtProj->m_verilogFiles[i];

				if (GetStatusChar(htFile.m_fileStatus) == ' ')
					continue;

				if (GetStatusChar(htFile.m_fileStatus) != 'd') {
					printf("Unable to diff file %s\n", htFile.m_fileName.c_str());
					continue;
				}

				printf("Diff file %s\n", htFile.m_fileName.c_str());

				char cmdLine[256];
				sprintf(cmdLine, "C:/HtGolden/WinDiff.exe \"%s\" \"%s\"",
					htFile.m_projPath.c_str(), htFile.m_goldPath.c_str());

				system(cmdLine);
			}

		} else if (strcmp(cmd, "lo") == 0) {
			printf("Options:\n");
			printf("  (0)  Ignore White Space     %s\n", bIgnoreWhiteSpace ? "on" : "off");
			printf("  (1)  Ignore Line Order      %s\n", bIgnoreLineOrder ? "on" : "off");

		} else if (strcmp(cmd, "to") == 0) {

			if (args == 1) {
				printf("  option number missing\n");
				continue;
			}

			switch (num) {
			case 0: bIgnoreWhiteSpace = !bIgnoreWhiteSpace; break;
			case 1: bIgnoreLineOrder = !bIgnoreLineOrder; break;
			default: printf("  option number out of range (0-1)\n"); break;
			}

		} else {
			if (strcmp(cmd, "h") != 0 && strcmp(cmd, "help") != 0)
				printf("Unknown command\n");

			printf("Help\n");
			printf("  la             - list all projects\n");
			printf("  lad            - list projects with differences\n");
			printf("  lp <proj num>  - list all files of project\n");
			printf("  lpd <proj num> - list files of project with differences");
			printf("  lps <proj num> - list systemc files of project\n");
			printf("  lpv <proj num> - list verilog files of project\n");
			printf("  df <file num>  - diff file\n");
			printf("  ua             - update all files of all projects\n");
			printf("  up <proj num>  - update all files of project\n");
			printf("  ups <proj num> - update systemc files of project\n");
			printf("  upv <proj num> - update verilog files of project\n");
			printf("  uf <file num>  - update file\n");
			printf("  sa             - scan all files of all projects\n");
			printf("  sp <proj num>  - scan all files of project\n");
			printf("  sf <file num>  - scan file\n");
			printf("  lo             - list options\n");
			printf("  to <opt num>   - toggle option\n");
			printf("HtDiff - v1.0\n");

		}
	}

	return 0;
}

void ScanProject(CHtProj & htProj)
{
	vector<string> projSystemcFiles;
	vector<string> projVerilogFiles;
	vector<string> goldSystemcFiles;
	vector<string> goldVerilogFiles;

	htProj.m_projStatus = "   ";
	htProj.m_systemcFiles.clear();
	htProj.m_verilogFiles.clear();

	DIR *pDir;
	struct dirent * pDirEnt;
	if ((pDir = opendir(htProj.m_projSystemcDir.c_str())) != 0) {
		while ((pDirEnt = readdir(pDir)) != 0) {
			string suffix = pDirEnt->d_name;
			if (suffix.find("-") != string::npos)
				continue;
			int n = suffix.find_last_of(".");
			if (n != string::npos)
				suffix.erase(0, n+1);
			if (suffix == "cpp" || suffix == "h" || suffix == "sc")
				projSystemcFiles.push_back(pDirEnt->d_name);
		}
		closedir(pDir);
	}

	if ((pDir = opendir(htProj.m_projVerilogDir.c_str())) != 0) {
		while ((pDirEnt = readdir(pDir)) != 0) {
			string suffix = pDirEnt->d_name;
			if (suffix.find("-") != string::npos)
				continue;
			int n = suffix.find_last_of(".");
			if (n != string::npos)
				suffix.erase(0, n+1);
			if (suffix == "v")
				projVerilogFiles.push_back(pDirEnt->d_name);
		}
		closedir(pDir);
	}

	if ((pDir = opendir(htProj.m_goldSystemcDir.c_str())) != 0) {
		while ((pDirEnt = readdir(pDir)) != 0) {
			string suffix = pDirEnt->d_name;
			int n = suffix.find_last_of(".");
			if (n > 0)
				suffix.erase(0, n+1);
			if (suffix == "cpp" || suffix == "h" || suffix == "sc")
				goldSystemcFiles.push_back(pDirEnt->d_name);
		}
		closedir(pDir);
	}

	if ((pDir = opendir(htProj.m_goldVerilogDir.c_str())) != 0) {
		while ((pDirEnt = readdir(pDir)) != 0) {
			string suffix = pDirEnt->d_name;
			int n = suffix.find_last_of(".");
			if (n > 0)
				suffix.erase(0, n+1);
			if (suffix == "v")
				goldVerilogFiles.push_back(pDirEnt->d_name);
		}
		closedir(pDir);
	}

	// check for missing and extra systemc files
	size_t pi = 0;
	size_t gi = 0;
	while (pi < projSystemcFiles.size() || gi < goldSystemcFiles.size()) {
		if (pi < projSystemcFiles.size() && gi < goldSystemcFiles.size() && projSystemcFiles[pi] == goldSystemcFiles[gi]) {
			CHtFile htFile;
			htFile.m_fileName = projSystemcFiles[pi];
			htFile.m_projPath = htProj.m_projSystemcDir + "/" + htFile.m_fileName;
			htFile.m_goldPath = htProj.m_goldSystemcDir + "/" + htFile.m_fileName;
			htFile.m_fileStatus = FileCompare(htFile.m_projPath, htFile.m_goldPath);
			htProj.m_systemcFiles.push_back(htFile);
			pi += 1;
			gi += 1;
			if (htFile.m_fileStatus != eMatch)
				htProj.m_projStatus[0] = 'd';
			continue;
		}
		if (gi == goldSystemcFiles.size() || pi < projSystemcFiles.size() && projSystemcFiles[pi] < goldSystemcFiles[gi]) {
			CHtFile htFile;
			htFile.m_fileName = projSystemcFiles[pi];
			htFile.m_projPath = htProj.m_projSystemcDir + "/" + htFile.m_fileName;
			htFile.m_goldPath = htProj.m_goldSystemcDir + "/" + htFile.m_fileName;
			htFile.m_fileStatus = eMissing;
			htProj.m_systemcFiles.push_back(htFile);
			pi += 1;
			htProj.m_projStatus[1] = 'm';
			continue;
		}
		if (pi == projSystemcFiles.size() || projSystemcFiles[pi] > goldSystemcFiles[gi]) {
			CHtFile htFile;
			htFile.m_fileName = goldSystemcFiles[gi];
			htFile.m_projPath = htProj.m_projSystemcDir + "/" + htFile.m_fileName;
			htFile.m_goldPath = htProj.m_goldSystemcDir + "/" + htFile.m_fileName;
			htFile.m_fileStatus = eExtra;
			htProj.m_systemcFiles.push_back(htFile);
			gi += 1;
			htProj.m_projStatus[2] = 'e';
			continue;
		}
	}

	// check for missing and extra verilog files
	pi = 0;
	gi = 0;
	while (pi < projVerilogFiles.size() || gi < goldVerilogFiles.size()) {
		if (pi < projVerilogFiles.size() && gi < goldVerilogFiles.size() && projVerilogFiles[pi] == goldVerilogFiles[gi]) {
			CHtFile htFile;
			htFile.m_fileName = projVerilogFiles[pi];
			htFile.m_projPath = htProj.m_projVerilogDir + "/" + htFile.m_fileName;
			htFile.m_goldPath = htProj.m_goldVerilogDir + "/" + htFile.m_fileName;
			htFile.m_fileStatus = FileCompare(htFile.m_projPath, htFile.m_goldPath);
			htProj.m_verilogFiles.push_back(htFile);
			pi += 1;
			gi += 1;
			if (htFile.m_fileStatus != eMatch)
				htProj.m_projStatus[0] = 'd';
			continue;
		}
		if (gi == goldVerilogFiles.size() || pi < projVerilogFiles.size() && projVerilogFiles[pi] < goldVerilogFiles[gi]) {
			CHtFile htFile;
			htFile.m_fileName = projVerilogFiles[pi];
			htFile.m_projPath = htProj.m_projVerilogDir + "/" + htFile.m_fileName;
			htFile.m_goldPath = htProj.m_goldVerilogDir + "/" + htFile.m_fileName;
			htFile.m_fileStatus = eMissing;
			htProj.m_verilogFiles.push_back(htFile);
			pi += 1;
			htProj.m_projStatus[1] = 'm';
			continue;
		}
		if (pi == projVerilogFiles.size() || gi < goldVerilogFiles.size() && projVerilogFiles[pi] > goldVerilogFiles[gi]) {
			CHtFile htFile;
			htFile.m_fileName = goldVerilogFiles[gi];
			htFile.m_projPath = htProj.m_projVerilogDir + "/" + htFile.m_fileName;
			htFile.m_goldPath = htProj.m_goldVerilogDir + "/" + htFile.m_fileName;
			htFile.m_fileStatus = eExtra;
			htProj.m_verilogFiles.push_back(htFile);
			gi += 1;
			htProj.m_projStatus[1] = 'e';
			continue;
		}
	}
}

EFileStatus FileCompare(string &file1, string &file2) {
	FILE *fp1 = fopen(file1.c_str(), "r");
	if (fp1 == 0) {
		fprintf(stderr, "Unable to open '%s'\n", file1.c_str());
		return eOpenError;
	}

	FILE *fp2 = fopen(file2.c_str(), "r");
	if (fp2 == 0) {
		fprintf(stderr, "Unable to open '%s'\n", file2.c_str());
		fclose(fp1);
		return eOpenError;
	}

	EFileStatus status = eMatch;

	vector<string> vlines1;
	vector<string> vlines2;

	// read all lines into vector of strings
	char buf[1024];
	while (fgets(buf, 1024, fp1)) {
		if (bIgnoreWhiteSpace)
			IgnoreWhiteSpace(buf);
		if (!bIgnoreWhiteSpace || buf[0] != '\0')
			vlines1.push_back(buf);
	}

	while (fgets(buf, 1024, fp2)) {
		if (bIgnoreWhiteSpace)
			IgnoreWhiteSpace(buf);
		if (!bIgnoreWhiteSpace || buf[0] != '\0')
			vlines2.push_back(buf);
	}

	if (vlines1.size() != vlines2.size())
		status = eDiff;
	else {
		if (bIgnoreLineOrder) {
			// compares files allowing order of lines to change
			// sort lines
			for (size_t i = 0; i < vlines1.size(); i += 1) {
				for (size_t j = 0; j < vlines1.size(); j += 1) {
					if (strcmp(vlines1[i].c_str(), vlines1[j].c_str()) > 0) {
						string tmp = vlines1[i];
						vlines1[i] = vlines1[j];
						vlines1[j] = tmp;
					}
				}
			}

			for (size_t i = 0; i < vlines2.size(); i += 1) {
				for (size_t j = 0; j < vlines2.size(); j += 1) {
					if (strcmp(vlines2[i].c_str(), vlines2[j].c_str()) > 0) {
						string tmp = vlines2[i];
						vlines2[i] = vlines2[j];
						vlines2[j] = tmp;
					}
				}
			}
		}

		for (size_t i = 0; i < vlines1.size(); i += 1) {
			if (vlines1[i] != vlines2[i]) {
				status = eDiff;
				break;
			}
		}
	}

	fclose(fp1);
	fclose(fp2);

	return status;
}

char GetStatusChar(EFileStatus status)
{
	switch (status) {
	case eMissing: return 'm';
	case eOpenError: return 'x';
	case eDiff: return 'd';
	case eMatch: return ' ';
	case eExtra: return 'e';
	}

	return '?';
}

void FileCopy(string &srcFile, string &dstFile)
{
	FILE *fp1 = fopen(srcFile.c_str(), "r");
	if (fp1 == 0) {
		fprintf(stderr, "Unable to open '%s' for reading\n", srcFile.c_str());
		return;
	}

	FILE *fp2 = fopen(dstFile.c_str(), "w");
	if (fp2 == 0) {
		fprintf(stderr, "Unable to open '%s' for writing\n", dstFile.c_str());
		fclose(fp1);
		return;
	}

	for (;;) {
		char buf[8192];

		int n1 = fread(buf, 1, 8192, fp1);
		int n2 = fwrite(buf, 1, n1, fp2);

		if (n1 == 0)
			break;

		if (n1 != n2) {
			printf("error writing to file '%s'\n", dstFile.c_str());
			break;
		}
	}

	fclose(fp1);
	fclose(fp2);
}

void FileDelete(string &file)
{
	if (remove(file.c_str()) != 0)
		printf("Error removing file '%s'\n", file.c_str());
}

bool CreateSubDir(string path, char * pSub1, char * pSub2)
{
	if ((CreateDirectory(path.c_str(), 0) || GetLastError() == ERROR_ALREADY_EXISTS) 
		&& (CreateDirectory((path + pSub1).c_str(), 0) || GetLastError() == ERROR_ALREADY_EXISTS) 
		&& (CreateDirectory((path + pSub1 + pSub2).c_str(), 0) || GetLastError() == ERROR_ALREADY_EXISTS))
		return true;

	printf("Could not create directories for '%s'\n", path.c_str());
	return false;
}

void IgnoreWhiteSpace(char * pBuf)
{
	// remove all white space (in place)
	char * pIn = pBuf;
	char * pOut = pBuf;
	while (*pIn) {
		if (*pIn == ' ' || *pIn == '\t' || *pIn == '\n' || *pIn == '\r') {
			pIn += 1;
			continue;
		}

		*pOut++ = *pIn++;
	}

	pOut = '\0';
}
