#ifndef __MTK_GPIO_H__
#define __MTK_GPIO_H__
//
// Protocol interface structure
//
typedef struct _MTK_GPIO MTK_GPIO;

//
// Data Types
//
typedef enum {
  GPIO_MODE_INPUT              = 0,
  GPIO_MODE_OUTPUT             = 8,
  GPIO_MODE_SPECIAL_FUNCTION_1 = 1,
  GPIO_MODE_SPECIAL_FUNCTION_2 = 2,
  GPIO_MODE_SPECIAL_FUNCTION_3 = 3,
  GPIO_MODE_SPECIAL_FUNCTION_4 = 4,
  GPIO_MODE_SPECIAL_FUNCTION_5 = 5,
  GPIO_MODE_SPECIAL_FUNCTION_6 = 6,
  GPIO_MODE_SPECIAL_FUNCTION_7 = 7
} MTK_GPIO_MODE;

typedef enum {
  MTK_GPIO_PULL_UP,
  MTK_GPIO_PULL_DOWN
} MTK_GPIO_PULL;

typedef struct {
  UINT32 BaseAddr;
  UINT32 SetOffset;
  UINT32 ResetOffset;
  UINT32 DirOffset;
  UINT32 DataOutOffset;
  UINT32 DataInOffset;
  UINT32 ModeOffset;
  UINT32 MaxPin;
} MTK_GPIO_PLATFORM_INFO;

struct MTK_GPIO_DRV_TABLE {
  UINT32 Pin;
  UINT32 Base;
  UINT32 Mask;
  UINT32 Shift; 
};

struct MTK_GPIO_R_TABLE {
  UINT32 Pin; 
  UINT32 R0Base;
  UINT32 R1Base;
  UINT32 R0Shift; 
  UINT32 R1Shift;
};

struct MTK_GPIO_PUPD_TABLE {
  UINT32 Pin;
  UINT32 Base;
  UINT32 Shift;
};

//
// Function Prototypes
//
typedef
VOID
(EFIAPI *MTK_GPIO_GET)(
  IN  UINT32         Pin,
  OUT BOOLEAN       *Status
  );

/*++

Routine Description:

  Gets the state of a GPIO pin

Arguments:

  Pin    - which pin to read
  Status - pointer to output state of the pin

--*/

typedef
VOID
(EFIAPI *MTK_GPIO_SET)(
  IN UINT32        Pin,
  IN BOOLEAN       Status
  );

/*++

Routine Description:

  Sets the state of a GPIO pin

Arguments:

  Pin     - which pin to modify
  Status  - state of the pin

--*/

typedef
VOID
(EFIAPI *MTK_GPIO_GET_DIR)(
  IN  UINT32          Pin,
  OUT BOOLEAN        *Direction
  );

/*++

Routine Description:

  Gets the direction of a GPIO pin

Arguments:

  Pin        - which pin
  Direction  - pointer to output mode value (FALSE: Output, TRUE: Input)

--*/

typedef
VOID
(EFIAPI *MTK_GPIO_SET_DIR)(
  IN  UINT32         Pin,
  IN  BOOLEAN        Direction
  );

/*++

Routine Description:

  Sets the direction of a GPIO pin

Arguments:

  Pin       - which pin
  Direction - FALSE: Output, TRUE: Input

--*/

typedef
VOID
(EFIAPI *MTK_GPIO_SET_MODE)(
  IN  UINT32         Pin,
  IN  MTK_GPIO_MODE  Mode
  );

/*++

Routine Description:

  Sets the mode of a GPIO pin

Arguments:

  Pin       - which pin
  Func      - which function

--*/

typedef
VOID
(EFIAPI *MTK_GPIO_SET_DRV)(
  UINT32 Pin,
  UINT32 Drv
  );

typedef
VOID
(EFIAPI *MTK_GPIO_SET_R)(
  UINT32 Pin
  );

typedef
VOID
(EFIAPI *MTK_GPIO_SET_PUPD)(
  UINT32  Pin,
  BOOLEAN IsPullDown
  );

struct _MTK_GPIO {
  MTK_GPIO_GET              Get;
  MTK_GPIO_SET              Set;
  MTK_GPIO_GET_DIR          GetDir;
  MTK_GPIO_SET_DIR          SetDir;
  MTK_GPIO_SET_MODE         SetMode;
  MTK_GPIO_SET_DRV          SetDrv;
  MTK_GPIO_SET_R            SetR0;
  MTK_GPIO_SET_R            SetR1;
  MTK_GPIO_SET_PUPD         SetPupd;
};

extern EFI_GUID  gMtkGpioProtocolGuid;

#endif
