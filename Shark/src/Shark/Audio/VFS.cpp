#include "skpch.h"
#include "VFS.h"

#include "Shark/Core/Application.h"
#include "Shark/Core/Project.h"
#include "Shark/File/Serialization/FileStream.h"

#include <miniaudio/miniaudio.h>

namespace Shark::Audio {

	static StreamReader* CreateReaderFor(const char* filepath)
	{
		StreamReader* stream = nullptr;

		// #TODO #audio #runtime support runtime
		// implement reader support for asset manager?

		if (!Application::Get().GetSpecification().IsRuntime)
		{
			auto handle = AssetHandle::Make(std::stoull(filepath));
			auto filesystemPath = Project::GetEditorAssetManager()->GetFilesystemPath(handle);

			Scope fileReader = sknew FileStreamReader(filesystemPath);
			if (fileReader->IsStreamGood())
			{
				stream = fileReader.Detach();
				SK_CORE_TRACE_TAG("Audio", "Created Reader for '{}' ({})", filepath, filesystemPath);
			}
		}

		return stream;
	}

	ma_result VfsOpen(ma_vfs* pVFS, const char* pFilePath, ma_uint32 openMode, ma_vfs_file* pFile)
	{
		if (openMode == MA_OPEN_MODE_WRITE)
			return ma_result::MA_NOT_IMPLEMENTED;

		auto stream = CreateReaderFor(pFilePath);
		if (!stream || !stream->IsStreamGood())
			return MA_ERROR;

		SK_CORE_TRACE_TAG("Audio", "Opened file {}", static_cast<FileStreamReader*>(stream)->GetFilepath());
		*pFile = stream;
		return MA_SUCCESS;
	}

	//ma_result VfsOpen_w(ma_vfs* pVFS, const wchar_t* pFilePath, ma_uint32 openMode, ma_vfs_file* pFile);

	ma_result VfsClose(ma_vfs* pVFS, ma_vfs_file file)
	{
		auto stream = static_cast<StreamReader*>(file);
		SK_CORE_TRACE_TAG("Audio", "Closed file {}", static_cast<FileStreamReader*>(stream)->GetFilepath());
		delete stream;
		return MA_SUCCESS;
	}

	ma_result VfsRead(ma_vfs* pVFS, ma_vfs_file file, void* pDst, size_t sizeInBytes, size_t* pBytesRead)
	{
		auto stream = static_cast<StreamReader*>(file);

		uint64_t bytesRead;
		if (!stream->ReadData(pDst, sizeInBytes, bytesRead))
			return MA_ERROR;

		if (pBytesRead)
			*pBytesRead = bytesRead;

		//SK_CORE_TRACE_TAG("Audio", "Read {} bytes from {}", bytesRead, static_cast<FileStreamReader*>(stream)->GetFilepath());
		if (bytesRead < sizeInBytes)
			return MA_AT_END;
		return MA_SUCCESS;
	}

	//ma_result VfsWrite(ma_vfs* pVFS, ma_vfs_file file, const void* pSrc, size_t sizeInBytes, size_t* pBytesWritten);

	ma_result VfsSeek(ma_vfs* pVFS, ma_vfs_file file, ma_int64 offset, ma_seek_origin origin)
	{
		auto stream = static_cast<StreamReader*>(file);

		switch (origin)
		{
			default: return MA_INVALID_ARGS;

			case ma_seek_origin_start:   stream->SetStreamPosition(offset);						 break;
			case ma_seek_origin_current: stream->SetStreamPosition(offset, SeekOrigin::Current); break;
			case ma_seek_origin_end:     stream->SetStreamPosition(offset, SeekOrigin::End);	 break;
		}

		return MA_SUCCESS;
	}

	ma_result VfsTell(ma_vfs* pVFS, ma_vfs_file file, ma_int64* pCursor)
	{
		auto stream = static_cast<StreamReader*>(file);

		*pCursor = stream->GetStreamPosition();
		return MA_SUCCESS;
	}

	ma_result VfsInfo(ma_vfs* pVFS, ma_vfs_file file, ma_file_info* pInfo)
	{
		auto stream = static_cast<StreamReader*>(file);

		const auto position = stream->GetStreamPosition();

		stream->SetStreamPosition(0, SeekOrigin::End);
		pInfo->sizeInBytes = stream->GetStreamPosition();
		stream->SetStreamPosition(position);
		return MA_SUCCESS;
	}

	void InitailizeVFS(ma_vfs_callbacks* vfs)
	{
		vfs->onOpen = &VfsOpen;
		//vfs->onOpenW = &VfsOpenW;
		vfs->onClose = &VfsClose;
		vfs->onRead = &VfsRead;
		//vfs->onWrite = &VfsWrite;
		vfs->onSeek = &VfsSeek;
		vfs->onTell = &VfsTell;
		vfs->onInfo = &VfsInfo;
	}

}
