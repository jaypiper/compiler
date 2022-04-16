ENTRY(_start)

SECTIONS {
  /* _pmem_start and _entry_offset are defined in LDFLAGS */
  . = 0x80000000;
  .text : {
    *(.text*)
  }
/*  . = _ram_start; */
  .rodata : {
    *(.rodata*)
  }
  .data : {
    *(.data)
  }
  edata = .;
  _data = .;
  .bss : {
	_bss_start = .;
    *(.bss*)
    *(.sbss*)
    *(.scommon)
  }
  _stack_top = ALIGN(0x1000);
  . = _stack_top + 0x8000;
  /*_stack_pointer = .;*/
  end = .;
  _end = .;
  _heap_start = ALIGN(0x1000);
}
