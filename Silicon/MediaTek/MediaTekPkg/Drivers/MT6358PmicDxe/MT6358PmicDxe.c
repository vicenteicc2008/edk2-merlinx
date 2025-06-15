#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>

#include <Protocol/MT6358Pmic.h>
#include <Protocol/MtkPmicWrapper.h>

#include "MT6358PmicDxe.h"

MTK_PWRAP *PmicWrapper;

struct MT6358_VOSEL {
  UINT32 Voltage;
  UINT32 Mask;
};

struct MT6358_LDO_CFG {
  UINT32 Con0Reg;
  UINT32 Con1Reg;
  UINT32 AnaReg;
  UINT32 VoselShift;
  UINT32 VoselBits;
  UINT32 VoselLen;
  struct MT6358_VOSEL *VoltRange;
};

struct MT6358_BUCK_CFG {
  UINT32 Con0Reg;
  UINT32 Con1Reg;
  UINT32 Elr0Reg;
  UINT32 VOutBits;
  UINT32 UVStep;
  UINT32 UVMin;
};

struct MT6358_VOSEL VmcRange[] = {
  {1800, 0x04},
  {2900, 0x0a},
  {3000, 0x0b},
  {3300, 0x0d}
};

struct MT6358_VOSEL VemcRange[] = {
  {2900, 0x02},
  {3000, 0x03},
  {3300, 0x05}
};

struct MT6358_VOSEL VmchRange[] = {
  {2900, 0x02},
  {3000, 0x03},
  {3300, 0x05}
};

struct MT6358_LDO_CFG Ldos[] = {
  {MT6358_LDO_VMC_CON0, MT6358_LDO_VMC_CON1, MT6358_LDO_VMC_ANA_CON0, 8, 4, 4,  VmcRange},
  {MT6358_LDO_VEMC_CON0, MT6358_LDO_VEMC_CON1, MT6358_LDO_VEMC_ANA_CON0, 8, 3, 3, VemcRange},
  {MT6358_LDO_VMCH_CON0, MT6358_LDO_VMCH_CON1, MT6358_LDO_VMCH_ANA_CON0, 8, 3, 3, VmchRange},
  {MT6358_LDO_VDRAM2_CON0, MT6358_LDO_VDRAM2_CON1, MT6358_LDO_VDRAM2_ANA_CON0, 0, 0, 0, NULL}
};

struct MT6358_BUCK_CFG Bucks[] = {
  {MT6358_BUCK_VPROC11_CON0, MT6358_BUCK_VPROC11_CON1, MT6358_BUCK_VPROC11_ELR0, 7, 6250, 500000},
  {MT6358_BUCK_VPROC12_CON0, MT6358_BUCK_VPROC12_CON1, MT6358_BUCK_VPROC12_ELR0, 7, 6250, 500000},
  {0, 0, MT6358_BUCK_VSRAM_VPROC11_CON0, 7, 6250, 500000},
  {0, 0, MT6358_BUCK_VSRAM_VPROC12_CON0, 7, 6250, 500000},
};

VOID
EFIAPI
LdoStatus (
  IN  UINT32   LdoId,
  OUT BOOLEAN *Status
  )
{
  struct MT6358_LDO_CFG Ldo;
  UINT16 Reg;
  Ldo = Ldos[LdoId];

  PmicWrapper->Read16 (Ldo.Con1Reg, &Reg);
  
  *Status = Reg & MT6358_LDO_CON1_STA;
}

VOID
EFIAPI
LdoControl (
  IN  UINT32  LdoId,
  IN  BOOLEAN Ctrl
  )
{
  struct MT6358_LDO_CFG Ldo;
  Ldo = Ldos[LdoId];

  PmicWrapper->Write16 (Ldo.Con0Reg, Ctrl);
}

VOID
EFIAPI
LdoGetVoltage (
  IN  UINT32  LdoId,
  OUT UINT32 *Voltage
  )
{
  struct MT6358_LDO_CFG Ldo;
  struct MT6358_VOSEL *VoltRange;
  UINT32 Vosel;
  UINT16 Reg;
  BOOLEAN Found = FALSE;
  Ldo = Ldos[LdoId];
  VoltRange = Ldo.VoltRange;

  PmicWrapper->Read16 (Ldo.AnaReg, &Reg);
  
  Vosel = (Reg >> Ldo.VoselShift) & ((1 << Ldo.VoselBits) - 1);

  for (int i = 0; i <= (Ldo.VoselLen - 1); i++)
  {
    if (Vosel == VoltRange[i].Mask) {
      *Voltage = VoltRange[i].Voltage;
      Found = TRUE;
      break;
    }
  }
  if (!Found) {
    DEBUG ((DEBUG_ERROR, "MT6358PmicDxe: Failed to get LDO%d voltage, VOSEL = 0x%x\n", LdoId, Vosel));
    ASSERT (FALSE);
  }
}

