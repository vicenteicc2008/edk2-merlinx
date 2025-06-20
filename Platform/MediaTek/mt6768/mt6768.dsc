## @file
#
#  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
#  Copyright (c) 2014, Linaro Limited. All rights reserved.
#  Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.
#  Copyright (c) 2018 - 2019, Bingxing Wang. All rights reserved.
#  Copyright (c) 2022, Xilin Wu. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################

[Defines]
  SOC_PLATFORM            = mt6768
  USE_PHYSICAL_TIMER      = TRUE

!include Silicon/MediaTek/MediaTekPkg/MediaTekCommonDsc.inc

[PcdsFixedAtBuild.common]
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x40000000
  gArmTokenSpaceGuid.PcdSystemMemorySize|0xE00000000

  gArmTokenSpaceGuid.PcdCpuVectorBaseAddress|0x40C40000     # CPU Vectors
  gArmTokenSpaceGuid.PcdArmArchTimerFreqInHz|13000000
  gArmTokenSpaceGuid.PcdArmArchTimerSecIntrNum|13
  gArmTokenSpaceGuid.PcdArmArchTimerIntrNum|14
  gArmTokenSpaceGuid.PcdArmArchTimerVirtIntrNum|11
  gArmTokenSpaceGuid.PcdArmArchTimerHypIntrNum|10
  gArmTokenSpaceGuid.PcdGicDistributorBase|0x0c000000
  gArmTokenSpaceGuid.PcdGicRedistributorsBase|0x0c400000
  gArmTokenSpaceGuid.PcdGicInterruptInterfaceBase|0x0c400000

  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultOemRevision|0x00000850
  gEmbeddedTokenSpaceGuid.PcdPrePiStackBase|0x40C00000      # UEFI Stack
  gEmbeddedTokenSpaceGuid.PcdPrePiStackSize|0x00040000      # 256K stack
  #gEmbeddedTokenSpaceGuid.PcdPrePiCpuIoSize|44

  gMediaTekTokenSpaceGuid.PcdUefiMemPoolBase|0x40C50000         # DXE Heap base address
  gMediaTekTokenSpaceGuid.PcdUefiMemPoolSize|0x0C1B0000         # UefiMemorySize, DXE heap size

  gMediaTekTokenSpaceGuid.PcdMipiFrameBufferLayers|2
  gMediaTekTokenSpaceGuid.PcdWatchdogWorkingTimer|0

  gArmPlatformTokenSpaceGuid.PcdCoreCount|8
  gArmPlatformTokenSpaceGuid.PcdClusterCount|1

  #
  # SimpleInit
  #
  gSimpleInitTokenSpaceGuid.PcdDeviceTreeStore|0x40000000
  gSimpleInitTokenSpaceGuid.PcdLoggerdUseConsole|FALSE

[LibraryClasses.common]
  GpioLib|Silicon/MediaTek/MediaTekPkg/Library/GpioLib/GpioLib.inf
  KeypadDeviceImplLib|Silicon/MediaTek/MediaTekPkg/Library/KeypadDeviceImplLib/KeypadDeviceImplLib.inf
  MtkGpioImplLib|Silicon/MediaTek/MT6768Pkg/Library/MtkGpioImplLib/MtkGpioImplLib.inf
  PmicWrapperImplLib|Silicon/MediaTek/MT6768Pkg/Library/PmicWrapperImplLib/PmicWrapperImplLib.inf
  PlatformMemoryMapLib|Silicon/MediaTek/MT6768Pkg/Library/PlatformMemoryMapLib/PlatformMemoryMapLib.inf
  PlatformPeiLib|Silicon/MediaTek/MT6768Pkg/Library/PlatformPeiLib/PlatformPeiLib.inf
  PlatformPrePiLib|Silicon/MediaTek/MT6768Pkg/Library/PlatformPrePiLib/PlatformPrePiLib.inf
  MsPlatformDevicesLib|Silicon/MediaTek/MT6768Pkg/Library/MsPlatformDevicesLib/MsPlatformDevicesLib.inf
  SOCSmbiosInfoLib|Silicon/MediaTek/MT6768Pkg/Library/SOCSmbiosInfoLib/SOCSmbiosInfoLib.inf

[Components.common]
  Silicon/MediaTek/MediaTekPkg/Drivers/MtkGpioDxe/MtkGpioDxe.inf
  Silicon/MediaTek/MediaTekPkg/Drivers/MT6358PmicDxe/MT6358PmicDxe.inf
  EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf