// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#include "KamoFileHelper.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManagerGeneric.h"


// Atomic read and write.
// Outer ATOMIC functions call inner LOCKED functions with retry mechanism
// to deal with lock contention.
//
//
// The operation of the inner LOCKED functions is different based on preprocessor macros:
// We have three implementations of the actual file read/write operations.
// PASSTHROUGH:  Just use the FileHelper methods SaveStringToFile/LoadFileToString
//		directly. This is problematic because of race conditions, since the read
//		function isn't protected against simultaneous write on linux.  It is safe on
//		Windows, though, because UE4 uses proper sharing flags for read or write on files.
// MODE_LOCKED:  Use file locking.  We implement same functionality on linux and
//		windows, even though the windows UE4 method is fully locked.
//		work over NFS.  so we reimplement both read and write using proper fcntl() locking.
// MODE_RENAME:	  We write to a temporary file, then rename it.  This requires no
//		locking on linux, since the rename is atomic, but the rename operation might
//		fail on windows, if the destination file is being read at the same time.
//
// On balance, and because it offeres interoperability, the RENAME mode seems to be best.
//
// However, KamoFileDB still needs a proper locking IO function to get the .lock file for a region
// and therefore the OpenWrite() function defined here is used, which uses fcntl() locking.


#define MODE_PASSTHROUGH 0
#define MODE_LOCKED 1
#define MODE_RENAME 2

#if PLATFORM_LINUX
#define LOCK_MODE MODE_LOCKED
#else
#define LOCK_MODE MODE_PASSTHROUGH
#endif

// just use rename
#undef LOCK_MODE
#define LOCK_MODE MODE_RENAME


// Atomic retry methods.
bool FKamoFileHelper::AtomicSaveStringToFile(const FString& String, const TCHAR* Filename)
{
	bool result;
	for (int i = 0; i < 3; i++) {
		if (i > 0)
		{
			// short sleep before retrying
			Nap();
		}
		result = SaveStringToLockedFile(String, Filename);
		if (!result) {
			if (!WasSharingViolation())
			{
				break;
			}
		}
		else
		{
			return result;
		}
	}
	return result;
}

bool FKamoFileHelper::AtomicLoadFileToString(FString& Result, const TCHAR* Filename)
{
	bool result;
	for (int i = 0; i < 3; i++) {
		if (i > 0)
		{
			// short sleep before retrying
			Nap();
		}
		result = LoadLockedFileToString(Result, Filename);
		if (!result) {
			if (!WasSharingViolation())
			{
				break;
			}
		}
		else
		{
			return result;
		}
	}
	return result;
}


#if LOCK_MODE == MODE_PASSTHROUGH

