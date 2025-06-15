#ifndef __MTK_PWRAP_H__
#define __MTK_PWRAP_H__
//
// Protocol interface structure
//

typedef struct _MTK_PWRAP MTK_PWRAP;

typedef enum {
  PWRAP_INIT_DONE2,
  PWRAP_WACS2_CMD,
  PWRAP_WACS2_VLDCLR,
  PWRAP_WACS2_RDATA,
  PWRAP_SWINF_2_WDATA_31_0,
  PWRAP_SWINF_2_RDATA_31_0,
  PWRAP_REG_MAX,
} MTK_PWRAP_REG;

typedef struct {
  UINT32         BaseAddr;
  MTK_PWRAP_REG *RegMap;
  BOOLEAN        ArbCap;
} MTK_PWRAP_PLATFORM_INFO;

//
// Function Prototypes
//

typedef
VOID
(EFIAPI *MTK_PWRAP_READ16)(
  IN  UINT16 Address,
  OUT UINT16 *Value
  );

typedef
VOID
(EFIAPI *MTK_PWRAP_WRITE16)(
  IN UINT16 Address,
  IN UINT16 Data
);

struct _MTK_PWRAP {
  MTK_PWRAP_READ16          Read16;
  MTK_PWRAP_WRITE16         Write16;
};

extern EFI_GUID gMediaTekPmicWrapperProtocolGuid;

#endif
