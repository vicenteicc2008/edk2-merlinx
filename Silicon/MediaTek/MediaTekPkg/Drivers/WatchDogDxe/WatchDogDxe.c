#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include <Library/PcdLib.h>

#define WATCHDOG_LENGTH (0x4)
#define WATCHDOG_RESTART (0x8)

#define WDT_LENGTH_KEY (0x8)
#define WDT_RST_KEY (0x1971)
#define WDT_MODE_KEY (0x22000000)

UINT32 WatchdogBase;
UINT32 WatchdogModeBackup;
BOOLEAN WorkingTimer;

EFI_EVENT WatchdogTimerEvent;
EFI_EVENT mEfiExitBootServicesEvent;

VOID
WatchdogDisable ()
{
  MmioWrite32 (WatchdogBase, WDT_MODE_KEY);
}

VOID
WatchdogEnable ()
{
  MmioWrite32 (WatchdogBase, WatchdogModeBackup | WDT_MODE_KEY);
}

VOID
WatchdogPing ()
{
  MmioWrite32 (WatchdogBase + WATCHDOG_RESTART, WDT_RST_KEY);
}

VOID
SetTimeout (UINT32 TimeoutMicroSeconds)
{
  // Watchdog timer timeout period is multiple of 15.6ms
  UINT32 Reg = (TimeoutMicroSeconds / 15600) | WDT_LENGTH_KEY;
  MmioWrite32 (WatchdogBase + WATCHDOG_LENGTH, Reg);

  WatchdogPing ();
}

VOID
EFIAPI
PingWatchdogEvent (
  IN EFI_EVENT Event,
  IN VOID *Context
  )
{
  if (WorkingTimer) {
    WatchdogPing ();
  } else {
    WatchdogEnable ();
  }
}

EFI_STATUS
EFIAPI
WatchdogDxeEntry (
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS Status;

  WatchdogBase = FixedPcdGet32(PcdWatchdogBaseAddress);
  WorkingTimer = FixedPcdGet32(PcdWatchdogWorkingTimer);
  WatchdogModeBackup = MmioRead32 (WatchdogBase);

  // Set watchdog timeout to 30s
  SetTimeout (30*1000*1000);

  if (WorkingTimer) {
    Status = gBS->CreateEvent (
      EVT_TIMER | EVT_NOTIFY_SIGNAL,
      TPL_CALLBACK,
      PingWatchdogEvent,
      NULL,
      &WatchdogTimerEvent
    );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "WatchDogDxe: Failed to create timer event: %r\n", Status));
      return Status;
    }

    // Ping watchdog every 5s
    Status = gBS->SetTimer (
      WatchdogTimerEvent,
      TimerPeriodic,
      5*1000*1000*10
    );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "WatchDogDxe: Failed to set periodic timer: %r\n", Status));
      return Status;
    }
  } else {
    WatchdogDisable ();
  }

  // Ping watchdog before os boot
  Status = gBS->CreateEvent (
    EVT_SIGNAL_EXIT_BOOT_SERVICES,
    TPL_NOTIFY,
    PingWatchdogEvent,
    NULL,
    &mEfiExitBootServicesEvent
  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "WatchDogDxe: Failed to create ExitBootServices event: %r\n", Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "WatchDogDxe: Watchdog kicker initialized.\n"));
  return EFI_SUCCESS;
}