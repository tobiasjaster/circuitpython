LD_FILE = $(FLASH).ld $(CHIP_FAMILY).ld imxrt10xx.ld

ifeq ($(LONGINT_IMPL),NONE)
MPY_TOOL_LONGINT_IMPL = -mlongint-impl=none
endif

ifeq ($(LONGINT_IMPL),MPZ)
MPY_TOOL_LONGINT_IMPL = -mlongint-impl=mpz
endif

ifeq ($(LONGINT_IMPL),LONGLONG)
MPY_TOOL_LONGINT_IMPL = -mlongint-impl=longlong
endif

INTERNAL_LIBM = 1

USB_HIGHSPEED = 1

# Number of USB endpoint pairs.
USB_NUM_EP = 8

INTERNAL_FLASH_FILESYSTEM = 1

CIRCUITPY_AUDIOIO = 0
CIRCUITPY_AUDIOBUSIO = 0
CIRCUITPY_FREQUENCYIO = 0
CIRCUITPY_I2CPERIPHERAL = 0
CIRCUITPY_NVM = 0
CIRCUITPY_ROTARYIO = 0
CIRCUITPY_COUNTIO = 0
CIRCUITPY_USB_MIDI = 1
LONGINT_IMPL = MPZ
