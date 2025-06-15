#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>

#include <Protocol/MtkPmicWrapper.h>
#include <Library/PmicWrapperImplLib.h>

#define WACS_FSM_IDLE     (0x0)
#define WACS_FSM_REQ      (0x2)
#define WACS_FSM_WFVLDCLR (0x6)

VOID PwrapWrite (
  IN UINT32 RegId,
  IN UINT32 Data
  )
{
  UINT32 Reg = PlatformInfo.BaseAddr +
               PlatformInfo.RegMap[RegId];
  MmioWrite32 (Reg, Data);
}

VOID PwrapRead (
  IN  UINT32 RegId,
  OUT UINT32 *Value
  )
{
  UINT32 Reg = PlatformInfo.BaseAddr +
               PlatformInfo.RegMap[RegId];
  *Value = MmioRead32 (Reg);
}

UINT32 WacsGetFsm (VOID)
{
  UINT32 Rdata;
  PwrapRead (PWRAP_WACS2_RDATA, &Rdata);
  if (PlatformInfo.ArbCap)
    return ((Rdata >> 1) & 0x7);
  else
    return (Rdata >> 16) & 0x7;
}

VOID WacsWaitFor (
  IN UINT32 Fsm
)
{
  while (WacsGetFsm () != Fsm)
  {
    MicroSecondDelay (100);
  }
}

VOID WacsCommand (
  IN BOOLEAN IsWrite,
  IN UINT32 Address,
  IN UINT32 Data
  )
{
  UINT32 WacsCmd;

  WacsWaitFor (WACS_FSM_IDLE);

  if (PlatformInfo.ArbCap) {
    /*
     * 29: Operation (0 read, 1 write)
     * 0:28 Address
     */
    if (IsWrite) {
      PwrapWrite (PWRAP_SWINF_2_WDATA_31_0, Data);
      WacsCmd = (1 << 29) | Address;
    } else {
      WacsCmd = Address;
    }
  } else {
    /*
     * 31: Operation (0 read, 1 write)
     * 16:30 Address
     * 0:15 Data
     */
    WacsCmd = (IsWrite << 31) |
              ((Address >> 1) << 16) |
              Data;
  }
  PwrapWrite (PWRAP_WACS2_CMD, WacsCmd);
}

VOID WacsWrite16 (
  IN UINT16 Address,
  IN UINT16 Data
  )
{
  WacsCommand (TRUE, Address, Data);
}

VOID WacsRead16 (
  IN  UINT16 Address,
  OUT UINT16 *Value
  )
{
  UINT32 Result;

  WacsCommand (FALSE, Address, 0);
  WacsWaitFor (WACS_FSM_WFVLDCLR);

  if (PlatformInfo.ArbCap) {
    PwrapRead (PWRAP_SWINF_2_RDATA_31_0, &Result);
  } else {
    PwrapRead (PWRAP_WACS2_RDATA, &Result);
  }

  PwrapWrite (PWRAP_WACS2_VLDCLR, 1);

  *Value = (Result & 0xFFFF);
}

MTK_PWRAP MtkPwrap = {
  WacsRead16,
  WacsWrite16
};

EFI_STATUS
EFIAPI
PmicWrapperInitialize (
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS Status;
  UINT32 InitState;

  /* We expect bootloader initialized it */
  PwrapRead (PWRAP_INIT_DONE2, &InitState);

  if (InitState != 1) {
    DEBUG ((DEBUG_ERROR, "PmicWrapperDxe: Pmic Wrapper is not initialized!\n"));
    Status = EFI_NOT_READY;
    ASSERT_EFI_ERROR(Status);
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
    &ImageHandle,
    &gMediaTekPmicWrapperProtocolGuid,
    &MtkPwrap,
    NULL);
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "PmicWrapperDxe: failed to install protocol!\n"));
    return Status;
  }

  return Status;
}
