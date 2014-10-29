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

#include <Windows.h>
#include <stdio.h>

#include <string>

#include "system_exception.h"

using namespace std;

class FileInfo
{
public:

	FileInfo() {};
	FileInfo::FileInfo(wstring filename);
	~FileInfo();

	DWORD analyze();
	bool operator <(const FileInfo& f);
	VOID open_for_fragmentation();
	VOID close();

	HANDLE handle()				{ return handle_; }
	LONGLONG frag_count()		{ return frag_count_; };
	LONGLONG cluster_count()	{ return cluster_count_; };
	wstring fullpath()			{ return fullpath_; };
	bool is_folder()			{ return is_folder_; };

private:

	HANDLE handle_;
	wstring fullpath_;
	LONGLONG frag_count_;
	LONGLONG cluster_count_;
	bool is_folder_;

};

