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

#include "stdafx.h"
#include "cmdline.h"
#include "defragmenter.h"
#include "system_exception.h"

int wmain(int argc, wchar_t* argv[])
{
	wchar_t *file_or_folder = NULL;
	CmdLine c(argc, argv);

	c.print_header();

	try
	{
		if (c.is_param(kParamAnalyzeFile) && (file_or_folder = c.get_param(kParamAnalyzeFile))) {
			// Analyze option 
			Defragmenter d(wstring(file_or_folder), false, kAnalyze);
			d.run();
		}
		else if (c.is_param(kParamDefragFile) && (file_or_folder = c.get_param(kParamDefragFile))) {
			// Defragmentation
			Defragmenter d(wstring(file_or_folder), false, kDefragment);
			d.run();
		}
		else if (c.is_param(kParamFragmentFile) && (file_or_folder = c.get_param(kParamFragmentFile))) {
			// Fragmentation
			Defragmenter d(wstring(file_or_folder), false, kFragment);
			d.run();
		}
		else if (c.is_param(kParamFreeAreaAnalysis) && (file_or_folder = c.get_param(kParamFreeAreaAnalysis))) {
			// Frea area analysis
			Defragmenter d(wstring(file_or_folder), false, kFreeAreaAnalysis);
			d.run();
		}
		else
		{
			c.print_usage();
		}
	}
	catch (SystemException *e)
	{
		wcout << L"Error: " << e->message() << endl;
		delete e;
	}
 
	return 0;
}

