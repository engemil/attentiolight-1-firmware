# al1_link_driver (STM32 side)

STM32C071 runtime for the wireless-module UART link to the ESP32-C3.

- `al1_frame.{c,h}` and `crc16_ccitt.{c,h}` are **identical copies** of the ESP32
  component (`attentiolight-1-wireless-module-firmware/fw_al1_wmod/components/al1_link/`).
  Keep them byte-identical so the two ends agree on the wire. The full protocol
  spec lives in that component's `README.md`.
- `al1_link_driver.{c,h}` is the ChibiOS-specific part: it carries the frames
  over **USART1 / `SD1`** (PB6 = USART1_TX, PB7 = USART1_RX, **AF0**, see board.h),
  at **921600 8N1**.

The link multiplexes opaque channels (`AP_CTRL`, `LOG`, `EVT`, `BULK`,
`KEEPALIVE`). The Attentio Protocol rides inside `AP_CTRL` and is not parsed
here; the AP↔MICB bridge is a later phase.

Constrained by the C071's 24 KB SRAM, `AL1_LINK_MAX_PAYLOAD` is overridden down
from the wire maximum (4096) via `UDEFS` in the Makefile.
