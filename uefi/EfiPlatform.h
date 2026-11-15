#ifndef ZUMA_EFI_PLATFORM_H
#define ZUMA_EFI_PLATFORM_H
#include <Uefi.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/SimplePointer.h>
#include <Protocol/AbsolutePointer.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/DevicePath.h>
#include <Guid/FileInfo.h>
#include "include/Runtime.h"
extern EFI_SYSTEM_TABLE *zST;
extern EFI_BOOT_SERVICES *zBS;
extern EFI_HANDLE zImage;
extern EFI_FILE_PROTOCOL *zRoot;
extern CHAR16 zDirectory[512];
extern EFI_GRAPHICS_OUTPUT_PROTOCOL *zGop;
extern EFI_SIMPLE_POINTER_PROTOCOL *zPointer;
extern EFI_ABSOLUTE_POINTER_PROTOCOL *zAbsolute;
extern int zExitCode;
void ZumaPrint(const char *text);
void ZumaRestoreDisplay(void);
uint64_t ZumaMilliseconds(void);
#endif
