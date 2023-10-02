#include <Uefi.h>

#include <Library/KeypadDeviceImplLib.h>
#include <Library/UefiLib.h>
#include <Library/GpioLib.h>
#include <Protocol/KeypadDevice.h>
#include <Library/DebugLib.h>

typedef enum {
  KEY_DEVICE_TYPE_UNKNOWN,
  KEY_DEVICE_TYPE_LEGACY
} KEY_DEVICE_TYPE;

typedef struct {
  KEY_CONTEXT     EfiKeyContext;
  BOOLEAN         IsValid;
  KEY_DEVICE_TYPE DeviceType;
  UINT32   Gpio;
  BOOLEAN ActiveLow;
} KEY_CONTEXT_PRIVATE;


STATIC KEY_CONTEXT_PRIVATE KeyContextVolumeDown;

STATIC KEY_CONTEXT_PRIVATE *KeyList[] = { &KeyContextVolumeDown };

STATIC
VOID KeypadInitializeKeyContextPrivate(KEY_CONTEXT_PRIVATE *Context)
{
  Context->IsValid     = FALSE;
  Context->Gpio        = 0;
  Context->GpioOut     = 0;
  Context->GpioIn      = 0;
  Context->DeviceType  = KEY_DEVICE_TYPE_UNKNOWN;
  Context->ActiveLow   = FALSE;
}

STATIC
KEY_CONTEXT_PRIVATE *KeypadKeyCodeToKeyContext(UINT32 KeyCode)
{
  if (KeyCode == 114)
    return &KeyContextVolumeDown;
  else
    return NULL;
}

RETURN_STATUS
EFIAPI
KeypadDeviceImplConstructor(VOID)
{
  UINTN                Index;
  KEY_CONTEXT_PRIVATE *StaticContext;

  // Reset all keys
  for (Index = 0; Index < (sizeof(KeyList) / sizeof(KeyList[0])); Index++) {
    KeypadInitializeKeyContextPrivate(KeyList[Index]);
  }

  // Configure keys
  #error hi
  // volume down
#ifdef VOLUME_DOWN_GPIO
#error hi
  StaticContext              = KeypadKeyCodeToKeyContext(114);
  StaticContext->DeviceType  = KEY_DEVICE_TYPE_LEGACY;
  StaticContext->Gpio        = VOLUME_DOWN_GPIO;
  StaticContext->ActiveLow   = TRUE;
  StaticContext->IsValid     = TRUE;
#endif
  return RETURN_SUCCESS;
}

EFI_STATUS EFIAPI KeypadDeviceImplReset(KEYPAD_DEVICE_PROTOCOL *This)
{
  LibKeyInitializeKeyContext(&KeyContextVolumeDown.EfiKeyContext);
  KeyContextVolumeDown.EfiKeyContext.KeyData.Key.ScanCode = SCAN_DOWN;

  return EFI_SUCCESS;
}

EFI_STATUS KeypadDeviceImplGetKeys(
    KEYPAD_DEVICE_PROTOCOL *This, KEYPAD_RETURN_API *KeypadReturnApi,
    UINT64 Delta)
{
    UINT8 GpioStatus;
    BOOLEAN IsPressed;
    UINTN Index;

    for (Index = 0; Index < (sizeof(KeyList) / sizeof(KeyList[0])); Index++) {
        KEY_CONTEXT_PRIVATE *Context = KeyList[Index];

        // check if this is a valid key
        if (Context->IsValid == FALSE)
            continue;

        // get status
        if (Context->DeviceType == KEY_DEVICE_TYPE_LEGACY) {
            GpioStatus = GpioGetIn(Context->Gpio);
        } else {
            continue;
        }

        IsPressed = FALSE;

        if (GpioStatus == GPIO_LEVEL_LOW && Context->ActiveLow ||
        	GpioStatus == GPIO_LEVEL_HIGH && !Context->ActiveLow) {
        	IsPressed = TRUE;
        }

        LibKeyUpdateKeyStatus(
            &Context->EfiKeyContext, KeypadReturnApi, IsPressed, Delta);
    }

    return EFI_SUCCESS;
}