VOID
EFIAPI
LdoSetVoltage (
  IN UINT32 LdoId,
  IN UINT32 Voltage
  )
{
  struct MT6358_LDO_CFG Ldo;
  struct MT6358_VOSEL *VoltRange;
  UINT32 Vosel;
  UINT16 Reg;
  BOOLEAN Found = FALSE;

  Ldo = Ldos[LdoId];
  VoltRange = Ldo.VoltRange;

  for (int i = 0; i <= (Ldo.VoselLen - 1); i++)
  {
    if (Voltage == VoltRange[i].Voltage) {
      Vosel = VoltRange[i].Mask;
      Found = TRUE;
      break;
    }
  }

  if (!Found) {
    DEBUG ((DEBUG_ERROR, "MT6358PmicDxe: Invalid voltage %d for LDO%d\n", Voltage, LdoId));
    ASSERT (FALSE);
  }

  PmicWrapper->Read16 (Ldo.AnaReg, &Reg);
  Reg &= ~(((1 << Ldo.VoselBits) - 1) << Ldo.VoselShift);
  Reg |= Vosel << Ldo.VoselShift;
  PmicWrapper->Write16 (Ldo.AnaReg, Reg);
}

VOID
EFIAPI
BuckGetMicroVoltage (
  IN  UINT32  BuckId,
  OUT UINT32 *MicroVoltage
  )
{
  struct MT6358_BUCK_CFG Buck;
  UINT16 Reg;
  UINT16 Code;

  Buck = Bucks[BuckId];

  PmicWrapper->Read16 (Buck.Elr0Reg, &Reg);
  
  Code = Reg & ((1 << Buck.VOutBits) - 1);
  *MicroVoltage = Buck.UVMin + (Code * Buck.UVStep);
}

VOID
EFIAPI
BuckSetMicroVoltage (
  IN  UINT32 BuckId,
  IN  UINT32 MicroVoltage
  )
{
  struct MT6358_BUCK_CFG Buck;
  UINT16 Reg;

  Buck = Bucks[BuckId];

  if (MicroVoltage < Buck.UVMin) {
    DEBUG ((DEBUG_ERROR, "MT6358PmicDxe: Invalid buck voltage!\n"));
    ASSERT (FALSE);
  }

  PmicWrapper->Read16 (Buck.Elr0Reg, &Reg);
  
  Reg &= ~((1 << Buck.VOutBits) - 1);
  Reg |= ((MicroVoltage - Buck.UVMin) / Buck.UVStep) & ((1 << Buck.VOutBits) - 1);

  PmicWrapper->Write16 (Buck.Elr0Reg, Reg);
}

VOID
EFIAPI
PowerButtonDebounce (
  OUT BOOLEAN *isPressed
  )
{
  UINT16 DebReg;
  PmicWrapper->Read16 (MT6358_TOPSTATUS, &DebReg);
  DebReg &= MT6358_PWRKEY_DEB;

  *isPressed = !DebReg;
}

VOID
EFIAPI
HomeButtonDebounce (
  OUT BOOLEAN     *isPressed
)
{
  UINT16 DebReg;
  PmicWrapper->Read16 (MT6358_TOPSTATUS, &DebReg);
  DebReg &= MT6358_HOMEKEY_DEB;

  *isPressed = !DebReg;
}

VOID
EFIAPI
RtcGetTime (
  OUT EFI_TIME *Time
  )
{
  UINT16 Second, Minute, Hour, Day, Month, Year;

  PmicWrapper->Read16 (MT6358_RTC_TC_SEC, &Second);
  PmicWrapper->Read16 (MT6358_RTC_TC_MIN, &Minute);
  PmicWrapper->Read16 (MT6358_RTC_TC_HOU, &Hour);
  PmicWrapper->Read16 (MT6358_RTC_TC_DOM, &Day);
  PmicWrapper->Read16 (MT6358_RTC_TC_MTH, &Month);
  PmicWrapper->Read16 (MT6358_RTC_TC_YEA, &Year);

  Time->Second = Second;
  Time->Minute = Minute;
  Time->Hour = Hour;
  Time->Day = Day;
  Time->Month = Month;
  Time->Year = 2000 + Year;
}

MT6358_PMIC MtkPmic = {
  PowerButtonDebounce,
  HomeButtonDebounce,
  LdoStatus,
  LdoControl,
  LdoGetVoltage,
  LdoSetVoltage,
  BuckGetMicroVoltage,
  BuckSetMicroVoltage,
  RtcGetTime,
};

EFI_STATUS
EFIAPI
MT6358PmicInitialize (
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS Status;

  Status = gBS->LocateProtocol (&gMediaTekPmicWrapperProtocolGuid, NULL, (VOID **)&PmicWrapper);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "MT6358PmicDxe: failed to get pmic wrapper protocol, Status = %r!\n", Status));
    return Status;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
    &ImageHandle,
    &gMediaTekMT6358PmicProtocolGuid,
    &MtkPmic,
    NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "MT6358PmicDxe: Failed to install protocol, Status = %r\n", Status));
    return Status;
  }

  return Status;
}
