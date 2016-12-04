/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved
                                                                                   

  This program and the accompanying materials are licensed and made available under

  the terms and conditions of the BSD License that accompanies this distribution.  

  The full text of the license may be found at                                     

  http://opensource.org/licenses/bsd-license.php.                                  

                                                                                   

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            

  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    

                                                                                   


--*/

/** @file
**/

#include <Uefi.h>
#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/DriverBinding.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/UnicodeCollation.h>

#include <Guid/FileSystemInfo.h>
#include <Guid/FileInfo.h>
#include <Guid/FileSystemVolumeLabelInfo.h>

#define EFI_PLATFORM_GOP_POLICY_PROTOCOL_GUID \
  { 0xec2e931b, 0x3281, 0x48a5, 0x81, 0x7, 0xdf, 0x8a, 0x8b, 0xed, 0x3c, 0x5d }

#define EFI_BMP_IMAGE_GUID \
  { 0x878AC2CC, 0x5343, 0x46F2, 0xB5, 0x63, 0x51, 0xF8, 0x9D, 0xAF, 0x56, 0xBA }

#define PLATFORM_GOP_POLICY_PROTOCOL_REVISION_01 0x01
#define PLATFORM_GOP_POLICY_PROTOCOL_REVISION_02 x0222

#pragma pack(1)

typedef enum {
  LidClosed,
  LidOpen,
  LidStatusMax
} LID_STATUS;

typedef
EFI_STATUS
(EFIAPI *GET_PLATFORM_LID_STATUS) (
   OUT LID_STATUS *CurrentLidStatus
);

typedef
EFI_STATUS
(EFIAPI *GET_VBT_DATA) (
   OUT EFI_PHYSICAL_ADDRESS *VbtAddress,
   OUT UINT32 *VbtSize
);

#pragma pack()

typedef struct _PLATFORM_GOP_POLICY_PROTOCOL {
  UINT32                             Revision;
  GET_PLATFORM_LID_STATUS            GetPlatformLidStatus;
  GET_VBT_DATA                       GetVbtData;
} PLATFORM_GOP_POLICY_PROTOCOL;

//
// Extern the GUID for protocol users.
//
extern EFI_GUID  gPlatformGOPPolicyGuid;

extern EFI_GUID  gBmpImageGuid;

#include <Library/UefiRuntimeServicesTableLib.h>

EFI_BOOT_SERVICES   *gBS;


PLATFORM_GOP_POLICY_PROTOCOL  mPlatformGOPPolicy;

//
// Function implementations
//

/**
  The function will excute with as the platform policy, and gives
  the Platform Lid Status. IBV/OEM can customize this code for their specific
  policy action.

  @param CurrentLidStatus  Gives the current LID Status

  @retval EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
GetPlatformLidStatus (
   OUT LID_STATUS *CurrentLidStatus
)
{
  *CurrentLidStatus = LidOpen;

  return EFI_SUCCESS;
}

/**
  The function will excute and gives the Video Bios Table Size and Address.

  @param VbtAddress  Gives the Physical Address of Video BIOS Table

  @param VbtSize     Gives the Size of Video BIOS Table

  @retval EFI_STATUS.

**/

EFI_STATUS
EFIAPI
GetVbtData (
   OUT EFI_PHYSICAL_ADDRESS *VbtAddress,
   OUT UINT32 *VbtSize
)
{
  EFI_STATUS                    Status;
  UINTN                         FvProtocolCount;
  EFI_HANDLE                    *FvHandles;
  EFI_FIRMWARE_VOLUME2_PROTOCOL  *Fv;
  UINTN                         Index;
  UINT32                        AuthenticationStatus;

  UINT8                         *Buffer;
  UINTN                         VbtBufferSize;

  Buffer = 0;
  FvHandles       = NULL;

  if (VbtAddress == NULL || VbtSize == NULL){
    return EFI_INVALID_PARAMETER;
  }
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareVolume2ProtocolGuid,
                  NULL,
                  &FvProtocolCount,
                  &FvHandles
                  );

  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < FvProtocolCount; Index++) {
      Status = gBS->HandleProtocol (
                      FvHandles[Index],
                      &gEfiFirmwareVolume2ProtocolGuid,
                      (VOID **) &Fv
                      );
      VbtBufferSize = 0;
      Status = Fv->ReadSection (
                     Fv,
                     &gBmpImageGuid,
                     EFI_SECTION_RAW,
                     0,
                    (void **)&Buffer,
                     &VbtBufferSize,
                     &AuthenticationStatus
                     );

      if (!EFI_ERROR (Status)) {
        *VbtAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer;
        *VbtSize = (UINT32)VbtBufferSize;
        Status = EFI_SUCCESS;
        break;
      }
    }
  } else {
    Status = EFI_NOT_FOUND;
  }

  if (FvHandles != NULL) {
    gBS->FreePool (FvHandles);
    FvHandles = NULL;
  }

  return Status;
}

/**
  Entry point for the Platform GOP Policy Driver.

  @param ImageHandle       Image handle of this driver.
  @param SystemTable       Global system service table.

  @retval EFI_SUCCESS           Initialization complete.
  @retval EFI_OUT_OF_RESOURCES  Do not have enough resources to initialize the driver.

**/

EFI_STATUS
EFIAPI
PlatformGOPPolicyEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )

{
  EFI_STATUS  Status = EFI_SUCCESS;

  gBS = SystemTable->BootServices;

  gBS->SetMem (
         &mPlatformGOPPolicy,
         sizeof (PLATFORM_GOP_POLICY_PROTOCOL),
         0
         );

  mPlatformGOPPolicy.Revision                = PLATFORM_GOP_POLICY_PROTOCOL_REVISION_01;
  mPlatformGOPPolicy.GetPlatformLidStatus    = GetPlatformLidStatus;
  mPlatformGOPPolicy.GetVbtData              = GetVbtData;

  //
  // Install protocol to allow access to this Policy.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
          &ImageHandle,
          &gPlatformGOPPolicyGuid,
          &mPlatformGOPPolicy,
          NULL
          );
  return Status;
}
