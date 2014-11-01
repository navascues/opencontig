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

#include <Windows.h>

#include "defragmenter.h"
#include "fileinfo.h"
#include "system_exception.h"

using namespace std;

static const byte shift[] = { 1, 2, 4, 8, 16, 32, 64, 128 };
static const size_t bitmap_size = sizeof(LARGE_INTEGER) * 2 + 4096; 	// 4KB

Defragmenter::Defragmenter(wstring file_or_folder, bool quiet, DefragmenterAction action) : file_or_folder_(file_or_folder), quiet_(quiet), action_(action)
{
	processed_files_count_ = 0;
	frag_count_before_ = frag_count_after_ = 0;

	if (action == kDefragment || action == kFragment)
		open_volume();
	else
		volume_handle_ = INVALID_HANDLE_VALUE;

}

ULONG get_bytes_per_cluster(wstring drive)
{
	DWORD SectorsPerCluster;
	DWORD BytesPerSector;
	DWORD NumberOfFreeClusters;
	DWORD TotalNumberOfClusters;
 
	if (GetDiskFreeSpace(drive.c_str(), &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters))
		return SectorsPerCluster * BytesPerSector;

	wcout << L"Assuming 4096 bytes per cluster..." << endl;

	return 4096;
}

VOID Defragmenter::free_area_analysis(wstring drive)
{
	ULONG bytes_per_cluster = get_bytes_per_cluster(drive);
	wstring volume_name = L"\\\\?\\" + drive;

	volume_handle_ = CreateFile(volume_name.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
	if (volume_handle_ == INVALID_HANDLE_VALUE)
		throw new SystemException(GetLastError());

	BYTE buffer[bitmap_size];
	PVOLUME_BITMAP_BUFFER bitmap = (PVOLUME_BITMAP_BUFFER)buffer;
	DWORD nbytes = 0;

	LONGLONG lcn = 0;
	LONGLONG free_cluster_count = 0;
	LONGLONG max_free_area_length = 0;

	LONGLONG free_area_count = 0;
	LONGLONG free_area_length = 0;
 

	ZeroMemory(buffer, sizeof(buffer));

	while (!DeviceIoControl(volume_handle_, FSCTL_GET_VOLUME_BITMAP, &lcn, sizeof(lcn), bitmap, bitmap_size, &nbytes, NULL))
	{
		DWORD i, j;
		DWORD nLogicalClusters = ((nbytes - (sizeof(LARGE_INTEGER) * 2)) * 8);

		if (GetLastError() != ERROR_MORE_DATA)
			throw new SystemException(GetLastError());

		for (i = 0; i < nLogicalClusters / 8; i++)
		{
			for (j = 0; j < 8; j++)
			{
				if (bitmap->Buffer[i] & shift[j]) // Used cluster
				{
					if (free_area_length > max_free_area_length) // new max. found
						max_free_area_length = free_area_length;

					free_area_length = 0;
				}
				else // Free cluster
				{
					if (free_area_length == 0)
						free_area_count++;

					free_area_length++;
					free_cluster_count++;
				}
			}
		}

		// next FSCTL_GET_VOLUME_BITMAP
		lcn = bitmap->StartingLcn.QuadPart + nLogicalClusters;
	}

	wcout << L"Free cluster space:       " << free_cluster_count << L" clusters (" << bytes_per_cluster*free_cluster_count << L" bytes)" << endl;
	wcout << L"Free space fragments:     " << free_area_count << L" fragments" << endl;
	wcout << L"Largest free space block: " << max_free_area_length << L" clusters (" << bytes_per_cluster*max_free_area_length << L" bytes)" << endl;
}
 

VOID Defragmenter::run()
{
	if (action_ == kFreeAreaAnalysis)
	{
		free_area_analysis (file_or_folder_);
	}
	else 
	{
		FileInfo f((PWCHAR)file_or_folder_.c_str());

		if (f.is_folder())
			recursive_search(file_or_folder_);
		else
		{ 
			processed_files_count_++;
			apply_action(&f);
		}

		wcout << endl;
		wcout << L"Number of files processed: " << processed_files_count_ << endl;

		if (action_ == kAnalyze)
		{
			wcout << L"Average fragmentation:     " << frag_count_before_ / processed_files_count_ << " frags/file" << endl;
		}
		else
		{
			wcout << L"Average fragmentation before:     " << frag_count_before_ / processed_files_count_ << " frags/file" << endl;
			wcout << L"Average fragmentation after:      " << frag_count_after_ / processed_files_count_ << " frags/file" << endl;
		}
	}
}

VOID Defragmenter::recursive_search (wstring folder)
{
	WIN32_FIND_DATA data;
	HANDLE handle;
	wstring search = folder + L"\\*";

	handle = FindFirstFile(search.c_str(), &data);
	if (handle == INVALID_HANDLE_VALUE) {
		//return GetLastError();
		return;
	}

	do {

		wstring subpath = folder + L"\\" + data.cFileName;

		if (!wcscmp(data.cFileName, L".") || !wcscmp(data.cFileName, L".."))
			continue;

		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {	
			recursive_search(subpath); 
			continue; 
		}

		try {
			FileInfo *f = new FileInfo(subpath);

			apply_action(f);
			processed_files_count_++;
			delete f;
		}
		catch (SystemException *e) {

			wcout << L"Warning: " << e->message();
			wcout << L"Skipping file: " << subpath << endl;
			delete e;
		}

	} while (FindNextFile(handle, &data));

	FindClose(handle);
}

VOID Defragmenter::apply_action(FileInfo *f)
{

	switch (action_)
	{

	case kAnalyze:
		
		if (f->frag_count() > 1) 
		{
			if (!quiet_)
				wcout << f->fullpath() << L" is in " << f->frag_count() << L" fragments" << endl;

			frag_count_before_ += f->frag_count();

		} else 
		{
			frag_count_before_++;
			
			if (!quiet_)
				wcout << f->fullpath() << L" is defragmented" << endl;
		}

		break;

	case kDefragment:

		LONGLONG area_starting_lcn;

		if (!quiet_)
			wcout << L"Processing " << f->fullpath() << "..." << endl;
		
		if (f->frag_count() == 1)
		{
			// Do nothing if file is defragmented
			frag_count_before_++;
			frag_count_after_++;
		}
		else if (find_area_for_file(f, &area_starting_lcn))
		{
			frag_count_before_ += f->frag_count();

			f->open_for_fragmentation();
			defrag(f->handle(), area_starting_lcn);
			f->close();

			// Recalculate file frag count
			f->analyze();
			frag_count_after_ += f->frag_count();
		}
		else
		{
			if (quiet_) // Show file on error
				wcout << L"Processing " << f->fullpath() << "..." << endl;

			frag_count_before_ += f->frag_count();
			frag_count_after_ += f->frag_count();
			
			wcout << L"Not enough free contiguous space for " << f->cluster_count() << L" clusters. File keeps its " << f->frag_count() << L" fragments." << endl;
		}
 
		break;

	case kFragment:

		if (!quiet_)
			wcout << L"Processing " << f->fullpath() << "..." << endl;

		frag_count_before_ += f->frag_count();

		f->open_for_fragmentation();
		fragment(f->handle());
		f->close();

		// Recalculate file frag count
		f->analyze();
		frag_count_after_ += f->frag_count();

		break;

	default:
		break;
	}

} 


VOID Defragmenter::open_volume()
{
	wchar_t vol_name_no_ns_prefix[MAX_PATH];

	if (!GetVolumePathName(file_or_folder_.c_str(), vol_name_no_ns_prefix, MAX_PATH))
		throw new SystemException(GetLastError());

	wstring volume_name = L"\\\\?\\" + wstring(vol_name_no_ns_prefix).substr(0, 2);

	volume_handle_ = CreateFile(volume_name.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
	if (volume_handle_ == INVALID_HANDLE_VALUE)
		throw new SystemException(GetLastError());

}

VOID Defragmenter::close_volume()
{
	if (!CloseHandle(volume_handle_))
		throw new SystemException(GetLastError());

	volume_handle_ = INVALID_HANDLE_VALUE;
}

bool Defragmenter::find_area_for_file (FileInfo *f, PLONGLONG area_starting_lcn)
{
	BYTE buffer[bitmap_size];
	PVOLUME_BITMAP_BUFFER bitmap = (PVOLUME_BITMAP_BUFFER)buffer;
	DWORD nbytes = 0;

	LONGLONG lcn = 0;
	LONGLONG free_area_length = 0;
	LONGLONG free_area_starting_lcn = 0;
	LONGLONG lcn_global_counter = 0;

	ZeroMemory(buffer, sizeof(buffer));

	while (!DeviceIoControl(volume_handle_, FSCTL_GET_VOLUME_BITMAP, &lcn, sizeof(lcn), bitmap, bitmap_size, &nbytes, NULL))
	{
		DWORD i, j;
		DWORD nLogicalClusters = ((nbytes - (sizeof(LARGE_INTEGER) * 2)) * 8);  

		if (GetLastError() != ERROR_MORE_DATA)
			throw new SystemException(GetLastError());
 
		for (i = 0; i < nLogicalClusters / 8; i++)
		{
			for (j = 0; j < 8; j++)
			{
				if (bitmap->Buffer[i] & shift[j]) // Used cluster
				{
					if (free_area_length > 0)
					{
						if (f->cluster_count() <= free_area_length)
						{
							*area_starting_lcn = free_area_starting_lcn;
							return TRUE;
						}
					}

					free_area_length = 0;
				}
				else // Free cluster
				{
					if (free_area_length == 0)
						free_area_starting_lcn = lcn_global_counter;

					free_area_length++;
				}

				// inc total cluster counter
				lcn_global_counter++;
			}
		}

		// next FSCTL_GET_VOLUME_BITMAP
		lcn = bitmap->StartingLcn.QuadPart + nLogicalClusters;
	}

	return FALSE;
}

VOID Defragmenter::move_clusters(PMOVE_FILE_DATA move_data)
{
	DWORD BytesReturned = 0;

	if (!DeviceIoControl(volume_handle_, FSCTL_MOVE_FILE, move_data, sizeof(MOVE_FILE_DATA), NULL, 0, &BytesReturned, NULL))
		throw new SystemException(GetLastError());
}

bool Defragmenter::defrag(HANDLE file_handle, LONGLONG area_starting_lcn)
{
	const size_t filemap_size = sizeof(RETRIEVAL_POINTERS_BUFFER) + (2 * sizeof(LARGE_INTEGER) * 511);
	BYTE buffer[filemap_size];
	BOOLEAN finished;
	LONGLONG previous_vcn = 0;
	PRETRIEVAL_POINTERS_BUFFER retrieval_pointers = (PRETRIEVAL_POINTERS_BUFFER)buffer;
	DWORD dummy = 0, i;

	MOVE_FILE_DATA moveData;

	moveData.FileHandle = file_handle;
	moveData.StartingLcn.QuadPart = area_starting_lcn;
	moveData.StartingVcn.QuadPart = 0;
	moveData.ClusterCount = 0;

	do
	{
		ZeroMemory(buffer, sizeof(buffer));

		finished = DeviceIoControl(file_handle,
			FSCTL_GET_RETRIEVAL_POINTERS,
			&previous_vcn,
			sizeof(previous_vcn),
			retrieval_pointers,
			sizeof(buffer), &dummy, NULL);

		// ERROR_HANDLE_EOF -> file smaller than cluster
		if (!finished && GetLastError() != ERROR_MORE_DATA)
			throw new SystemException(GetLastError());

		for (i = 0; i < retrieval_pointers->ExtentCount; i++)
		{
			moveData.StartingVcn.QuadPart = previous_vcn;

			if (i == 0)
				moveData.ClusterCount = (DWORD)(retrieval_pointers->Extents[i].NextVcn.QuadPart - retrieval_pointers->StartingVcn.QuadPart);
			else
				moveData.ClusterCount = (DWORD)(retrieval_pointers->Extents[i].NextVcn.QuadPart - retrieval_pointers->Extents[i - 1].NextVcn.QuadPart);

			move_clusters(&moveData);

			moveData.StartingLcn.QuadPart = moveData.StartingLcn.QuadPart + moveData.ClusterCount;

			// update offset for next FSCTL_GET_RETRIEVAL_POINTERS iteration
			previous_vcn = retrieval_pointers->Extents[i].NextVcn.QuadPart;
		}

	} while (!finished);


	return true;
}

LONGLONG Defragmenter::get_first_free_cluster()
{
	BYTE buffer[bitmap_size];
	PVOLUME_BITMAP_BUFFER bitmap = (PVOLUME_BITMAP_BUFFER)buffer;
	DWORD nbytes = 0;
	LONGLONG lcn = 0, lcn_global_counter = 0;

	ZeroMemory(buffer, sizeof(buffer));

	while (!DeviceIoControl(volume_handle_, FSCTL_GET_VOLUME_BITMAP, &lcn, sizeof(lcn), bitmap, bitmap_size, &nbytes, NULL))
	{
		DWORD i, j;
		DWORD nLogicalClusters = ((nbytes - (sizeof(LARGE_INTEGER) * 2)) * 8);

		if (GetLastError() != ERROR_MORE_DATA)
			throw new SystemException(GetLastError());

		for (i = 0; i < nLogicalClusters / 8; i++)
			for (j = 0; j < 8; j++, lcn_global_counter++)
				if (!(bitmap->Buffer[i] & shift[j]))
					return lcn_global_counter;



		// next FSCTL_GET_VOLUME_BITMAP
		lcn = bitmap->StartingLcn.QuadPart + nLogicalClusters;
	}

	return lcn;
}

LONGLONG Defragmenter::get_last_free_cluster()
{
	BYTE buffer[bitmap_size];
	PVOLUME_BITMAP_BUFFER bitmap = (PVOLUME_BITMAP_BUFFER)buffer;
	DWORD nbytes = 0;
	LONGLONG lcn = 0;
	LONGLONG last_free_lcn = 0, lcn_global_counter = 0;

	ZeroMemory(buffer, sizeof(buffer));

	while (!DeviceIoControl(volume_handle_, FSCTL_GET_VOLUME_BITMAP, &lcn, sizeof(lcn), bitmap, bitmap_size, &nbytes, NULL))
	{
		DWORD i, j;
		DWORD nLogicalClusters = ((nbytes - (sizeof(LARGE_INTEGER) * 2)) * 8);

		if (GetLastError() != ERROR_MORE_DATA)
			throw new SystemException(GetLastError());

		for (i = 0; i < nLogicalClusters / 8; i++)
			for (j = 0; j < 8; j++, lcn_global_counter++)
				if (!(bitmap->Buffer[i] & shift[j]))
					last_free_lcn = lcn_global_counter;

		// next FSCTL_GET_VOLUME_BITMAP
		lcn = bitmap->StartingLcn.QuadPart + nLogicalClusters;
	}

	return last_free_lcn;
}


bool Defragmenter::fragment(HANDLE file_handle)
{
	const size_t filemap_size = sizeof(RETRIEVAL_POINTERS_BUFFER) + (2 * sizeof(LARGE_INTEGER) * 511);
	BYTE buffer[filemap_size];
	BOOLEAN finished;
	LONGLONG previous_vcn = 0;
	PRETRIEVAL_POINTERS_BUFFER retrieval_pointers = (PRETRIEVAL_POINTERS_BUFFER)buffer;
	DWORD dummy = 0, i, j;

	MOVE_FILE_DATA moveData;

	moveData.FileHandle = file_handle;
	moveData.StartingLcn.QuadPart = 0;
	moveData.StartingVcn.QuadPart = 0;
	moveData.ClusterCount = 1;

	do
	{
		ZeroMemory(buffer, sizeof(buffer));

		finished = DeviceIoControl(file_handle,
			FSCTL_GET_RETRIEVAL_POINTERS,
			&previous_vcn,
			sizeof(previous_vcn),
			retrieval_pointers,
			sizeof(buffer), &dummy, NULL);

		// ERROR_HANDLE_EOF -> file smaller than cluster
		if (!finished && GetLastError() != ERROR_MORE_DATA)
			throw new SystemException(GetLastError());

		for (i = 0; i < retrieval_pointers->ExtentCount; i++)
		{
			DWORD cluster_count;

			moveData.StartingVcn.QuadPart = previous_vcn;

			if (i == 0)
				cluster_count = (DWORD)(retrieval_pointers->Extents[i].NextVcn.QuadPart - retrieval_pointers->StartingVcn.QuadPart);
			else
				cluster_count = (DWORD)(retrieval_pointers->Extents[i].NextVcn.QuadPart - retrieval_pointers->Extents[i - 1].NextVcn.QuadPart);

			for (j = 0; j < cluster_count; j++)
			{

				if (j % 2 == 0)
					moveData.StartingLcn.QuadPart = get_first_free_cluster();
				else
					moveData.StartingLcn.QuadPart = get_last_free_cluster();

				move_clusters(&moveData);
				moveData.StartingVcn.QuadPart++;
			}

			// update offset for next FSCTL_GET_RETRIEVAL_POINTERS iteration
			previous_vcn = retrieval_pointers->Extents[i].NextVcn.QuadPart;
		}

	} while (!finished);


	return true;
}


Defragmenter::~Defragmenter()
{
	if (volume_handle_ != INVALID_HANDLE_VALUE)
		this->close_volume();
}
