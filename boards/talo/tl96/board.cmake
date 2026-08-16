# SPDX-License-Identifier: MIT

board_runner_args(dfu-util "--pid=0x9632" "--vid=0x544C" "--alt=0")
include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)