bool FKamoFileHelper::SaveStringToLockedFile(const FString& String, const TCHAR* Filename)
{
	return FFileHelper::SaveStringToFile(String, Filename, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
bool FKamoFileHelper::LoadLockedFileToString(FString& Result, const TCHAR* Filename)
{
	return FFileHelper::LoadFileToString(Result, Filename);
}

#elif LOCK_MODE == MODE_LOCKED

// On windows, FFileHelper properly locks files for reading and writing so that there are no
// sharing conflicts or overwrites.  On unix, though, only OpenWrite() uses flock(), (not OpenRead())
// and flock() itself does not work on nfs drives.  So, we reimplement the whole thing here.
// using fcntl() locking, which does work on unix.
// On Windows, the file handle uses the windows UE implementation underneath.

bool FKamoFileHelper::SaveStringToLockedFile(const FString& String, const TCHAR* Filename)
{
	TUniquePtr<IKamoFileHandle> WriteHandle(OpenWrite(Filename, false, false));
	if (!WriteHandle)
	{
		return false;
	}
	
	if (String.IsEmpty())
		return true;

	// Support only utf8
	FTCHARToUTF8 UTF8String(*String, String.Len());
	return WriteHandle->Write((UTF8CHAR*)UTF8String.Get(), UTF8String.Length() * sizeof(UTF8CHAR));
}

// Unfortunately, the linux platform functions for OpenRead _do not_ in fact use flock.
// this opens us up for a race condition when reading a file that is simultaneously
// being written.  This function is copied from FFileHelper but with a custom Reader
// We implement same functionality for windows AND linux, to aid in cross compilation.
bool FKamoFileHelper::LoadLockedFileToString(FString& Result, const TCHAR* Filename)
{
	TUniquePtr<IKamoFileHandle> ReadHandle(OpenRead(Filename, false));
	if (!ReadHandle)
	{
		return false;
	}

	int32 Size = ReadHandle->Size();
	if (!Size)
	{
		Result.Empty();
		return true;
	}

	uint8* Ch = (uint8*)FMemory::Malloc(Size);
	bool Success = ReadHandle->Read(Ch, Size);
	ReadHandle = nullptr;
	if (Success)
	{
		FFileHelper::BufferToString(Result, Ch, Size);
	}

	// free manually since not running SHA task
	FMemory::Free(Ch);
	return Success;
}

#elif LOCK_MODE == MODE_RENAME

// different implementation, achieves atomicity by writing files and doing atomic renames.
// However, on windows, this still can cause a writer to get a sharing violation when the rename is tried.
// also, it is more complex than just using the locking primitives provided by the system.


// there is no atomic read as such, the entire file is read if opened.
bool FKamoFileHelper::LoadLockedFileToString(FString& Result, const TCHAR* Filename)
{
	return FFileHelper::LoadFileToString(Result, Filename);
}


bool FKamoFileHelper::SaveStringToLockedFile(const FString& String, const TCHAR* Filename)
{
	FString tmp = GetTempFileName(Filename);
	bool ok = FFileHelper::SaveStringToFile(String, *tmp, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	const bool overwrite = true;
	if (ok) {
		if (overwrite)
		{
			ok = ReplaceAtomic(Filename, *tmp);
		}
		else
		{
			ok = MoveAtomic(Filename, *tmp);
		}
		if (ok)
		{
			return ok;
		}
		auto& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		PlatformFile.DeleteFile(*tmp);
	}
	return ok;
}

// construct a temporary file relative to a target file.
// if the filename ends with foo.json, the tmp file name will be foo.json.xxxx.tmp
FString FKamoFileHelper::GetTempFileName(const TCHAR* Filename)
{
	FString path = FPaths::GetPath(Filename);
	FString file = FPaths::GetCleanFilename(Filename);
	return FPaths::CreateTempFilename(*path, *file);
}

#endif

// Platform specific functions
// 
// wrapper for IFileHandle
class FKamoFileHandleWrap : public IKamoFileHandle
{
public:
	FKamoFileHandleWrap(IFileHandle* _wrapped)
		: wrapped(_wrapped)
	{}
	virtual int64 Size() override
	{
		return wrapped->Size();
	}
	virtual bool Seek(int64 NewPosition) override
	{
		return wrapped->Seek(NewPosition);
	}
	virtual bool SeekFromEnd(int64 NewPositionRelativeToEnd)
	{
		return wrapped->SeekFromEnd(NewPositionRelativeToEnd);
	}
	virtual bool Read(uint8* Destination, int64 BytesToRead) override
	{
		return wrapped->Read(Destination, BytesToRead);
	}
	virtual bool Write(const uint8* Source, int64 BytesToWrite) override
	{
		return wrapped->Write(Source, BytesToWrite);
	}
private:
	TUniquePtr<IFileHandle> wrapped;
};

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <WinBase.h>
#include "Windows/HideWindowsPlatformTypes.h"


class FKamoFileHandle : public IKamoFileHandle
{
	enum { READWRITE_SIZE = 1024 * 1024 };
	HANDLE FileHandle;
	/** The overlapped IO struct to use for determining async state	*/
	OVERLAPPED OverlappedIO;
	/** Manages the location of our file position for setting on the overlapped struct for reads/writes */
	int64 FilePos;
	/** Need the file size for seek from end */
	int64 FileSize;

	FORCEINLINE bool IsValid()
	{
		return FileHandle != NULL && FileHandle != INVALID_HANDLE_VALUE;
	}

	FORCEINLINE void UpdateOverlappedPos()
	{
		ULARGE_INTEGER LI;
		LI.QuadPart = FilePos;
		OverlappedIO.Offset = LI.LowPart;
		OverlappedIO.OffsetHigh = LI.HighPart;
	}

	FORCEINLINE void UpdateFileSize()
	{
		LARGE_INTEGER LI;
		GetFileSizeEx(FileHandle, &LI);
		FileSize = LI.QuadPart;
	}

public:
	FKamoFileHandle(HANDLE InFileHandle = NULL)
		: FileHandle(InFileHandle)
		, FilePos(0)
		, FileSize(0)
	{
		if (IsValid())
		{
			UpdateFileSize();
		}
		FMemory::Memzero(&OverlappedIO, sizeof(OVERLAPPED));
	}
	virtual ~FKamoFileHandle()
	{
		CloseHandle(FileHandle);
		FileHandle = NULL;
	}
	virtual int64 Size() override
	{
		check(IsValid());
		return FileSize;
	}

	virtual bool Read(uint8* Destination, int64 BytesToRead) override
	{
		check(IsValid());
		// Now kick off an async read

		int64 TotalNumRead = 0;
		do
		{
			uint32 BytesToRead32 = FMath::Min(BytesToRead, int64(UINT32_MAX));
			uint32 NumRead = 0;

			if (!ReadFile(FileHandle, Destination, BytesToRead32, (::DWORD*)&NumRead, &OverlappedIO))
			{
				uint32 ErrorCode = GetLastError();
				if (ErrorCode != ERROR_IO_PENDING)
				{
					// Read failed
					return false;
				}
				// Wait for the read to complete
				NumRead = 0;
				if (!GetOverlappedResult(FileHandle, &OverlappedIO, (::DWORD*)&NumRead, true))
				{
					// Read failed
					return false;
				}
			}

			BytesToRead -= BytesToRead32;
			Destination += BytesToRead32;
			TotalNumRead += NumRead;
			// Update where we are in the file
			FilePos += NumRead;
			UpdateOverlappedPos();
		} while (BytesToRead > 0);
		return true;
	}
};

#if 0
IKamoFileHandle* FKamoFileHelper::OpenRead(const TCHAR* Filename, bool bAllowWrite = false)
{
	uint32  Access = GENERIC_READ;
	uint32  WinFlags = FILE_SHARE_READ | (bAllowWrite ? FILE_SHARE_WRITE : 0);
	uint32  Create = OPEN_EXISTING;

	HANDLE Handle = CreateFileW(*NormalizeFilename(Filename), Access, WinFlags, NULL, Create, FILE_ATTRIBUTE_NORMAL, NULL);
	if (Handle != INVALID_HANDLE_VALUE)
	{
		return new FKamoFileHandle(Handle);
	}
	return NULL;
}
#else
IKamoFileHandle* FKamoFileHelper::OpenRead(const TCHAR* Filename, bool bAllowWrite)
{
	auto& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	IFileHandle *fh = PlatformFile.OpenRead(Filename, bAllowWrite);
	return fh ? new FKamoFileHandleWrap(fh) : nullptr;
}
#endif

IKamoFileHandle* FKamoFileHelper::OpenWrite(const TCHAR* Filename, bool bAppend, bool bAllowRead)
{
	auto& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	IFileHandle *fh = PlatformFile.OpenWrite(Filename, bAppend, bAllowRead);
	return fh ? new FKamoFileHandleWrap(fh) : nullptr;
}


void FKamoFileHelper::Nap()
{
	Sleep(0);
}

bool FKamoFileHelper::WasSharingViolation()
{
	return GetLastError() == ERROR_SHARING_VIOLATION;
}

// taken from WindowsPlatformFile
FString FKamoFileHelper::NormalizeFilename(const TCHAR* Filename)
{
	FString Result(Filename);
	FPaths::NormalizeFilename(Result);
	if (Result.StartsWith(TEXT("//")))
	{
		Result = FString(TEXT("\\\\")) + Result.RightChop(2);
	}
	return FPaths::ConvertRelativePathToFull(Result);
}

// This one fails if the destination exists. 
// MoveFile doesn't behave that way on linux (it uses rename() on linux, MoveFileW on windows)
bool FKamoFileHelper::MoveAtomic(const TCHAR *To, const TCHAR *From)
{
	auto& pf = FPlatformFileManager::Get().GetPlatformFile();
	return pf.MoveFile(To, From);
}

// this one replaces the destination, but there will be a sharing violation if the destination is being
// read at the same time.
bool FKamoFileHelper::ReplaceAtomic(const TCHAR *To, const TCHAR *From)
{
	return MoveFileExW(*NormalizeFilename(From), *NormalizeFilename(To), MOVEFILE_REPLACE_EXISTING);
}


#elif PLATFORM_LINUX

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

/** simple file handle for reading
 * allows us to try this both for windows and linux
 */

class FKamoFileHandle : public IKamoFileHandle
{
	enum { READWRITE_SIZE = SSIZE_MAX };

	bool IsValid()
	{
		return FileHandle != -1;
	}

public:
	FKamoFileHandle(int32 InFileHandle, bool InFileOpenAsWrite)
		: FileHandle(InFileHandle), FileOpenAsWrite(InFileOpenAsWrite)
	{
		check(FileHandle > -1);
	}

	virtual ~FKamoFileHandle()
	{
		close(FileHandle);
		FileHandle = -1;
	}

private:
	
	int64 ReadInternal(uint8* Destination, int64 BytesToRead)
	{
		check(IsValid());
		int64 BytesRead = 0;
		while (BytesToRead)
		{
			check(BytesToRead >= 0);
			int64 ThisSize = FMath::Min<int64>(READWRITE_SIZE, BytesToRead);
			check(Destination);
			int64 ThisRead = read(FileHandle, Destination, ThisSize);
			if (ThisRead == -1 && errno == EINTR)
			{
				continue;
			}
			BytesRead += ThisRead;
			if (ThisRead != ThisSize)
			{
				return BytesRead;
			}
			Destination += ThisSize;
			BytesToRead -= ThisSize;
		}
		return BytesRead;
	}
public:

	virtual int64 Size() override
	{
		struct stat FileInfo;
		fstat(FileHandle, &FileInfo);
		return FileInfo.st_size;
	}

	virtual bool Seek(int64 NewPosition) override
	{
		check(NewPosition >= 0);
		check(IsValid());
		return lseek(FileHandle, NewPosition, SEEK_SET) != -1;
	}

	virtual bool SeekFromEnd(int64 NewPositionRelativeToEnd) override
	{
		check(NewPositionRelativeToEnd <= 0);
		check(IsValid());
		return lseek(FileHandle, NewPositionRelativeToEnd, SEEK_END) != -1;
	}

	virtual bool Read(uint8* Destination, int64 BytesToRead) override
	{
		return ReadInternal(Destination, BytesToRead) == BytesToRead;
	}

	virtual bool Write(const uint8* Source, int64 BytesToWrite) override
	{
		check(IsValid());
		check(FileOpenAsWrite);
		while (BytesToWrite)
		{
			check(BytesToWrite >= 0);
			int64 ThisSize = FMath::Min<int64>(READWRITE_SIZE, BytesToWrite);
			check(Source);
			if (write(FileHandle, Source, ThisSize) != ThisSize)
			{
				return false;
			}
			Source += ThisSize;
			BytesToWrite -= ThisSize;
		}
		return true;
	}

private:
	// Holds the internal file handle.
	int32 FileHandle = -1;
	bool FileOpenAsWrite;
};

// This is based on FUnixPlatformFile::OpenRead()
// but uses proper fcntl locking to ensure that a file isn't overwritten while being read.  This is NFS compatible
IKamoFileHandle* FKamoFileHelper::OpenRead(const TCHAR* Filename, bool bAllowWrite)
{
	int Flags = O_CLOEXEC;	// prevent children from inheriting this

	Flags |= O_RDONLY;

	int32 Handle = open(TCHAR_TO_UTF8(*NormalizeFilename(Filename)), Flags, 0);
	if (Handle != -1)
	{
		if (!bAllowWrite)
		{
			struct flock fl;
			memset(&fl, 0, sizeof(fl));
			fl.l_type = F_RDLCK; // a read lock
			fl.l_whence = SEEK_SET;
			// mimic Windows "shared read" behavior (we use FILE_SHARE_READ) by locking the file.
			// note that the (non-mandatory) "lock" will be removed by itself when the last file descriptor is close()d
			if (fcntl(Handle, F_SETLK, &fl) == -1)
			{
				// if locked, consider operation a failure
				if (EAGAIN == errno || EWOULDBLOCK == errno)
				{
					close(Handle);
					return nullptr;
				}
				// all the other locking errors are ignored.
			}
		}

		FKamoFileHandle* FileHandleRead = new FKamoFileHandle(Handle, false);

		return FileHandleRead;
	}

	//int ErrNo = errno;
	//UE_LOG_UNIX_FILE(Warning, TEXT("open('%s', Flags=0x%08X) failed: errno=%d (%s)"), *NormalizeFilename(Filename), Flags, ErrNo, UTF8_TO_TCHAR(strerror(ErrNo)));
	return nullptr;
}


// Open a file for exclusive write.  Used for lock files.
IKamoFileHandle* FKamoFileHelper::OpenWrite(const TCHAR* Filename, bool bAppend, bool bAllowRead)
{
	int Flags = O_CREAT | O_CLOEXEC;
	if (bAllowRead)
	{
		Flags |= O_RDWR;
	}
	else
	{
		Flags |= O_WRONLY;
	}

	int32 Handle = open(TCHAR_TO_UTF8(*NormalizeFilename(Filename)), Flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (Handle != -1)
	{
		struct flock fl;
		memset(&fl, 0, sizeof(fl));
		fl.l_type = F_WRLCK;
		fl.l_whence = SEEK_SET;
		// use fcntl to get exclusive lock that works even over NFS
		if (fcntl(Handle, F_SETLK, &fl) == -1)
		{
			// if locked, consider operation a failure
			if (EAGAIN == errno || EWOULDBLOCK == errno)
			{
				close(Handle);
				return nullptr;
			}
			// all the other locking errors are ignored.
		}
		// truncate the file now that we locked it
		if (!bAppend)
		{
			if (ftruncate(Handle, 0) != 0)
			{
				int ErrNo = errno;
				//UE_LOG_UNIX_FILE(Warning, TEXT("ftruncate() failed for '%s': errno=%d (%s)"),
				//	Filename, ErrNo, UTF8_TO_TCHAR(strerror(ErrNo)));
				close(Handle);
				return nullptr;
			}
		}

		FKamoFileHandle* FileHandle = new FKamoFileHandle(Handle, true);

		if (bAppend)
		{
			FileHandle->SeekFromEnd(0);
		}
		return FileHandle;
	}

	//int ErrNo = errno;
	//UE_LOG_UNIX_FILE(Warning, TEXT("open('%s', Flags=0x%08X) failed: errno=%d (%s)"), *NormalizeFilename(Filename), Flags, ErrNo, UTF8_TO_TCHAR(strerror(ErrNo)));
	return nullptr;
}


// taken from UnixPlatformFile.cpp
FString FKamoFileHelper::NormalizeFilename(const TCHAR* Filename)
{
	FString Result(Filename);
	FPaths::NormalizeFilename(Result);
	return FPaths::ConvertRelativePathToFull(Result);
}

// On linux, to avoid replacing a destination file, use link() and unlink()
bool FKamoFileHelper::MoveAtomic(const TCHAR *To, const TCHAR *From)
{
#ifdef USE_FANCY_PANTS_MOVE
	auto to8 = TCHAR_TO_UTF8(*NormalizeFilename(To));
	auto from8 = TCHAR_TO_UTF8(*NormalizeFilename(From));

	int32 Result = link(from8, to8);
	if (Result == 0) {
		unlink(from8);
		return true;
	}
	return false;
#else
	//  A small experiment
	auto& pf = FPlatformFileManager::Get().GetPlatformFile();
	return pf.MoveFile(To, From);
#endif
}

// Unix implementation of IPlatformFile::MoveFile uses rename() which actually will replace a destination file
// This is contrary to the interface documentation of MoveFile
bool FKamoFileHelper::ReplaceAtomic(const TCHAR *To, const TCHAR *From)
{
	auto& pf = FPlatformFileManager::Get().GetPlatformFile();
	return pf.MoveFile(To, From);
}

bool FKamoFileHelper::WasSharingViolation()
{
	return EAGAIN == errno || EWOULDBLOCK == errno;
}
void FKamoFileHelper::Nap()
{
	sleep(0);
}

#else // PLATFORM_LINUX

// All the other platforms that's not Linux or Windows
IKamoFileHandle* FKamoFileHelper::OpenRead(const TCHAR* Filename, bool bAllowWrite)
{
	auto& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	IFileHandle* fh = PlatformFile.OpenRead(Filename, bAllowWrite);
	return fh ? new FKamoFileHandleWrap(fh) : nullptr;
}

IKamoFileHandle* FKamoFileHelper::OpenWrite(const TCHAR* Filename, bool bAppend, bool bAllowRead)
{
	auto& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	IFileHandle* fh = PlatformFile.OpenWrite(Filename, bAppend, bAllowRead);
	return fh ? new FKamoFileHandleWrap(fh) : nullptr;
}

bool FKamoFileHelper::WasSharingViolation()
{
	return false;
}

void FKamoFileHelper::Nap()
{
	FPlatformProcess::Sleep(0);
}

bool FKamoFileHelper::MoveAtomic(const TCHAR* To, const TCHAR* From)
{
	auto& pf = FPlatformFileManager::Get().GetPlatformFile();
	return pf.MoveFile(To, From);
}

bool FKamoFileHelper::ReplaceAtomic(const TCHAR* To, const TCHAR* From)
{
	auto& pf = FPlatformFileManager::Get().GetPlatformFile();
	return pf.MoveFile(To, From);
}

#endif
