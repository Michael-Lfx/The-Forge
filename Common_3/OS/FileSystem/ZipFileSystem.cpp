///*
// * Copyright (c) 2018-2021 The Forge Interactive Inc.
// *
// * This file is part of The-Forge
// * (see https://github.com/ConfettiFX/The-Forge).
// *
// * Licensed to the Apache Software Foundation (ASF) under one
// * or more contributor license agreements.  See the NOTICE file
// * distributed with this work for additional information
// * regarding copyright ownership.  The ASF licenses this file
// * to you under the Apache License, Version 2.0 (the
// * "License"); you may not use this file except in compliance
// * with the License.  You may obtain a copy of the License at
// *
// *   http://www.apache.org/licenses/LICENSE-2.0
// *
// * Unless required by applicable law or agreed to in writing,
// * software distributed under the License is distributed on an
// * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// * KIND, either express or implied.  See the License for the
// * specific language governing permissions and limitations
// * under the License.
//*/

#include "../../ThirdParty/OpenSource/zip/zip.h"

#include "../Interfaces/ILog.h"
#include "../Interfaces/IMemory.h"

static bool ZipOpen(IFileSystem* pIO, const ResourceDirectory resourceDir, const char* fileName, FileMode mode, FileStream* pOut)
{
	ASSERT(pIO && pOut);
	// #TODO: Write to zip

	zip_t* zip = (zip_t*)pIO->pUser;
	char filePath[FS_MAX_PATH] = {};
	fsAppendPathComponent(fsGetResourceDirectory(resourceDir), fileName, filePath);

	int error = zip_entry_open(zip, filePath);
	if (error)
	{
		LOGF(LogLevel::eINFO, "Error %i finding file %s for opening in zip: %s", error, fileName, fileName);
		return NULL;
	}

	// Extract the contents of the zip entry
	ssize_t uncompressedSize = zip_entry_size(zip);
	if (uncompressedSize && (mode & FM_READ))
	{
		void* uncompressed = tf_malloc(uncompressedSize);
		ssize_t bytesRead = zip_entry_noallocread(zip, uncompressed, uncompressedSize);
		UNREF_PARAM(bytesRead);
		ASSERT(bytesRead == uncompressedSize);
		zip_entry_close(zip);
		return fsOpenStreamFromMemory(uncompressed, uncompressedSize, mode, true, pOut);
	}
	else
	{
		pOut->pIO = pIO;
		pOut->mMemory.mCursor = 0;
		pOut->mMemory.pBuffer = NULL;
		pOut->mMemory.mOwner = true;
		pOut->mMode = mode;
		pOut->mSize = 0;
		pOut->pUser = pIO->pUser;
	}
	
	return true;
}

static bool ZipClose(FileStream* pFile)
{
	ASSERT(pFile);
	zip_entry_close((zip_t*)pFile->pUser);
	return true;
}

static size_t ZipWrite(FileStream* pFile, const void* sourceBuffer, size_t byteCount)
{
	ASSERT(pFile);
	int error = zip_entry_write((zip_t*)pFile->pUser, sourceBuffer, byteCount);
	if (error)
	{
		LOGF(LogLevel::eERROR, "Error %i writing file in a zip", error);
	}
	return error;
}

// #NOTE - Only one function needed for zip file IO as we unzip the zip entry when it is opened and treat it as memory buffer
// More will be needed if we want to support zip write
static IFileSystem gZipFileIO =
{
	ZipOpen,
	ZipClose,
	NULL,
	ZipWrite
};

bool fsOpenZipFile(const ResourceDirectory resourceDir, const char* fileName, FileMode mode, IFileSystem* pOut)
{
	char zipMode = 0;

	if (mode & FM_WRITE)
	{
		zipMode = 'w';
	}
	else if (mode & FM_READ)
	{
		zipMode = 'r';
	}
	else
	{
		zipMode = 'a';
	}

	zip_t* zipFile = zip_open(resourceDir, fileName, ZIP_DEFAULT_COMPRESSION_LEVEL, zipMode);

	if (!zipFile)
	{
		LOGF(LogLevel::eERROR, "Error creating file system from zip file at %s", fileName);
		return false;
	}

	IFileSystem system = gZipFileIO;
	system.pUser = zipFile;
	*pOut = system;

	return true;
}

bool fsCloseZipFile(IFileSystem* pZip)
{
	ASSERT(pZip);
	zip_close((zip_t*)pZip->pUser);
	return true;
}

int fsEntryCountZipFile(IFileSystem* pZip)
{
	ASSERT(pZip);
	return zip_total_entries((zip_t*)pZip->pUser);
}

bool fsOpenZipEntryByIndex(IFileSystem* pZip, int index)
{
	ASSERT(pZip);
	return !zip_entry_openbyindex((zip_t*)pZip->pUser, index);
}

bool fsCloseZipEntry(IFileSystem* pZip)
{
	ASSERT(pZip);
	return !zip_entry_close((zip_t*)pZip->pUser);
}

const char* fsGetZipEntryName(IFileSystem* pZip)
{
	ASSERT(pZip);
	return zip_entry_name((zip_t*)pZip->pUser);
}
