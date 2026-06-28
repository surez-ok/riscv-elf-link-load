SECTIONS {
    . = 0x10000; 
    .text : {
        b.o(.text)
        a.o(.text)
    }
    
    .data : { *(.data) }
}
