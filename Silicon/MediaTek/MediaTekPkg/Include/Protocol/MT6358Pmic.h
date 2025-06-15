#ifndef __MT6358_PMIC_H__
#define __MT6358_PMIC_H__
//
// Protocol interface structure
//

enum MT6358_LDO {
  MT6358_LDO_VMC,
  MT6358_LDO_VEMC,
  MT6358_LDO_VMCH,
  MT6358_LDO_VDRAM2
};

enum MT6358_BUCK {
  MT6358_BUCK_VPROC11,
  MT6358_BUCK_VPROC12,
  MT6358_BUCK_VSRAM_PROC11,
  MT6358_BUCK_VSRAM_PROC12
};

typedef struct _MT6358_PMIC MT6358_PMIC;

//
// Function Prototypes
//

typedef
VOID
(EFIAPI *MT6358_KEY_DEB)(
  OUT BOOLEAN     *isPressed
  );

typedef
VOID
(EFIAPI *MT6358_LDO_STA)(
  IN  UINT32   LdoId,
  OUT BOOLEAN *Status
  );

typedef
VOID
(EFIAPI *MT6358_LDO_CTRL)(
  IN  UINT32  LdoId,
  IN  BOOLEAN Ctrl
  );

typedef
VOID
(EFIAPI *MT6358_LDO_GET_VOLT)(
  IN  UINT32  LdoId,
  OUT UINT32 *Voltage
  );

typedef
VOID
(EFIAPI *MT6358_LDO_SET_VOLT)(
  IN UINT32 LdoId,
  IN UINT32 Voltage
  );

typedef
VOID
(EFIAPI *MT6358_BUCK_GET_VOLT)(
  IN  UINT32  BuckId,
  OUT UINT32 *MicroVoltage
  );

typedef
VOID
(EFIAPI *MT6358_BUCK_SET_VOLT)(
  IN UINT32 BuckId,
  IN UINT32 MicroVoltage
  );

typedef
VOID
(EFIAPI *MT6358_RTC_GET_TIME)(
  OUT EFI_TIME *Time
  );

struct _MT6358_PMIC {
  MT6358_KEY_DEB        PowerButtonDebounce;
  MT6358_KEY_DEB        HomeButtonDebounce;
  MT6358_LDO_STA        LdoStatus;
  MT6358_LDO_CTRL       LdoControl;
  MT6358_LDO_GET_VOLT   LdoGetVoltage;
  MT6358_LDO_SET_VOLT   LdoSetVoltage;
  MT6358_BUCK_GET_VOLT  BuckGetMicroVoltage;
  MT6358_BUCK_SET_VOLT  BuckSetMicroVoltage;
  MT6358_RTC_GET_TIME   RtcGetTime;
};

extern EFI_GUID gMediaTekMT6358PmicProtocolGuid;

#endif // __MT6358_PMIC_H__
