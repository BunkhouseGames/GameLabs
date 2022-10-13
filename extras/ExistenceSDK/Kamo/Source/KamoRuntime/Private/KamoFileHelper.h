// Copyright 2019-2021 Directive Games, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"

// A file helper which makes reads and writes from a file atomic, as long as these apis are used for acces.
// See .cpp file for implementation details.

// Subset of the IFileHandle interface used internally by UE.  Use this to keep the code here similar
// to what UE4 does.
class IKamoFileHandle
{
public:
	virtual ~IKamoFileHandle() {};
	virtual int64 Size() = 0;
	virtual bool Seek(int64 NewPosition) = 0;
	virtual bool SeekFromEnd(int64 NewPositionRelativeToEnd = 0) = 0;
	virtual bool Read(uint8* Destination, int64 BytesToRead) = 0;
	virtual bool Write(const uint8* Source, int64 BytesToWrite) = 0;
};


struct FKamoFileHelper
{
	// atomic save and load of string to/from file.
	static bool AtomicSaveStringToFile(const FString& String, const TCHAR* Filename);
	static bool AtomicLoadFileToString(FString& Result, const TCHAR* Filename);

	// platform dependent files
	// Open shared reaqd file
	static IKamoFileHandle* OpenRead(const TCHAR* Filename, bool bAllowWrite);
	// Open an exclusively locked file.
	static IKamoFileHandle* OpenWrite(const TCHAR* Filename, bool bAppend, bool bAllowRead);

private:
	// Functions with different implementation based on the MODE that we operate in
	// read and write functions without retry on sharing collision
	static bool SaveStringToLockedFile(const FString& String, const TCHAR* Filename);
	static bool LoadLockedFileToString(FString& Result, const TCHAR* Filename);
	static FString GetTempFileName(const TCHAR* Filename);
	static bool KamoLoadFileToString(FString& Result, const TCHAR* Filename);

	// functions with platform specific implementation
	static FString NormalizeFilename(const TCHAR* Filename);
	static bool WasSharingViolation();
	static void Nap();
	
	static bool MoveAtomic(const TCHAR *To, const TCHAR *From);
	static bool ReplaceAtomic(const TCHAR *To, const TCHAR *From);
};