# ──────────────────────────────────────────────────────────────
# Board fragment: WT32-SC01
#   ESP32-WROVER-B · 3.5" 480x320 ST7796 SPI LCD · FT6336 touch
#   DS18B20 1-Wire probes on GPIO4 · no user LED (MockLed bound)
#
# A board fragment may append to BOARD_SOURCES (extra .cpp files under this
# folder that need compiling). Component deps are NOT set here — see the note
# in main/CMakeLists.txt: managed deps go in main/idf_component.yml, IDF
# built-ins in COMPONENT_REQUIRES.
#
# The panel driver lives in this folder rather than hardware/drivers/ because
# it is not board-independent: it *is* this board's panel, wired one way, and
# no sibling board would share it.
# ──────────────────────────────────────────────────────────────

list(APPEND BOARD_SOURCES "${CMAKE_CURRENT_LIST_DIR}/BoardContext.cpp")
list(APPEND BOARD_SOURCES "${CMAKE_CURRENT_LIST_DIR}/Display_WT32SC01.cpp")
