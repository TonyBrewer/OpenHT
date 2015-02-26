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

#include "CnyHt.h"

enum EMsvsFile { Undefined, Include, Compile, Custom, CustomHtLib, NonBuild, Filter };

struct CMsvsFilter {
	CMsvsFilter(string filterName, string extensions, bool bFound)
		: m_filterName(filterName), m_extensions(extensions), m_bFound(bFound)
	{
	}

	string m_filterName;
	string m_extensions;
	bool m_bFound;
};

struct CMsvsFile {
	CMsvsFile(EMsvsFile fileType, string pathName, string filterName, string dependencies, bool bGenFx, bool bFound)
		: m_fileType(fileType), m_pathName(pathName), m_filterName(filterName), m_dependencies(dependencies), m_bGenFx(bGenFx), m_bFound(bFound)
	{
	}

	EMsvsFile m_fileType;
	string m_pathName;
	string m_filterName;
	string m_dependencies;
	vector<string> m_excludeFromBuild;
	vector<string> m_extraDefines[4];
	vector<string> m_forcedIncludeFiles[4];
	bool m_bGenFx;
	bool m_bFound;
};

class CMsvsProject {
public:
	CMsvsProject(CHtString projName, CHtString unitName)
	{
		m_projName = projName;
		m_unitName = unitName;
	}

	void AddFilter(string filterName, string extensions, bool bFound = false)
	{
		m_filterList.push_back(CMsvsFilter(filterName, extensions, bFound));
	}

	void AddFile(EMsvsFile fileType, string pathName, string filterName, string dependencies = "", bool bGenFx = false, bool bFound = false)
	{
		m_fileList.push_back(CMsvsFile(fileType, pathName, filterName, dependencies, bGenFx, bFound));

		if (pathName.substr(pathName.size() - 3) == ".sc" || pathName.substr(pathName.size() - 3) == ".v_") {
			size_t pos = pathName.find_last_of("\\");

			string verilogPathName = "..\\ht\\verilog\\" + pathName.substr(pos + 1, pathName.size() - pos - 4) + ".v";

			m_fileList.push_back(CMsvsFile(NonBuild, verilogPathName, "ht\\verilog", "", bGenFx, false));
		}

		if (pathName.substr(pathName.size() - 3) == ".v_") {
			// create the .v_ file if it doesn't already exist
			struct stat buf;
			if (stat(pathName.c_str(), &buf) != 0) {
				FILE *fp = fopen(pathName.c_str(), "w");
				if (fp)
					fclose(fp);
			}
		}
	}

	string GenGuid();

	bool CheckIfProjFilterFileOkay();
	bool CheckIfProjFileOkay();
	void GenMsvsFiles();

	void GenMsvsFilterHeader(FILE *fp);
	void GenMsvsFiltersXml(FILE *fp);
	void GenMsvsFilterFilesXml(FILE *fp, EMsvsFile eFileType);
	void GenMsvsFilterTrailer(FILE *fp);

	bool CheckRequiredProperties(vector<string> &reqPropList);

	void ReadMsvsProjectFile();
	void ReadItemDefinitionGroup();

	void ReadPropertyGroup();
	void ReadConfigurationType(int condIdx);

	void ReadItemGroup();
	void ReadNextProperty(string &propStr, bool &bOpen);
	bool ReadCondition(string &conditionStr);
	void ReadPreprocessorDefinitions(int condIdx);
	void ReadAdditionalIncludeDirectories(int condIdx);
	void ReadAdditionalLibraryDirectories(int condIdx);
	void ReadAdditionalDependencies(int condIdx);
	void ReadForcedIncludeFiles(int condIdx);
	void ReadDisableSpecificWarnings(int condIdx);
	void ReadCommandDefines(vector<string> &extraDefines);
	void ReadForcedIncludeFiles(vector<string> &forcedIncludeFiles);
	void ReadPropertyValue(string &valueStr);
	void ParsePropertyValueStr(string &valueStr, vector<string> &valueList);
	void ParseCommandValueStr(string &valueStr, vector<string> &valueList);
	bool ReadPropertyQualifier(string &nameStr, string &valueStr);
	bool PathCmp(string path1, string path2, size_t len = 0x7fffffff);
	void AddStringToList(vector<string> & list, string & str);

	void GenMsvsProjectHeader(FILE *fp);
	void GenMsvsProjectFiles(FILE *fp, EMsvsFile eFileType);
	void GenMsvsProjectTrailer(FILE *fp);

public:
	CHtString				m_projName;
	CHtString				m_unitName;

	vector<CMsvsFilter>		m_filterList;
	vector<CMsvsFile>		m_fileList;

	// ItemDefinitionGroup values
	struct CItemDefintionGroup {
		CItemDefintionGroup() { m_bHtModel = false; }
		bool m_bHtModel;
		vector<string> m_preprocessorDefintions;
		vector<string> m_additionalIncludeDirectories;
		vector<string> m_additionalLibraryDirectories;
		vector<string> m_additionalDependencies;
		vector<string> m_forcedIncludeFiles;
		vector<string> m_disableSpecificWarnings;
		vector<string> m_configurationType;
	} m_idg[4];

	// xml parsing variables
	FILE *	m_fp;
	bool	m_bEof;
	char	m_line[1024];
	char *	m_pLine;
};
