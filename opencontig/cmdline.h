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

#pragma once

#include <vector>
using namespace std;

extern const wchar_t* kParamQuiet;
extern const wchar_t* kParamAnalyzeFile;
extern const wchar_t* kParamDefragFile;
extern const wchar_t* kParamFragmentFile;
extern const wchar_t* kParamFreeAreaAnalysis;

class CmdLine
{
public:
	static int a;

	CmdLine() {};
	CmdLine::CmdLine(int argc, wchar_t** argv);
	~CmdLine();
	bool CmdLine::is_param(const wchar_t *name);
	wchar_t* CmdLine::get_param(const wchar_t *name);

	void CmdLine::print_header();
	void CmdLine::print_usage();

private:

	vector<wchar_t*> parameters_;
};

