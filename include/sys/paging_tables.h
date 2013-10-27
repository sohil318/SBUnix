enum PAGE_FLAGS {

    I86_PRESENT         =   1,                                //00000000000000000000000000000000 00000000000000000000000000000001
    I86_WRITABLE        =   2,                                //00000000000000000000000000000000 00000000000000000000000000000010
    I86_USER            =   4,                                //00000000000000000000000000000000 00000000000000000000000000000100
    I86_PWT             =   8,                                //00000000000000000000000000000000 00000000000000000000000000001000
    I86_PCD             =   0x10,                             //00000000000000000000000000000000 00000000000000000000000000010000
    I86_ACCESSED        =   0x20,                             //00000000000000000000000000000000 00000000000000000000000000100000
    I86_IGN             =   0x40,                             //00000000000000000000000000000000 00000000000000000000000001000000
    I86_ZERO            =   0,                                //00000000000000000000000000000000 00000000000000000000000000000000
    I86_ING2            =   0x100,                            //00000000000000000000000000000000 00000000000000000000000100000000
    I86_MBZ_PML4        =   0x180,                            //00000000000000000000000000000000 00000000000000000000000110000000
    I86_AVL             =   0xD00,                            //00000000000000000000000000000000 00000000000000000000111000000000
    I86_ADDR            =   0x000FFFFFFFFFF000,               //00000000000011111111111111111111 11111111111111111111000000000000
    I86_AVAILABLE       =   0x7FF0000000000000,               //01111111111100000000000000000000 00000000000000000000000000000000
    I86_NX              =   0x8000000000000000,               //10000000000000000000000000000000 00000000000000000000000000000000
};
