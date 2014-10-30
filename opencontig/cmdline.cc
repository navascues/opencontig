/*
Copyright (c) 2014 - Julian de Navascues, julian.navascues@outlook.com

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http ://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "cmdline.h"

static const wchar_t* kFreeContigVersion = L"v1.0";
static const wchar_t* kFreeContigEmail = L"<julian.navascues@outlook.com>";
const wchar_t* kParamAnalyzeFile = L"-analyze";
const wchar_t* kParamDefragFile = L"-defrag";
const wchar_t* kParamFragmentFile = L"-fragment";
const wchar_t* kParamFreeAreaAnalysis = L"-freearea";

CmdLine::CmdLine(int argc, wchar_t** argv)
{
	for (int i = 0; i < argc; i++)
		parameters_.push_back(_wcsdup(argv[i]));
}

CmdLine::~CmdLine()
{
	for (size_t i = 0; i < parameters_.size(); i++)
		delete parameters_[i];
}

bool CmdLine::is_param(const wchar_t *name)
{
	for (size_t i = 0; i < parameters_.size(); i++)
		if (_wcsicmp(parameters_[i], name) == 0)
			return true;
	
	return false;
}

wchar_t* CmdLine::get_param(const wchar_t *name)
{
	for (size_t i = 0; i < parameters_.size(); i++)
		if (_wcsicmp(parameters_[i], name) == 0)
			return parameters_[i + 1];

	return NULL;
}

void CmdLine::print_header()
{
	wprintf(
		L"\n"
		L"OpenContig %s File defragmentation/fragmentation tool.\n"
		L"Copyright (c) 2013 %s\n"
		L"\n",
		kFreeContigVersion,
		kFreeContigEmail
		);
}

void CmdLine::print_usage()
{
	wprintf(
		L" Usage: opencontig.exe [action] [parameter]\n"
		L"\n"
		L" Actions:\n"
		L"\t %s [file or folder]\n"
		L"\n"
		L"\t\t Prints fragmentation information of a file or folder.\n"
		L"\n"
		L"\t %s [drive:]\n"
		L"\n"
		L"\t\t Analyze free space fragmentation.\n"
		L"\t\t (Administrator rights are required)\n"
		L"\n"
		L"\t %s [file or folder]\n"
		L"\n"
		L"\t\t Defragments file or folder contents.\n"
		L"\t\t (Administrator rights are required)\n"
		L"\n"
		L"\t %s [file or folder]\n"
		L"\n"
		L"\t\t THINK TWICE BEFORE USING THIS FEATURE \n"
		L"\n"
		L"\t\t Splits file or folder contents in to the\n"
		L"\t\t maximum number of fragments (i.e. filesize/clustersize).\n"
		L"\t\t It also ruins the order of virtual clusters,\n" 
		L"\t\t leading to a slower access for the split files.\n"
		L"\t\t (Administrator rights are required)\n"
		L"\n",
			kParamAnalyzeFile,
			kParamFreeAreaAnalysis,
			kParamDefragFile,
			kParamFragmentFile 
		);
}