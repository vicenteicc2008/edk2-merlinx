/* SimpleFbDxe: Simple FrameBuffer */
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/FrameBufferBltLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <PiDxe.h>
#include <Protocol/GraphicsOutput.h>
#include <Uefi.h>

#include <arm_neon.h>

/// Defines
/*
 * Convert enum video_log2_bpp to bytes and bits. Note we omit the outer
 * brackets to allow multiplication by fractional pixels.
 */
#define VNBYTES(bpix) (1 << (bpix)) / 8
#define VNBITS(bpix) (1 << (bpix))

#define FB_BITS_PER_PIXEL (32)
#define FB_BYTES_PER_PIXEL (FB_BITS_PER_PIXEL / 8)

/*
 * Bits per pixel selector. Each value n is such that the bits-per-pixel is
 * 2 ^ n
 */
enum video_log2_bpp {
  VIDEO_BPP1 = 0,
  VIDEO_BPP2,
  VIDEO_BPP4,
  VIDEO_BPP8,
  VIDEO_BPP16,
  VIDEO_BPP32,
};

typedef struct {
  VENDOR_DEVICE_PATH DisplayDevicePath;
  EFI_DEVICE_PATH    EndDevicePath;
} DISPLAY_DEVICE_PATH;

DISPLAY_DEVICE_PATH mDisplayDevicePath = {
    {{HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
          (UINT8)(sizeof(VENDOR_DEVICE_PATH)),
          (UINT8)((sizeof(VENDOR_DEVICE_PATH)) >> 8),
      }},
     EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID},
    {END_DEVICE_PATH_TYPE,
     END_ENTIRE_DEVICE_PATH_SUBTYPE,
     {sizeof(EFI_DEVICE_PATH_PROTOCOL), 0}}};

/// Declares

STATIC FRAME_BUFFER_CONFIGURE *mFrameBufferBltLibConfigure;
STATIC UINTN                   mFrameBufferBltLibConfigureSize;

STATIC
EFI_STATUS
EFIAPI
DisplayQueryMode(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL *This, IN UINT32 ModeNumber,
    OUT UINTN *SizeOfInfo, OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info);

STATIC
EFI_STATUS
EFIAPI
DisplaySetMode(IN EFI_GRAPHICS_OUTPUT_PROTOCOL *This, IN UINT32 ModeNumber);

STATIC
EFI_STATUS
EFIAPI
DisplayBlt(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL *            BltBuffer,
    OPTIONAL IN EFI_GRAPHICS_OUTPUT_BLT_OPERATION BltOperation,
    IN UINTN SourceX, IN UINTN SourceY, IN UINTN DestinationX,
    IN UINTN DestinationY, IN UINTN Width, IN UINTN Height,
    IN UINTN Delta OPTIONAL);

STATIC EFI_GRAPHICS_OUTPUT_PROTOCOL mDisplay = {
    DisplayQueryMode, DisplaySetMode, DisplayBlt, NULL};

STATIC
EFI_STATUS
EFIAPI
DisplayQueryMode(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL *This, IN UINT32 ModeNumber,
    OUT UINTN *SizeOfInfo, OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info)
{
  EFI_STATUS Status;
  Status = gBS->AllocatePool(
      EfiBootServicesData, sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION),
      (VOID **)Info);

  ASSERT_EFI_ERROR(Status);

  *SizeOfInfo                   = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
  (*Info)->Version              = This->Mode->Info->Version;
  (*Info)->HorizontalResolution = This->Mode->Info->HorizontalResolution;
  (*Info)->VerticalResolution   = This->Mode->Info->VerticalResolution;
  (*Info)->PixelFormat          = This->Mode->Info->PixelFormat;
  (*Info)->PixelsPerScanLine    = This->Mode->Info->PixelsPerScanLine;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
