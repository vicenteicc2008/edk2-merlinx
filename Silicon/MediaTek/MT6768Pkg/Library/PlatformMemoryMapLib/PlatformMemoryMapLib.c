#include <Library/BaseLib.h>
#include <Library/PlatformMemoryMapLib.h>

static ARM_MEMORY_REGION_DESCRIPTOR_EX gDeviceMemoryDescriptorEx[] = {
/*                                                    EFI_RESOURCE_ EFI_RESOURCE_ATTRIBUTE_ EFI_MEMORY_TYPE ARM_REGION_ATTRIBUTE_
     MemLabel(32 Char.),  MemBase,    MemSize, BuildHob, ResourceType, ResourceAttribute, MemoryType, CacheAttributes
--------------------- Register ---------------------*/
    {"Periphs",           0x00000000, 0x1B000000,  AddMem, MEM_RES, UNCACHEABLE,  RtCode,   NS_DEVICE},

//--------------------- DDR --------------------- */

    {"HLOS 0",            0x40000000, 0x00C00000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv,   WRITE_BACK_XN},
    {"UEFI Stack",        0x40C00000, 0x00040000, AddMem, SYS_MEM, SYS_MEM_CAP, BsData, WRITE_BACK},
    {"CPU Vectors",       0x40C40000, 0x00010000, AddMem, SYS_MEM, SYS_MEM_CAP, BsCode, WRITE_BACK},
    {"HLOS 1",            0x40C50000, 0x0C1B0000, AddMem, SYS_MEM, SYS_MEM_CAP, Conv,   WRITE_BACK},
    {"ATF Reserved",      0x4CE00000, 0x00060000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
    {"UEFI FD",           0x50000000, 0x00700000, AddMem, SYS_MEM, SYS_MEM_CAP, BsCode, WRITE_BACK},


    {"TEE Reserved",      0x70000000, 0x05000000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},
    {"Display Reserved",  0x7D9B0000, 0x02250000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_THROUGH_XN},
    {"SSPM Reserved",     0x7FF00000, 0x000C0000, AddMem, MEM_RES, SYS_MEM_CAP, Reserv, WRITE_BACK},

//------------------- Terminator for MMU ---------------------
{"Terminator", 0, 0, 0, 0, 0, 0, 0}};

ARM_MEMORY_REGION_DESCRIPTOR_EX *GetPlatformMemoryMap()
{
  return gDeviceMemoryDescriptorEx;
}
