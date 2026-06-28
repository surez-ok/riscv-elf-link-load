SECTIONS {
    . = 0x10000; 

    .data : { *(.data) }

    .text : { *(.text) }
}