DisplaySetMode(IN EFI_GRAPHICS_OUTPUT_PROTOCOL *This, IN UINT32 ModeNumber)
{
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
DisplayBlt(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL *            BltBuffer,
    OPTIONAL IN EFI_GRAPHICS_OUTPUT_BLT_OPERATION BltOperation,
    IN UINTN SourceX, IN UINTN SourceY, IN UINTN DestinationX,
    IN UINTN DestinationY, IN UINTN Width, IN UINTN Height,
    IN UINTN Delta OPTIONAL)
{
  RETURN_STATUS Status;
  EFI_TPL       Tpl;
  UINT32       *Framebuffer;
  UINT32        ScreenWidth;
  UINT32       *StartPixel;

  __builtin_prefetch((const void *)(UINTN)mDisplay.Mode->FrameBufferBase, 1, 3);

  //
  // We have to raise to TPL_NOTIFY, so we make an atomic write to the frame
  // buffer. We would not want a timer based event (Cursor, ...) to come in
  // while we are doing this operation.
  //
  Tpl = gBS->RaiseTPL(TPL_NOTIFY);

  if (BltOperation == EfiBltBufferToVideo && BltBuffer != NULL) {
    UINTN   ActualDelta = (Delta != 0) ? Delta : Width * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
    UINT32 *Pixel32 = (UINT32*)BltBuffer;
    UINTN   PixelsPerRow = ActualDelta / sizeof(UINT32);

    __builtin_prefetch(Pixel32, 1, 2);

    if (Width >= 8) {
      uint32x4_t AlphaMask = vdupq_n_u32(0xFF000000);

      for (UINTN y = 0; y < Height; y++) {
        UINT32 *RowStart = Pixel32 + (y * PixelsPerRow);
        UINTN x = 0;

        if (y + 1 < Height) {
          __builtin_prefetch(Pixel32 + ((y + 1) * PixelsPerRow), 1, 2);
        }

        for (; x + 31 < Width; x += 32) {
          uint32x4_t p1 = vld1q_u32(RowStart + x);
          uint32x4_t p2 = vld1q_u32(RowStart + x + 4);
          uint32x4_t p3 = vld1q_u32(RowStart + x + 8);
          uint32x4_t p4 = vld1q_u32(RowStart + x + 12);
          uint32x4_t p5 = vld1q_u32(RowStart + x + 16);
          uint32x4_t p6 = vld1q_u32(RowStart + x + 20);
          uint32x4_t p7 = vld1q_u32(RowStart + x + 24);
          uint32x4_t p8 = vld1q_u32(RowStart + x + 28);

          p1 = vorrq_u32(p1, AlphaMask);
          p2 = vorrq_u32(p2, AlphaMask);
          p3 = vorrq_u32(p3, AlphaMask);
          p4 = vorrq_u32(p4, AlphaMask);
          p5 = vorrq_u32(p5, AlphaMask);
          p6 = vorrq_u32(p6, AlphaMask);
          p7 = vorrq_u32(p7, AlphaMask);
          p8 = vorrq_u32(p8, AlphaMask);

          vst1q_u32(RowStart + x, p1);
          vst1q_u32(RowStart + x + 4, p2);
          vst1q_u32(RowStart + x + 8, p3);
          vst1q_u32(RowStart + x + 12, p4);
          vst1q_u32(RowStart + x + 16, p5);
          vst1q_u32(RowStart + x + 20, p6);
          vst1q_u32(RowStart + x + 24, p7);
          vst1q_u32(RowStart + x + 28, p8);
        }

        for (; x + 3 < Width; x += 4) {
          uint32x4_t pixels = vld1q_u32(RowStart + x);
          pixels = vorrq_u32(pixels, AlphaMask);
          vst1q_u32(RowStart + x, pixels);
        }

        for (; x < Width; x++) {
          RowStart[x] |= 0xFF000000;
        }
      }
    } else {
      for (UINTN y = 0; y < Height; y++) {
        UINT32 *RowStart = Pixel32 + (y * PixelsPerRow);
        UINT64 *Row64 = (UINT64*)RowStart;
        UINT64  AlphaMask64 = 0xFF000000FF000000ULL;
        UINTN   Pairs = Width / 2;

        UINTN j = 0;
        for (; j + 3 < Pairs; j += 4) {
          Row64[j]     |= AlphaMask64;
          Row64[j + 1] |= AlphaMask64;
          Row64[j + 2] |= AlphaMask64;
          Row64[j + 3] |= AlphaMask64;
        }
        for (; j < Pairs; j++) {
          Row64[j] |= AlphaMask64;
        }

        if (Width & 1) {
          RowStart[Width - 1] |= 0xFF000000;
        }
      }
    }
  }

  Status = FrameBufferBlt(
      mFrameBufferBltLibConfigure, BltBuffer, BltOperation, SourceX, SourceY,
      DestinationX, DestinationY, Width, Height, Delta);

  gBS->RestoreTPL(Tpl);

  if (BltOperation != EfiBltBufferToVideo) {
    Framebuffer = (UINT32 *)mDisplay.Mode->FrameBufferBase;
    ScreenWidth = mDisplay.Mode->Info->HorizontalResolution;
    StartPixel = &Framebuffer[DestinationY * ScreenWidth + DestinationX];

    BOOLEAN IsAligned = ((UINTN)StartPixel & 15) == 0;

    if (Width >= 32 && Height >= 4) {
      uint32x4_t AlphaMask = vdupq_n_u32(0xFF000000);

      for (UINTN y = 0; y < Height; y += 1) {
        UINT32 *CurrentPixel = StartPixel + (y * ScreenWidth);
        UINTN x = 0;

        if (y + 1 < Height) {
          __builtin_prefetch(StartPixel + ((y + 1) * ScreenWidth), 1, 3);
        }
        if (y + 2 < Height) {
          __builtin_prefetch(StartPixel + ((y + 2) * ScreenWidth), 1, 2);
        }

        for (; x + 63 < Width; x += 64) {
          uint32x4_t p1 = vld1q_u32(CurrentPixel + x);
          uint32x4_t p2 = vld1q_u32(CurrentPixel + x + 4);
          uint32x4_t p3 = vld1q_u32(CurrentPixel + x + 8);
          uint32x4_t p4 = vld1q_u32(CurrentPixel + x + 12);
          uint32x4_t p5 = vld1q_u32(CurrentPixel + x + 16);
          uint32x4_t p6 = vld1q_u32(CurrentPixel + x + 20);
          uint32x4_t p7 = vld1q_u32(CurrentPixel + x + 24);
          uint32x4_t p8 = vld1q_u32(CurrentPixel + x + 28);
          uint32x4_t p9 = vld1q_u32(CurrentPixel + x + 32);
          uint32x4_t p10 = vld1q_u32(CurrentPixel + x + 36);
          uint32x4_t p11 = vld1q_u32(CurrentPixel + x + 40);
          uint32x4_t p12 = vld1q_u32(CurrentPixel + x + 44);
          uint32x4_t p13 = vld1q_u32(CurrentPixel + x + 48);
          uint32x4_t p14 = vld1q_u32(CurrentPixel + x + 52);
          uint32x4_t p15 = vld1q_u32(CurrentPixel + x + 56);
          uint32x4_t p16 = vld1q_u32(CurrentPixel + x + 60);

          p1 = vorrq_u32(p1, AlphaMask);
          p2 = vorrq_u32(p2, AlphaMask);
          p3 = vorrq_u32(p3, AlphaMask);
          p4 = vorrq_u32(p4, AlphaMask);
          p5 = vorrq_u32(p5, AlphaMask);
          p6 = vorrq_u32(p6, AlphaMask);
          p7 = vorrq_u32(p7, AlphaMask);
          p8 = vorrq_u32(p8, AlphaMask);
          p9 = vorrq_u32(p9, AlphaMask);
          p10 = vorrq_u32(p10, AlphaMask);
          p11 = vorrq_u32(p11, AlphaMask);
          p12 = vorrq_u32(p12, AlphaMask);
          p13 = vorrq_u32(p13, AlphaMask);
          p14 = vorrq_u32(p14, AlphaMask);
          p15 = vorrq_u32(p15, AlphaMask);
          p16 = vorrq_u32(p16, AlphaMask);

          vst1q_u32(CurrentPixel + x, p1);
          vst1q_u32(CurrentPixel + x + 4, p2);
          vst1q_u32(CurrentPixel + x + 8, p3);
          vst1q_u32(CurrentPixel + x + 12, p4);
          vst1q_u32(CurrentPixel + x + 16, p5);
          vst1q_u32(CurrentPixel + x + 20, p6);
          vst1q_u32(CurrentPixel + x + 24, p7);
          vst1q_u32(CurrentPixel + x + 28, p8);
          vst1q_u32(CurrentPixel + x + 32, p9);
          vst1q_u32(CurrentPixel + x + 36, p10);
          vst1q_u32(CurrentPixel + x + 40, p11);
          vst1q_u32(CurrentPixel + x + 44, p12);
          vst1q_u32(CurrentPixel + x + 48, p13);
          vst1q_u32(CurrentPixel + x + 52, p14);
          vst1q_u32(CurrentPixel + x + 56, p15);
          vst1q_u32(CurrentPixel + x + 60, p16);
        }

        for (; x + 15 < Width; x += 16) {
          uint32x4_t p1 = vld1q_u32(CurrentPixel + x);
          uint32x4_t p2 = vld1q_u32(CurrentPixel + x + 4);
          uint32x4_t p3 = vld1q_u32(CurrentPixel + x + 8);
          uint32x4_t p4 = vld1q_u32(CurrentPixel + x + 12);

          p1 = vorrq_u32(p1, AlphaMask);
          p2 = vorrq_u32(p2, AlphaMask);
          p3 = vorrq_u32(p3, AlphaMask);
          p4 = vorrq_u32(p4, AlphaMask);

          vst1q_u32(CurrentPixel + x, p1);
          vst1q_u32(CurrentPixel + x + 4, p2);
          vst1q_u32(CurrentPixel + x + 8, p3);
          vst1q_u32(CurrentPixel + x + 12, p4);
        }

        for (; x + 3 < Width; x += 4) {
          uint32x4_t pixels = vld1q_u32(CurrentPixel + x);
          pixels = vorrq_u32(pixels, AlphaMask);
          vst1q_u32(CurrentPixel + x, pixels);
        }

        for (; x < Width; x++) {
          CurrentPixel[x] |= 0xFF000000;
        }
      }
    } else {
      for (UINTN y = 0; y < Height; y++) {
        UINT32 *CurrentPixel = StartPixel + (y * ScreenWidth);
        UINT64 *Row64 = (UINT64*)CurrentPixel;
        UINT64  AlphaMask64 = 0xFF000000FF000000ULL;
        UINTN   Pairs = Width / 2;
        UINTN j = 0;

        for (; j + 15 < Pairs; j += 16) {
          Row64[j]      |= AlphaMask64; Row64[j + 1]  |= AlphaMask64;
          Row64[j + 2]  |= AlphaMask64; Row64[j + 3]  |= AlphaMask64;
          Row64[j + 4]  |= AlphaMask64; Row64[j + 5]  |= AlphaMask64;
          Row64[j + 6]  |= AlphaMask64; Row64[j + 7]  |= AlphaMask64;
          Row64[j + 8]  |= AlphaMask64; Row64[j + 9]  |= AlphaMask64;
          Row64[j + 10] |= AlphaMask64; Row64[j + 11] |= AlphaMask64;
          Row64[j + 12] |= AlphaMask64; Row64[j + 13] |= AlphaMask64;
          Row64[j + 14] |= AlphaMask64; Row64[j + 15] |= AlphaMask64;
        }

        for (; j < Pairs; j++) {
          Row64[j] |= AlphaMask64;
        }

        if (Width & 1) {
          CurrentPixel[Width - 1] |= 0xFF000000;
        }
      }
    }
  }

  return RETURN_ERROR(Status) ? EFI_INVALID_PARAMETER : EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SimpleFbDxeInitialize(
    IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{

  EFI_STATUS Status             = EFI_SUCCESS;
  EFI_HANDLE hUEFIDisplayHandle = NULL;

  /* Retrieve simple frame buffer from pre-SEC bootloader */
  DEBUG(
      (EFI_D_ERROR,
       "SimpleFbDxe: Retrieve MIPI FrameBuffer parameters from PCD\n"));
  UINT32 MipiFrameBufferAddr   = FixedPcdGet32(PcdMipiFrameBufferAddress);
  UINT32 MipiFrameBufferWidth  = FixedPcdGet32(PcdMipiFrameBufferWidth);
  UINT32 MipiFrameBufferHeight = FixedPcdGet32(PcdMipiFrameBufferHeight);
  UINT32 MipiFrameBufferLayers = FixedPcdGet32(PcdMipiFrameBufferLayers);

  /* Sanity check */
  if (MipiFrameBufferAddr == 0 || MipiFrameBufferWidth == 0 ||
      MipiFrameBufferHeight == 0) {
    DEBUG((EFI_D_ERROR, "SimpleFbDxe: Invalid FrameBuffer parameters\n"));
    return EFI_DEVICE_ERROR;
  }

  /* Prepare struct */
  if (mDisplay.Mode == NULL) {
    Status = gBS->AllocatePool(
        EfiBootServicesData, sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE),
        (VOID **)&mDisplay.Mode);

    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
      return Status;

    ZeroMem(mDisplay.Mode, sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE));
  }

  if (mDisplay.Mode->Info == NULL) {
    Status = gBS->AllocatePool(
        EfiBootServicesData, sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION),
        (VOID **)&mDisplay.Mode->Info);

    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
      return Status;

    ZeroMem(mDisplay.Mode->Info, sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION));
  }

  /* Set information */
  mDisplay.Mode->MaxMode       = 1;
  mDisplay.Mode->Mode          = 0;
  mDisplay.Mode->Info->Version = 0;

  mDisplay.Mode->Info->HorizontalResolution = MipiFrameBufferWidth;
  mDisplay.Mode->Info->VerticalResolution   = MipiFrameBufferHeight;

  /* SimpleFB runs on a8r8g8b8 (VIDEO_BPP32) for DB410c */
  UINT32               LineLength = MipiFrameBufferWidth * VNBYTES(VIDEO_BPP32);
  UINT32               FrameBufferSize    = LineLength * MipiFrameBufferHeight;
  EFI_PHYSICAL_ADDRESS FrameBufferAddress = MipiFrameBufferAddr;

  mDisplay.Mode->Info->PixelsPerScanLine = MipiFrameBufferWidth;
  mDisplay.Mode->Info->PixelFormat = PixelBlueGreenRedReserved8BitPerColor;
  mDisplay.Mode->SizeOfInfo      = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
  mDisplay.Mode->FrameBufferBase = FrameBufferAddress;
  mDisplay.Mode->FrameBufferSize = FrameBufferSize;

  //
  // Create the FrameBufferBltLib configuration.
  //
  Status = FrameBufferBltConfigure(
      (VOID *)(UINTN)mDisplay.Mode->FrameBufferBase, mDisplay.Mode->Info,
      mFrameBufferBltLibConfigure, &mFrameBufferBltLibConfigureSize);
  if (Status == RETURN_BUFFER_TOO_SMALL) {
    mFrameBufferBltLibConfigure = AllocatePool(mFrameBufferBltLibConfigureSize);
    if (mFrameBufferBltLibConfigure != NULL) {
      Status = FrameBufferBltConfigure(
          (VOID *)(UINTN)mDisplay.Mode->FrameBufferBase, mDisplay.Mode->Info,
          mFrameBufferBltLibConfigure, &mFrameBufferBltLibConfigureSize);
    }
  }
  ASSERT_EFI_ERROR(Status);

  // zhuowei: clear the screen to black
  // UEFI standard requires this, since text is white - see
  // OvmfPkg/QemuVideoDxe/Gop.c
  for (UINT32 i = 0; i <= MipiFrameBufferLayers; i++) {
    ZeroMem((void *)(FrameBufferAddress + (FrameBufferSize * i)), FrameBufferSize);
  }

  /* Register handle */
  Status = gBS->InstallMultipleProtocolInterfaces(
      &hUEFIDisplayHandle, &gEfiDevicePathProtocolGuid, &mDisplayDevicePath,
      &gEfiGraphicsOutputProtocolGuid, &mDisplay, NULL);

  ASSERT_EFI_ERROR(Status);

  return Status;
}
