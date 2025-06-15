#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MtkGpioImplLib.h>
#include <Library/DebugLib.h>

#include <Protocol/MtkGpio.h>

/* Offset for Set|Get values */
#define PIN_SG_OFFSET(P)   ((P / 32) * 0x10)
#define PIN_MODE_OFFSET(P) ((P / 8) * 0x10)
#define PIN_MODE_BIT(P)    ((Pin > 8) ? (Pin % 8) * 4 : Pin * 4)

VOID
GpioWrite (
  IN UINT32    Reg,
  IN UINT32    Value
  )
{
  MmioWrite32(PlatformInfo.BaseAddr + Reg, Value);
}

VOID
GpioRead (
  IN  UINT32    Reg,
  OUT UINT32   *Value
  )
{
  *Value = MmioRead32(PlatformInfo.BaseAddr + Reg);
}

VOID
GpioSetMode (
	UINT32        Pin,
	MTK_GPIO_MODE Mode
  )
{
	UINT32 Offset = PlatformInfo.ModeOffset + PIN_MODE_OFFSET (Pin);
	UINT32 Bit = PIN_MODE_BIT (Pin);
	UINT32 ModeNum = (UINT32) Mode;
  UINT32 DataSet, DataReset;

	GpioRead (Offset + PlatformInfo.SetOffset, &DataSet);
	GpioRead (Offset + PlatformInfo.ResetOffset, &DataReset);

	for (UINT8 i = 0; i < 3; i++) {
		if (ModeNum & (1 << i)) {
			DataSet |= (1U << (Bit + i));
		} else {
			DataReset |= (1U << (Bit + i));
		}
	}

	GpioWrite (Offset + PlatformInfo.SetOffset, DataSet);
	GpioWrite (Offset + PlatformInfo.ResetOffset, DataReset);
}

VOID
GpioSetDir (
	UINT32        Pin,
	BOOLEAN       Direction
  )
{
  UINT32 Offset, GpioReg;

	Offset = PlatformInfo.DirOffset + PIN_SG_OFFSET (Pin);

	if (Direction) {
		Offset += PlatformInfo.SetOffset;
	} else {
		Offset += PlatformInfo.ResetOffset;
	}

  GpioRead (Offset, &GpioReg);
	GpioWrite (Offset, GpioReg | (1 << (Pin % 32)));
}

VOID
GpioGetDir (
	UINT32   Pin,
  BOOLEAN *Direction
  )
{
  UINT32 Offset, GpioReg;

	Offset = PlatformInfo.DataInOffset + PIN_SG_OFFSET (Pin);

  GpioRead (Offset, &GpioReg);
  *Direction = (GpioReg & (1 << (Pin % 32))) ? 0 : 1;
}

VOID
GpioGetOut (
	UINT32 Pin,
  BOOLEAN *Status
  )
{
  UINT32 Offset, GpioReg;

	Offset = PlatformInfo.DataOutOffset + PIN_SG_OFFSET (Pin);

  GpioRead (Offset, &GpioReg);
	*Status = GpioReg & (1 << (Pin % 32));
}

VOID
GpioGetIn (
	UINT32   Pin,
  BOOLEAN *Status
  )
{
	UINT32 Offset, GpioReg;

  Offset = PlatformInfo.DataInOffset + PIN_SG_OFFSET (Pin);

  GpioRead (Offset, &GpioReg);
	*Status = GpioReg & (1 << (Pin % 32));
}

VOID
GpioSet (
	UINT32     Pin,
	BOOLEAN    Status
  )
{
	UINT32 Offset, GpioReg;

  Offset = PlatformInfo.DataOutOffset + PIN_SG_OFFSET (Pin);

	if (Status) {
		Offset += PlatformInfo.SetOffset;
	} else {
		Offset += PlatformInfo.ResetOffset;
	}

  GpioRead (Offset, &GpioReg);

	GpioWrite (Offset, GpioReg | (1 << (Pin % 32)));
}

