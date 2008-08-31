/* WinKexec: kexec for Windows
 * Copyright (C) 2008 John Stumpo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Some routines to work with PE files from kernel mode.
   kexec.sys uses these to change ntoskrnl.exe's idea of where
   hal.dll's HalReturnToFirmware() function is.  Feel free to
   use these in your own drivers too. */

#include <ddk/ntddk.h>
#include "KexecDriverPe.h"

/* M$ makes you declare it yourself... */
int _snwprintf(PWCHAR buffer, size_t count, const PWCHAR format, ...);

/* Read the entirety of a file from system32 into a nonpaged buffer.
   Returns NULL on error.  If a buffer is returned, you must
   call ExFreePool() on it when you are finished with it. */
PVOID KexecPeReadSystemFile(PCHAR Filename)
{
  HANDLE FileHandle;
  NTSTATUS status;
  OBJECT_ATTRIBUTES ObjectAttributes;
  WCHAR FullFilenameBuffer[256];
  UNICODE_STRING FullFilename;
  IO_STATUS_BLOCK StatusBlock;
  PVOID FileReadBuffer;
  FILE_STANDARD_INFORMATION FileInfo;

  /* More black magic - \SystemRoot goes to the Windows install root. */
  _snwprintf(FullFilenameBuffer, 255, L"\\SystemRoot\\system32\\%s", Filename);
  RtlInitUnicodeString(&FullFilename, FullFilenameBuffer);

  InitializeObjectAttributes(&ObjectAttributes, &FullFilename, 0, NULL, NULL);

  /* Open the file. */
  status = ZwOpenFile(&FileHandle, GENERIC_READ, &ObjectAttributes,
    &StatusBlock, FILE_SHARE_READ, FILE_NON_DIRECTORY_FILE);
  if (!NT_SUCCESS(status))
    return NULL;

  /* Figure out how long it is. */
  status = ZwQueryInformationFile(FileHandle, &StatusBlock, &FileInfo,
    sizeof(FileInfo), FileStandardInformation);
  if (!NT_SUCCESS(status))
    return NULL;

  /* Make sure value is sane. */
  if (FileInfo.EndOfFile.HighPart)
    return NULL;

  /* Grab a buffer for the file. */
  FileReadBuffer = ExAllocatePoolWithTag(NonPagedPool,
    FileInfo.EndOfFile.LowPart, TAG('K', 'x', 'e', 'c'));
  if (!FileReadBuffer)
    return NULL;

  /* Read the file in. */
  status = ZwReadFile(FileHandle, NULL, NULL, NULL, &StatusBlock,
    FileReadBuffer, FileInfo.EndOfFile.LowPart, NULL, NULL);
  if (!NT_SUCCESS(status)) {
    ExFreePool(FileReadBuffer);
    return NULL;
  }

  /* Close the file. */
  status = ZwClose(FileHandle);
  if (!NT_SUCCESS(status)) {
    ExFreePool(FileReadBuffer);
    return NULL;
  }

  return FileReadBuffer;
}

/* Find the offset to an exported function in a loaded PE file. */
PVOID KexecPeGetExportedFunction(PCHAR Filename, PCHAR FunctionName)
{
  PVOID FileContents;
  PIMAGE_NT_HEADERS PeHeader;
  PVOID ExportDirectory;

  /* Read the PE file from disk. */
  FileContents = KexecPeReadSystemFile(Filename);
  if (!FileContents)
    return NULL;

  /* Verify the MZ magic number. */
  if (((PIMAGE_DOS_HEADER)FileContents)->e_magic != 0x5a4d) {
    ExFreePool(FileContents);
    return NULL;
  }

  /* Get to the PE header.
     Verify the magic number ("PE\0\0") while we're here. */
  PeHeader = FileContents + ((PIMAGE_DOS_HEADER)FileContents)->e_lfanew;
  if (PeHeader->Signature != 0x00004550)
  {
    ExFreePool(FileContents);
    return NULL;
  }

  ExportDirectory = &PeHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
}
