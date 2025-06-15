//
// MT6768 PWRAP Configuration
//

#include <Protocol/MtkPmicWrapper.h>

MTK_PWRAP_REG PwrapRegs[] = {
  0x0,  // INIT_DONE2
  0xC20, // WACS2_CMD
  0xC28, // WACS2_VLDCLR
  0xC24, // WACS2_RDATA
};

MTK_PWRAP_PLATFORM_INFO PlatformInfo = {
  0x1000d000, // Base
  PwrapRegs,
  TRUE        // Have ARB Capability
};