VOID GpioGet (
  UINT32    Pin,
  BOOLEAN  *Status
  )
{
  BOOLEAN Direction;

  GpioGetDir (Pin, &Direction);

  if (Direction) {
    GpioGetIn (Pin, Status);
  } else {
    GpioGetOut (Pin, Status);
  }
}

VOID
GpioSetDrv (
  UINT32 Pin,
  UINT32 Drv
  )
{
  struct MTK_GPIO_DRV_TABLE PinInfo;
  UINT32 Reg;

  for (UINT32 i = 0; i <= PlatformInfo.MaxPin; i++) {
    if (DrvTable[i].Pin == Pin) {
      PinInfo = DrvTable[i];
      break;
    }
  }

  Reg = MmioRead32 (PinInfo.Base);
  Reg &= ~(PinInfo.Mask << PinInfo.Shift);
  Reg |= (Drv << PinInfo.Shift);
  MmioWrite32 (PinInfo.Base, Reg);
}

VOID
GpioSetR0 (
  UINT32 Pin
  )
{
  struct MTK_GPIO_R_TABLE PinInfo;
  UINT32 Reg;

  for (UINT32 i = 0; i <= PlatformInfo.MaxPin; i++) {
    if (RTable[i].Pin == Pin) {
      PinInfo = RTable[i];
      break;
    }
  }
  // SET R0
  Reg = MmioRead32 (PinInfo.R0Base);
  Reg |= (1 << PinInfo.R0Shift);
  MmioWrite32 (PinInfo.R0Base, Reg);
  // CLR R1
  Reg = MmioRead32 (PinInfo.R1Base);
  Reg &= ~(1 << PinInfo.R1Shift);
  MmioWrite32 (PinInfo.R1Base, Reg);
}

VOID
GpioSetR1 (
  UINT32 Pin
  )
{
  struct MTK_GPIO_R_TABLE PinInfo;
  UINT32 Reg;

  for (UINT32 i = 0; i <= PlatformInfo.MaxPin; i++) {
    if (RTable[i].Pin == Pin) {
      PinInfo = RTable[i];
      break;
    }
  }
  // CLR R0
  Reg = MmioRead32 (PinInfo.R0Base);
  Reg &= ~(1 << PinInfo.R0Shift);
  MmioWrite32 (PinInfo.R0Base, Reg);
  // SET R1
  Reg = MmioRead32 (PinInfo.R1Base);
  Reg |= (1 << PinInfo.R1Shift);
  MmioWrite32 (PinInfo.R1Base, Reg);
}

VOID
GpioSetPupd (
  UINT32  Pin,
  BOOLEAN IsPullDown
  )
{
  struct MTK_GPIO_PUPD_TABLE PinInfo;
  UINT32 Reg;

  for (UINT32 i = 0; i <= PlatformInfo.MaxPin; i++) {
    if (PupdTable[i].Pin == Pin) {
      PinInfo = PupdTable[i];
      break;
    }
  }

  Reg = MmioRead32 (PinInfo.Base);
  if (IsPullDown)
    Reg |= (1 << PinInfo.Shift);
  else
    Reg &= ~(1 << PinInfo.Shift);
  MmioWrite32 (PinInfo.Base, Reg);
}

MTK_GPIO MtkGpio = {
  GpioGet,
  GpioSet,
  GpioGetDir,
  GpioSetDir,
  GpioSetMode,
  GpioSetDrv,
  GpioSetR0,
  GpioSetR1,
  GpioSetPupd
};

EFI_STATUS
EFIAPI
MtkGpioInitialize(
  IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  EFI_STATUS Status;

  Status = gBS->InstallMultipleProtocolInterfaces(
    &ImageHandle,
    &gMtkGpioProtocolGuid,
    &MtkGpio,
    NULL);

  return Status;
}
