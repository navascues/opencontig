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

#include <iostream>
#include <iomanip> 
#include <string>

#include "fileinfo.h"

enum DefragmenterAction { kAnalyze, kDefragment, kFragment, kFreeAreaAnalysis };

using namespace std;

class Defragmenter
{

public:
	Defragmenter() {};
	Defragmenter(wstring file_or_folder, bool quiet, DefragmenterAction action); 
	Defragmenter(wstring drive);
	~Defragmenter();

	void run();

private:

	bool quiet_;
	wstring file_or_folder_;
	DefragmenterAction action_;

	HANDLE volume_handle_;

	double frag_count_after_;
	double frag_count_before_;
	ULONG processed_files_count_;
 
	VOID apply_action(FileInfo *f);
	VOID recursive_search(wstring  folder);
	bool find_area_for_file(FileInfo *f, PLONGLONG area_starting_lcn);
	bool defrag(HANDLE file_handle, LONGLONG area_starting_lcn);
	bool fragment(HANDLE file_handle);
	VOID move_clusters(PMOVE_FILE_DATA move_data);
	LONGLONG get_first_free_cluster();
	LONGLONG get_last_free_cluster();
	VOID open_volume();
	VOID close_volume();
	VOID free_area_analysis(wstring drive);

};

