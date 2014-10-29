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

#include "fileinfo.h"


 
FileInfo::FileInfo(wstring filename)
{
	// full path
	DWORD lenght = GetFullPathName(filename.c_str(), 0, NULL, NULL);

	wchar_t* tmp = new wchar_t[lenght];
	if (!GetFullPathName(filename.c_str(), lenght, tmp, NULL))
		throw new SystemException(GetLastError());

	fullpath_ = wstring(tmp);
	delete tmp;

	// folder flag

	DWORD rc = GetFileAttributes(fullpath_.c_str());
	if (GetFileAttributes(fullpath_.c_str()) == INVALID_FILE_ATTRIBUTES)
		throw new SystemException(GetLastError());

	is_folder_ = rc & FILE_ATTRIBUTE_DIRECTORY ? true : false;

	if (!is_folder_)
		this->analyze();
}

bool FileInfo::operator <(const FileInfo& f)
{
	return (cluster_count_ < f.cluster_count_);
}

VOID FileInfo::open_for_fragmentation()
{
	handle_ = CreateFile(fullpath_.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, 0);
	
	if (handle_ == INVALID_HANDLE_VALUE)
		throw new SystemException(GetLastError());
}

VOID FileInfo::close()
{
	if (!CloseHandle(handle_))
		throw new SystemException(GetLastError());
	
	handle_ = INVALID_HANDLE_VALUE;
}

DWORD FileInfo::analyze()
{
	BOOLEAN finished;
	BYTE buffer[(sizeof(RETRIEVAL_POINTERS_BUFFER)) + (2 * sizeof(LARGE_INTEGER) * 511)]; // 512 extents
	PRETRIEVAL_POINTERS_BUFFER retrieval_pointers = (PRETRIEVAL_POINTERS_BUFFER)buffer;
	DWORD dummy = 0, i;
	LONGLONG starting_vcn = 0, ending_lcn = 0;

	open_for_fragmentation();
	
	frag_count_ = 1;
	cluster_count_ = 0;

	do
	{
		ZeroMemory(buffer, sizeof(buffer));

		finished = DeviceIoControl(handle_,
									FSCTL_GET_RETRIEVAL_POINTERS,
									&starting_vcn, sizeof(starting_vcn),
									retrieval_pointers, sizeof(buffer), &dummy, NULL);

		if (!finished && GetLastError() != ERROR_MORE_DATA)
		{
			close();
			if (GetLastError() == ERROR_HANDLE_EOF) 
				return ERROR_SUCCESS;
			else
				throw new SystemException(GetLastError());
		}
			

		for (i = 0; i < retrieval_pointers->ExtentCount; i++)
		{
			LONGLONG length;

			if (i == 0)
				length = retrieval_pointers->Extents[i].NextVcn.QuadPart - retrieval_pointers->StartingVcn.QuadPart;
			else
				length = retrieval_pointers->Extents[i].NextVcn.QuadPart - retrieval_pointers->Extents[i - 1].NextVcn.QuadPart;

			if (starting_vcn != 0 && (ending_lcn != retrieval_pointers->Extents[i].Lcn.QuadPart))
				frag_count_++;

			cluster_count_ += length;
			ending_lcn = retrieval_pointers->Extents[i].Lcn.QuadPart + length;
			starting_vcn = retrieval_pointers->Extents[i].NextVcn.QuadPart;
		}

	} while (!finished);

	close();

	return ERROR_SUCCESS;
}

FileInfo::~FileInfo()
{
	//if (fullpath_)
	//	delete fullpath_;
}
