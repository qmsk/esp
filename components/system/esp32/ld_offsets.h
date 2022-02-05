#pragma once


// External flash (Data)
static const unsigned drom0_org = 0x3F400000;

// Internal SRAM 2 (Data)
static const unsigned dram0_org = 0x3FFAE000;
static const size_t dram0_size = 0x32000;

// Internal SRAM 1 (Data)
static const unsigned dram1_org = 0x3FFE0000;
static const size_t dram1_size = 0x20000;

// Internal SRAM 0 (Instruction)
static const unsigned iram0_org = 0x40080000;
static const size_t iram0_size = 0x20000;

// External Flash (Instruction))
static const unsigned irom0_org = 0x400C2000;

// symbols defined in components/esp_system/ld/esp32/memory.ld.in

extern int _static_data_end;             // .dram0
extern int _heap_end;                    // .dram1

// symbols defined in components/esp_system/ld/esp32/sections.ld.in
extern int _data_start, _data_end;       // .dram0.data
extern int _noinit_start, _noinit_end;   // .dram0.noinit
extern int _bss_start, _bss_end;         // .dram0.bss
extern int _heap_start;                  // .dram0

extern int _rodata_start, _rodata_end;  // drom0_0_seg .flash.appdesc, flash.rodata (Data / External Flash)
extern int _text_start, _text_end;      // iram0_2_seg .flash.text (Instruction / External Flash)

extern int _iram_start, _iram_end;            // iram0_0_seg .iram0.*
extern int _vector_table, _init_end;          // iram0_0_sgg .iram0.vectors
extern int _iram_text_start, _iram_text_end;  // iram0_0_seg .iram0.text
extern int _iram_data_start, _iram_data_end;  // iram0_0_seg .iram0.data
extern int _iram_bss_start, _iram_bss_end;    // iram0_0_seg .iram0.bss
