#pragma once

// symbols defined in components/esp8266/ld/esp8266.project.ld.in, must be updated if changed
extern char _iram_start, _iram_end;       // .iram0.*
extern char _iram_text_start, _iram_text_end; // .iram0.text
extern char _iram_bss_start, _iram_bss_end;   // .iram0.bss
extern char _data_start, _data_end;       // .dram0.data
extern char _bss_start, _bss_end;         // .dram0.bss
extern char _text_start, _text_end;       // .flash.text
extern char _rodata_start, _rodata_end;   // .flash.rodata
