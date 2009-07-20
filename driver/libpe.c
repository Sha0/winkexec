/* libpe: Small library to do interesting things with PE
 *   executables from kernel mode under Windows.
 * Originally developed as part of WinKexec: kexec for Windows
 * Copyright (C) 2008-2009 John Stumpo
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
 *
 * This file is available under a proprietary license as well.
 * Contact John Stumpo <stump@jstump.com> for details.
 */

/* Some routines to work with PE files from kernel mode.
   kexec.sys uses these to change ntoskrnl.exe's idea of where
   hal.dll's HalReturnToFirmware() function is.  Feel free to
   use these in your own drivers too. */

#include "libpe.h"

/* M$ makes you declare it yourself... */
int _snwprintf(PWCHAR buffer, size_t count, const PWCHAR format, ...);

/* Read the entirety of a file from system32 into a nonpaged buffer.
   Returns NULL on error.  If a buffer is returned, you must
   call ExFreePool() on it when you are finished with it. */
PVOID PeReadSystemFile(PWCHAR Filename)
{
  HANDLE FileHandle;
  NTSTATUS status;
  OBJECT_ATTRIBUTES ObjectAttributes;
  WCHAR FullFilenameBuffer[256];
  UNICODE_STRING FullFilename;
  IO_STATUS_BLOCK StatusBlock;
  PVOID FileReadBuffer;
  FILE_STANDARD_INFORMATION FileInfo;
  LARGE_INTEGER FilePointer;

  /* More black magic - \SystemRoot goes to the Windows install root. */
  _snwprintf(FullFilenameBuffer, 255, L"\\SystemRoot\\system32\\%s", Filename);
  RtlInitUnicodeString(&FullFilename, FullFilenameBuffer);

  InitializeObjectAttributes(&ObjectAttributes, &FullFilename, 0, NULL, NULL);

  /* Open the file. */
  status = ZwOpenFile(&FileHandle, GENERIC_READ, &ObjectAttributes, &StatusBlock,
    FILE_SHARE_READ, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
  if (!NT_SUCCESS(status))
    return NULL;

  /* Figure out how long it is. */
  status = ZwQueryInformationFile(FileHandle, &StatusBlock, &FileInfo,
    sizeof(FileInfo), FileStandardInformation);
  if (!NT_SUCCESS(status)) {
    ZwClose(FileHandle);
    return NULL;
  }

  /* Make sure value is sane. */
  if (FileInfo.EndOfFile.HighPart) {
    ZwClose(FileHandle);
    return NULL;
  }

  /* Grab a buffer for the file. */
  FileReadBuffer = ExAllocatePoolWithTag(NonPagedPool,
    FileInfo.EndOfFile.LowPart, TAG('K', 'x', 'e', 'c'));
  if (!FileReadBuffer) {
    ZwClose(FileHandle);
    return NULL;
  }

  /* Read the file in. */
  FilePointer.HighPart = FilePointer.LowPart = 0;
  status = ZwReadFile(FileHandle, NULL, NULL, NULL, &StatusBlock,
    FileReadBuffer, FileInfo.EndOfFile.LowPart, &FilePointer, NULL);
  if (!NT_SUCCESS(status)) {
    ExFreePool(FileReadBuffer);
    ZwClose(FileHandle);
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

/* Return the PIMAGE_NT_HEADERS for a loaded PE file.
   Returns NULL on error. */
PIMAGE_NT_HEADERS PeGetNtHeaders(PVOID PeFile)
{
  PIMAGE_NT_HEADERS PeHeaders;

  /* Verify the MZ magic number. */
  if (((PIMAGE_DOS_HEADER)PeFile)->e_magic != 0x5a4d)
    return NULL;

  /* Get to the PE header.
     Verify the magic number ("PE\0\0") while we're here. */
  PeHeaders = PeFile + ((PIMAGE_DOS_HEADER)PeFile)->e_lfanew;
  if (PeHeaders->Signature != 0x00004550)
    return NULL;

  return PeHeaders;
}

/* Return a pointer to the first PE section header, or NULL on error. */
PIMAGE_SECTION_HEADER PeGetFirstSectionHeader(PVOID PeFile)
{
  if (!PeGetNtHeaders(PeFile))
    return NULL;

  return (PIMAGE_SECTION_HEADER)(PeGetNtHeaders(PeFile) + 1);
}

/* Return a the number of PE section headers, or zero on error. */
WORD PeGetSectionCount(PVOID PeFile)
{
  if (!PeGetNtHeaders(PeFile))
    return 0;

  return PeGetNtHeaders(PeFile)->FileHeader.NumberOfSections;
}

/* Get the section header for the section that houses the given address. */
PIMAGE_SECTION_HEADER PeFindSectionHeaderForAddress(PVOID PeFile, DWORD Address)
{
  PIMAGE_SECTION_HEADER CurrentSection;

  if (!PeGetNtHeaders(PeFile))
    return NULL;

  PeForEachSectionHeader(PeFile, CurrentSection) {
    if (Address >= CurrentSection->VirtualAddress &&
      Address < CurrentSection->VirtualAddress + CurrentSection->SizeOfRawData)
        return CurrentSection;
  }
  return NULL;
}

/* Convert a relative virtual address (RVA) into a usable pointer
   into the raw PE file data.  Returns NULL on error. */
PVOID PeConvertRva(PVOID PeFile, DWORD Rva)
{
  PIMAGE_SECTION_HEADER RelevantSection;

  if (!(RelevantSection = PeFindSectionHeaderForAddress(PeFile, Rva)))
    return NULL;

  return RelevantSection->PointerToRawData - RelevantSection->VirtualAddress +
    PeFile + Rva;
}

/* Return the export directory from a PE file, or NULL on error. */
PIMAGE_EXPORT_DIRECTORY PeGetExportDirectory(PVOID PeFile)
{
  PIMAGE_NT_HEADERS PeHeaders;

  if (!(PeHeaders = PeGetNtHeaders(PeFile)))
    return NULL;

  return PeConvertRva(PeFile,
    PeHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
}

/* Return the import directory from a PE file, or NULL on error.
   (This is really the first import descriptor - there is no separate
   directory, as with the exports.) */
PIMAGE_IMPORT_DESCRIPTOR PeGetFirstImportDescriptor(PVOID PeFile)
{
  PIMAGE_NT_HEADERS PeHeaders;

  if (!(PeHeaders = PeGetNtHeaders(PeFile)))
    return NULL;

  return PeConvertRva(PeFile,
    PeHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
}

/* Return the address of an exported function, or NULL on error. */
DWORD PeGetExportFunction(PVOID PeFile, PCHAR FunctionName)
{
  PIMAGE_EXPORT_DIRECTORY ExportDirectory;
  DWORD * Functions;
  DWORD * Names;
  WORD * Ordinals;
  DWORD i;

  if (!(ExportDirectory = PeGetExportDirectory(PeFile)))
    return (DWORD)NULL;

  Functions = PeConvertRva(PeFile, ExportDirectory->AddressOfFunctions);
  Names = PeConvertRva(PeFile, ExportDirectory->AddressOfNames);
  Ordinals = PeConvertRva(PeFile, ExportDirectory->AddressOfNameOrdinals);

  for (i = 0; i < ExportDirectory->NumberOfFunctions; i++) {
    if (!strcmp(PeConvertRva(PeFile, Names[i]), FunctionName))
      return Functions[Ordinals[ExportDirectory->Base + i - 1]];
  }
  return (DWORD)NULL;
}

/* Return the address of the import pointer, or NULL on error. */
DWORD PeGetImportPointer(PVOID PeFile, PCHAR DllName, PCHAR FunctionName)
{
  PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
  PIMAGE_THUNK_DATA NameThunk, CallThunk;
  PIMAGE_IMPORT_BY_NAME NamedImport;

  if (!(ImportDescriptor = PeGetFirstImportDescriptor(PeFile)))
    return (DWORD)NULL;

  while (ImportDescriptor->Name) {
    if (!strcasecmp(PeConvertRva(PeFile, ImportDescriptor->Name), DllName))
      goto FoundDll;
    ImportDescriptor++;
  }
  return (DWORD)NULL;

FoundDll:
  for (NameThunk = PeConvertRva(PeFile, ImportDescriptor->OriginalFirstThunk),
       CallThunk = (PIMAGE_THUNK_DATA)ImportDescriptor->FirstThunk;
    NameThunk->u1.AddressOfData; NameThunk++, CallThunk++)
  {
    NamedImport = PeConvertRva(PeFile, NameThunk->u1.AddressOfData);
    if (!strcmp(NamedImport->Name, FunctionName))
      return (DWORD)CallThunk;
  }
  return (DWORD)NULL;
}
