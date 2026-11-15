#include "EfiPlatform.h"
#include "../src/Application.h"

EFI_SYSTEM_TABLE *zST;
EFI_BOOT_SERVICES *zBS;
EFI_HANDLE zImage;
EFI_FILE_PROTOCOL *zRoot;
CHAR16 zDirectory[512];
EFI_GRAPHICS_OUTPUT_PROTOCOL *zGop;
EFI_SIMPLE_POINTER_PROTOCOL *zPointer;
EFI_ABSOLUTE_POINTER_PROTOCOL *zAbsolute;
int zExitCode;
static UINT32 oldMode;
static int graphicsActive;
static uint64_t ticksPerMs, startTicks;
unsigned __int64 __rdtsc(void);
#pragma intrinsic(__rdtsc)
void ZumaFreeAllocations(void);
void ZumaPrint(const char *text) {
    CHAR16 chunk[128];
    while(*text) {UINTN n=0;while(*text && n<125){if(*text=='\n')chunk[n++]=L'\r';chunk[n++]=(unsigned char)*text++;}chunk[n]=0;zST->ConOut->OutputString(zST->ConOut,chunk);}
}
uint64_t ZumaMilliseconds(void) {return ticksPerMs?(__rdtsc()-startTicks)/ticksPerMs:0;}
void ZumaRestoreDisplay(void) {
    if(graphicsActive && zGop) {zGop->SetMode(zGop,oldMode);graphicsActive=0;}
    zST->ConOut->EnableCursor(zST->ConOut,TRUE);
}
void ZumaBeginGraphics(void) {if(zGop){oldMode=zGop->Mode->Mode;graphicsActive=1;zST->ConOut->EnableCursor(zST->ConOut,FALSE);}}

EFI_STATUS EFIAPI EfiMain(EFI_HANDLE image,EFI_SYSTEM_TABLE *st) {
    zST=st;zBS=st->BootServices;zImage=image;
    uint64_t before=__rdtsc();zBS->Stall(20000);ticksPerMs=(__rdtsc()-before)/20;startTicks=__rdtsc();
    if(!ticksPerMs)ticksPerMs=1;
    EFI_GUID loadedGuid=EFI_LOADED_IMAGE_PROTOCOL_GUID,fsGuid=EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_GUID gopGuid=EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID,ptrGuid=EFI_SIMPLE_POINTER_PROTOCOL_GUID,absGuid=EFI_ABSOLUTE_POINTER_PROTOCOL_GUID;
    EFI_LOADED_IMAGE_PROTOCOL *loaded=NULL;EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs=NULL;
    EFI_STATUS status=zBS->HandleProtocol(image,&loadedGuid,(void **)&loaded);
    if(!EFI_ERROR(status))status=zBS->HandleProtocol(loaded->DeviceHandle,&fsGuid,(void **)&fs);
    if(!EFI_ERROR(status))status=fs->OpenVolume(fs,&zRoot);
    if(EFI_ERROR(status)){ZumaPrint("Zuma: cannot open the application volume.\n");return status;}
    /* Use the image's directory, independent of the UEFI Shell working directory. */
    UINTN n=0;EFI_DEVICE_PATH_PROTOCOL *node=loaded->FilePath;
    while(node && node->Type!=END_DEVICE_PATH_TYPE) {
        UINTN length=node->Length[0]|((UINTN)node->Length[1]<<8);
        if(length<4)break;
        if(node->Type==MEDIA_DEVICE_PATH && node->SubType==MEDIA_FILEPATH_DP) {
            CHAR16 *part=(CHAR16 *)((UINT8 *)node+4);UINTN chars=(length-4)/2;
            for(UINTN i=0;i<chars && part[i] && n<510;i++)zDirectory[n++]=part[i];
        }
        node=(EFI_DEVICE_PATH_PROTOCOL *)((UINT8 *)node+length);
    }
    while(n && zDirectory[n-1]!=L'\\' && zDirectory[n-1]!=L'/')--n;
    zDirectory[n]=0;
    if(EFI_ERROR(zBS->LocateProtocol(&gopGuid,NULL,(void **)&zGop))) {
        ZumaPrint("Zuma requires the UEFI Graphics Output Protocol.\n");zRoot->Close(zRoot);return EFI_UNSUPPORTED;
    }
    /* Prefer console pointer handles; firmware may expose additional USB devices. */
    zBS->HandleProtocol(st->ConsoleInHandle,&ptrGuid,(void **)&zPointer);
    if(!zPointer)zBS->LocateProtocol(&ptrGuid,NULL,(void **)&zPointer);
    zBS->HandleProtocol(st->ConsoleInHandle,&absGuid,(void **)&zAbsolute);
    if(!zAbsolute)zBS->LocateProtocol(&absGuid,NULL,(void **)&zAbsolute);
    if(zPointer)zPointer->Reset(zPointer,FALSE);
    if(zAbsolute)zAbsolute->Reset(zAbsolute,FALSE);
    zST->ConIn->Reset(zST->ConIn,FALSE);
    int result=ApplicationZuma_Start();
    ZumaRestoreDisplay();
    if(result){ZumaPrint("Zuma failed to start. Check resources beside BOOTX64.EFI.\nPress a key to return.\n");UINTN index;zBS->WaitForEvent(1,&zST->ConIn->WaitForKey,&index);}
    zRoot->Close(zRoot);zRoot=NULL;ZumaFreeAllocations();
    return result?EFI_ABORTED:EFI_SUCCESS;
}
