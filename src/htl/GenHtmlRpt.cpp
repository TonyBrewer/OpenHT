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
#include "GenHtmlRpt.h"

CGenHtmlRpt::CGenHtmlRpt(string htlPath, string fileName, int argc, char const **argv)
{
	m_fp = fopen(fileName.c_str(), "w");

	if (m_fp == 0) {
		fprintf(stderr, "Could not open %s\n", fileName.c_str());
		exit(1);
	}

	m_bPendingText = false;

	// Path to html helper files
#ifdef WIN32
	m_htmlPath = "C:/Ht/html";
#else
	int pos = htlPath.find_last_of("/\\");
	m_htmlPath = htlPath.substr(0, pos) + "/../html";
#endif

	fprintf(m_fp, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n");
	fprintf(m_fp, "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n");
	fprintf(m_fp, "    <head>\n");
	fprintf(m_fp, "        <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n");
	fprintf(m_fp, "        <title>HTL Report</title>\n");
	fprintf(m_fp, "        <link rel=\"stylesheet\" href=\"%s/css/style.css\" type=\"text/css\" media=\"screen, projection\" />\n", m_htmlPath.c_str());
	fprintf(m_fp, "        <script type=\"text/javascript\" src=\"%s/js/jquery-1.4.2.min.js\"></script>\n", m_htmlPath.c_str());
	fprintf(m_fp, "        <script type=\"text/javascript\" src=\"%s/js/scripts.js\"></script>\n", m_htmlPath.c_str());
	fprintf(m_fp, "    </head>\n");
	fprintf(m_fp, "    <body>\n");
	fprintf(m_fp, "        <h1><b>Summary</b></h1>\n");
	fprintf(m_fp, "          <dl>\n");
	fprintf(m_fp, "            <dt>Execution Date:</dt>\n");
	time_t t;
	struct tm *tm;
	char str[256];
	t = time(NULL);
	tm = localtime(&t);
	strftime(str, sizeof(str), "%a, %d %b %y %H:%M:%S %z", tm);
	fprintf(m_fp, "              <dd>%s</dd>\n", str);
	fprintf(m_fp, "              <br/>\n");
	fprintf(m_fp, "            <dt>Execution Directory:</dt>\n");
	getcwd(str, sizeof(str));
	fprintf(m_fp, "              <dd>%s</dd>\n", str);
	fprintf(m_fp, "              <br/>\n");
	fprintf(m_fp, "            <dt>Command Line:</dt>\n");
	fprintf(m_fp, "              <dd>");
	for (int i = 0; i<argc; i++)
		fprintf(m_fp, " %s", argv[i]);
	fprintf(m_fp, "</dd>\n");
}

void CGenHtmlRpt::AddLevel(const char *pFormat, ...)
{
	HtlAssert(this);
	if (m_fp == 0)
		return;

	m_indentLevel += 1;

	HtlAssert(!m_bPendingText);

	va_list args;
	va_start(args, pFormat);

	char buf1[1024];
	vsprintf(buf1, pFormat, args);

	char buf2[1024];
	char * p1 = buf1;
	char * p2 = buf2;

	if (m_indentLevel <= m_maxBoldLevel) {
		*p2++ = '<';
		*p2++ = 'b';
		*p2++ = '>';
	}
	while (*p1 != '\0') {
		if (*p1 == '<') {
			*p2++ = '&'; *p2++ = 'l'; *p2++ = 't'; *p2++ = ';';
		} else
			*p2++ = *p1;
		p1++;
	}
	*p2 = '\0';

	if (m_bPendingIndent) {
		fprintf(m_fp, "%s    <ul>\n", m_indent.c_str());
		m_indent += "        ";
		m_bPendingIndent = false;
	}

	m_bPendingText = *(p2 - 1) != '\n';
	if (!m_bPendingText)
		*(p2 - 1) = '\0';

	fprintf(m_fp, "%s<li>\n", m_indent.c_str());
	fprintf(m_fp, "%s    %s", m_indent.c_str(), buf2);

	if (m_indentLevel <= m_maxBoldLevel)
		sprintf(m_pendingTextStr, "</b>\n");
	else
		sprintf(m_pendingTextStr, "\n");

	m_bPendingIndent = true;

	if (!m_bPendingText)
		fputs(m_pendingTextStr, m_fp);
}

void CGenHtmlRpt::AddItem(const char *pFormat, ...)
{
	HtlAssert(this);
	if (m_fp == 0)
		return;

	HtlAssert(!m_bPendingText);

	va_list args;
	va_start(args, pFormat);

	char buf1[1024];
	vsprintf(buf1, pFormat, args);

	char buf2[1024];
	char * p1 = buf1;
	char * p2 = buf2;
	while (*p1 != '\0') {
		if (*p1 == '<') {
			*p2++ = '&'; *p2++ = 'l'; *p2++ = 't'; *p2++ = ';';
		} else
			*p2++ = *p1;
		p1++;
	}
	*p2 = '\0';

	if (m_bPendingIndent) {
		fprintf(m_fp, "%s    <ul>\n", m_indent.c_str());
		m_indent += "        ";
		m_bPendingIndent = false;
	}

	m_bPendingText = *(p2 - 1) != '\n';
	if (!m_bPendingText)
		*(p2 - 1) = '\0';

	fprintf(m_fp, "%s<li>\n", m_indent.c_str());
	fprintf(m_fp, "%s    <span>%s", m_indent.c_str(), buf2);

	sprintf(m_pendingTextStr, "</span>\n%s</li>\n", m_indent.c_str());

	if (!m_bPendingText)
		fputs(m_pendingTextStr, m_fp);
}

void CGenHtmlRpt::AddText(const char *pFormat, ...)
{
	HtlAssert(this);
	if (m_fp == 0)
		return;

	HtlAssert(m_bPendingText);

	va_list args;
	va_start(args, pFormat);

	char buf1[1024];
	vsprintf(buf1, pFormat, args);

	char buf2[1024];
	char * p1 = buf1;
	char * p2 = buf2;
	while (*p1 != '\0') {
		if (*p1 == '<') {
			*p2++ = '&'; *p2++ = 'l'; *p2++ = 't'; *p2++ = ';';
		} else
			*p2++ = *p1;
		p1++;
	}
	*p2 = '\0';

	m_bPendingText = *(p2 - 1) != '\n';
	if (!m_bPendingText)
		*(p2 - 1) = '\0';

	fprintf(m_fp, "%s", buf2);

	if (!m_bPendingText)
		fputs(m_pendingTextStr, m_fp);
}

void CGenHtmlRpt::EndLevel()
{
	HtlAssert(this);
	if (m_fp == 0)
		return;

	m_indentLevel -= 1;

	HtlAssert(!m_bPendingText);

	if (!m_bPendingIndent) {
		m_indent.erase(0, 8);
		fprintf(m_fp, "%s    </ul>\n", m_indent.c_str());
	}
	fprintf(m_fp, "%s</li>\n", m_indent.c_str());
	m_bPendingIndent = false;
}

CGenHtmlRpt::~CGenHtmlRpt()
{
	if (m_fp) {
		fprintf(m_fp, "    </body>\n");
		fprintf(m_fp, "</html>\n");

		fclose(m_fp);
	}
}

void CGenHtmlRpt::GenApiHdr()
{
	fprintf(m_fp, "        <h1><b>Generated APIs</b></h1>\n");
	fprintf(m_fp, "        <div id=\"listContainer\">\n");
	fprintf(m_fp, "            <div class=\"listControl\">\n");
	fprintf(m_fp, "                <a id=\"expandList\">Expand All</a>\n");
	fprintf(m_fp, "                <a id=\"collapseList\">Collapse All</a>\n");
	fprintf(m_fp, "            </div>\n");
	fprintf(m_fp, "            <ul id=\"expList\">\n");

	m_indent = "                ";
	m_bPendingIndent = false;
	m_indentLevel = 0;
	m_maxBoldLevel = 1;
}

void CGenHtmlRpt::GenApiEnd()
{
	fprintf(m_fp, "            </ul>\n");
	fprintf(m_fp, "        </div>\n");
}

void CGenHtmlRpt::GenCallGraph()
{
	fprintf(m_fp, "        <h1><b>Generated Call Graph</b></h1>\n");
	fprintf(m_fp, "        <object type=\"image/png\" data=\"ht/HtCallGraph.png\"></object>\n");
}
