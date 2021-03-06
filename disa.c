#include <string.h>
#include "cpu68.h"

char *DesaPtr ;

static void cat_hexa_nibble(int val)
{
        if (val<10)
                *DesaPtr++ = val+48 ;
        else
                *DesaPtr++ = val+65-10 ;
}

static void cat_hexa_byte(int val)
{
        *DesaPtr++ = '$' ;
        cat_hexa_nibble((val>>4)&15) ;
        cat_hexa_nibble(val&15) ;
}

static void cat_hexa_word(int val)
{
        int i ;
        *DesaPtr++ = '$' ;
        for (i=0;i<4;i++)
        {
                cat_hexa_nibble((val>>12)&15) ;
                val = val << 4 ;
        }
}

static void cat_hexa_long(int val)
{
        int i ;
        *DesaPtr++ = '$' ;
        for (i=0;i<8;i++)
        {
                cat_hexa_nibble((val>>28)&15) ;
                val = val << 4 ;
        }
}

///////////////////////////////////////////////////////////// Traitements EA


static void cat_size(int siz)
{
        *DesaPtr++ = '.' ;
        switch(siz) {
                case 0 : *DesaPtr++ = 'B' ; break ;
                case 1 : *DesaPtr++ = 'W' ; break ;
                case 2 : *DesaPtr++ = 'L' ; break ;
                case 3 : *DesaPtr++ = '?' ;
        }
        *DesaPtr++ = ' ' ;
}

static void cat_size_wl(int siz)
{
        *DesaPtr++ = '.' ;
        switch(siz) {
                case 0 : *DesaPtr++ = 'W' ; break ;
                case 1 : *DesaPtr++ = 'L' ; break ;
//                         *DesaPtr++ = '?' ;
        }
        *DesaPtr++ = ' ' ;
}

static void cat_Dreg(int reg)
{
        *DesaPtr++ = 'D' ;
        *DesaPtr++ = reg+0x30 ;
}

static void cat_Areg(int reg)
{
        *DesaPtr++ = 'A' ;
        *DesaPtr++ = reg+0x30 ;
}

static void cat_Aind(int reg)
{
        *DesaPtr++ = '(' ;
        *DesaPtr++ = 'A' ;
        *DesaPtr++ = reg+0x30 ;
        *DesaPtr++ = ')' ;
}

static void cat_Aipi(int reg)
{
        cat_Aind(reg) ;
        *DesaPtr++ = '+' ;
}

static void cat_Aipd(int reg)
{
        *DesaPtr++ = '-' ;
        cat_Aind(reg) ;
}

static void cat_Ad16(int reg, MPTR *pc)
{
        cat_hexa_word(read_st_word(*pc))  ;
        *pc+=2 ;
        cat_Aind(reg) ;
}

static void cat_Absw(MPTR *pc)
{
        cat_hexa_word(read_st_word(*pc)) ;
        *pc+=2 ;
        *DesaPtr++='.' ;
        *DesaPtr++='W' ;
}

static void cat_AbsL(MPTR *pc)
{
        cat_hexa_long(read_st_long(*pc)) ;
        *pc+=4 ;
}

static void cat_Pc16(MPTR *pc)
{
        cat_hexa_long(*pc + (signed int)(signed short)read_st_word(*pc)) ;
        *pc+=2 ;
        *DesaPtr++='(' ;
        *DesaPtr++='P' ;
        *DesaPtr++='C' ;
        *DesaPtr++=')' ;
}


static void cat_Ad8r(int reg, MPTR *pc)
{
        int code ;
        code = read_st_byte(*pc) ;
        cat_hexa_byte(read_st_byte(1+*pc)) ;
        *pc+=2 ;
        *DesaPtr++ = '(' ;
        *DesaPtr++ = 'A' ;
        *DesaPtr++ = reg+0x30 ;
        *DesaPtr++ = ',' ;
        if (code&0x80) *DesaPtr++ = 'A' ;
                else *DesaPtr++ = 'D' ;
        *DesaPtr++ = ((code>>4)&7)+0x30 ;
        *DesaPtr++ = '.' ;
        if (code&8) *DesaPtr++ = 'L' ;
                else *DesaPtr++ = 'W' ;
        *DesaPtr++ = ')' ;
}

static void cat_Pc8r(MPTR *pc)
{
        int code ;
        code = read_st_byte(*pc) ;
        cat_hexa_byte(read_st_byte(1+*pc)) ;
        *pc+=2 ;
        *DesaPtr++ = '(' ;
        *DesaPtr++ = 'P' ;
        *DesaPtr++ = 'C' ;
        *DesaPtr++ = ',' ;
        if (code&0x80) *DesaPtr++ = 'A' ;
                else *DesaPtr++ = 'D' ;
        *DesaPtr++ = ((code>>4)&7)+0x30 ;
        *DesaPtr++ = '.' ;
        if (code&8) *DesaPtr++ = 'L' ;
                else *DesaPtr++ = 'W' ;
        *DesaPtr++ = ')' ;
}

static void cat_Imme(int siz, MPTR *pc)
{
        *DesaPtr++ = '#' ;
        if (siz) {
          cat_hexa_long(read_st_long(*pc)) ;
          *pc += 4 ;
        }
        else {
          cat_hexa_word(read_st_word(*pc)) ;
          *pc += 2 ;
        }
}

        int do_decal(int d, int s)
        {
          if (s)
            return d>>1 ;
           else
            return d<<1 ;
        } ;

static void cat_regmask(int regmaski, int ispredecrem) // mask inverted if -(Ax)
{
        int i ;
        int testmask ;
        int testbit ;
        int decal ;
        int msk = regmaski ;

        if (ispredecrem) {
                testmask = 0xff00 ;
                testbit = 0x8000 ;
                decal = 0 ;
        } else {
                testmask = 0x00ff ;
                testbit = 0x0001 ;
                decal = 1 ;
        }

                if (msk&testmask) {
                        *DesaPtr++ = 'D' ;
                        for (i='0';i<'8';i++,msk = do_decal(msk,decal))
                                if (msk&testbit) *DesaPtr++ = i ;
                if (msk&testmask) *DesaPtr++ = '/' ;
                }
                        if (msk&testmask) {
                        *DesaPtr++ = 'A' ;
                        for (i='0';i<'8';i++,msk = do_decal(msk,decal))
                                if (msk&testbit) *DesaPtr++ = i ;
        }
}

static void cat_ea(int ea, MPTR *pc)
{
        int mode, reg, siz ;
        mode = (ea>>3)&7 ;
        reg = ea&7 ;
        siz = (ea>>8)&1 ;

        switch(mode) {
                case 0: cat_Dreg(reg) ; break ;
                case 1: cat_Areg(reg) ; break ;
                case 2: cat_Aind(reg) ; break ;
                case 3: cat_Aipi(reg) ; break ;
                case 4: cat_Aipd(reg) ; break ;
                case 5: cat_Ad16(reg, pc) ; break ;
                case 6: cat_Ad8r(reg, pc) ; break ;
                case 7: switch(reg) {
                        case 0: cat_Absw(pc) ; break ;
                        case 1: cat_AbsL(pc) ; break ;
                        case 2: cat_Pc16(pc) ; break ;
                        case 3: cat_Pc8r(pc) ; break ;
                        case 4: cat_Imme(siz,pc) ; break ;
                }
        } ;
}

/////////////////////////////////////////////////////////////////////////////

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "NONE"                          �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様� */
static void disa_Type_None(UWORD Instr, MPTR *pc)
{
}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "ABCD"                          �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC BA9 8 7654   3 210   R/M = 0 -> data register     Dy,Dx
敖陳賃陳賃賃陳陳堕陳堕陳�  R/M = 1 -> adresse register -(Ay),-(Ax)
�####�Rx �#�####�R/M�Ry �  Rx : destination register
青陳珍陳珍珍陳陳祖陳祖陳�  Ry : source register
*/

static void disa_Type_ABCD(UWORD Instr, MPTR *pc)
{
        *DesaPtr++='\t' ;
        if ((Instr&8)==8) {     // type -(Ay),-(Ax)
                cat_Aipd(Instr&7) ;
                *DesaPtr++ = ',' ;
                cat_Aipd((Instr>>9)&7) ;
        } else {
                cat_Dreg(Instr&7) ;
                *DesaPtr++ = ',' ;
                cat_Dreg((Instr>>9)&7) ;
        }
}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "ADD"                           �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC  B  A  9   8 7 6   543   2 1 0                     .b     .w      .l
敖陳賃陳陳陳陳賃陳陳陳賃陳陳堕陳陳陳朕
�####�Registre �OP-Mode�Mode�Register� Op-Modes:        000     001     010     Dn + EA -> Dn
青陳珍陳陳陳陳珍陳陳陳珍陳陳祖陳陳陳潰                  100     101     110     EA + Dn -> EA

*/

static void disa_Type_ADD(UWORD Instr, MPTR *pc)
{
        //int fakesize = Instr;
        int datasrc = (Instr>>8)&1 ;
        cat_size((Instr>>6)&3) ;
        //if (!(Instr&0x40)) fakesize=0x100 ;
        //Instr = (Instr&0xfeff)|fakesize ;

        if (Instr&0x40) Instr|=0x100 ;
        else Instr&=~0x100 ;


        *DesaPtr++='\t' ;
        if (datasrc)           // Bit 3 OpMode � 1 => Dn source
                {
                        cat_Dreg((Instr>>9)&7) ;
                        *DesaPtr++ = ',' ;
                        cat_ea(Instr,pc) ;
                }
                        else
                {
                        cat_ea(Instr,pc) ;
                        *DesaPtr++ = ',' ;
                        cat_Dreg((Instr>>9)&7) ;
                }

}


/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "ADDA"                          �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC  B  A  9   8 7 6   543   2 1 0
敖陳賃陳陳陳陳賃陳陳陳賃陳陳堕陳陳陳朕
�####�Registre �OP-Mode�Mode�Register�   ADDA <AE>, An
青陳珍陳陳陳陳珍陳陳陳珍陳陳祖陳陳陳潰
                 s 1 1  \___ E.A. __/

*/

static void disa_Type_ADDA(UWORD Instr, MPTR *pc)
{
        cat_size_wl((Instr>>8)&1) ;
        *DesaPtr++='\t' ;
        cat_ea(Instr,pc) ;
        *DesaPtr++ = ',' ;
        cat_Areg((Instr>>9)&7) ;
}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "ADDI"                          �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDCBA98  76   543   2 1 0
敖陳陳陳賃陳陳堕陳賃陳陳陳陳�
�########�Size�Mode�Register�   ADDI #nnnn, EA
青陳陳陳珍陳陳祖陳珍陳陳陳陳�

*/

static void disa_Type_ADDI(Instr,pc)
UWORD Instr ;
MPTR *pc ;
{
        int siz = ((Instr>>6)&3) ;
        cat_size(siz) ;
        *DesaPtr++='\t' ;
        *DesaPtr++ = '#' ;
        switch(siz) {
                case 0 : cat_hexa_byte(read_st_byte(1+*pc)) ; *pc += 2 ; break ;
                case 1 : cat_hexa_word(read_st_word(*pc)) ; *pc += 2 ; break ;
                case 2 : cat_hexa_long(read_st_long(*pc)) ; *pc += 4 ; break ;
                default:
                        *DesaPtr++ = '?' ;
        }
        *DesaPtr++ = ',' ;
        cat_ea(Instr,pc) ;
}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "ADDQ"                          �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC  BA9  8  76  543   2 1 0
敖陳賃陳陳賃賃陳陳堕陳賃陳陳陳陳�
�####�Value�#�Size�Mode�Register�
青陳珍陳陳珍珍陳陳祖陳珍陳陳陳陳�

*/

static void disa_Type_ADDQ(UWORD Instr, MPTR *pc)
{
        int value = (Instr>>9)&7 ;
        if (value==0) value = 8 ;
        cat_size((Instr>>6)&3) ;
        *DesaPtr++='\t';
        *DesaPtr++='#' ;
        cat_hexa_byte(value) ;
        *DesaPtr++ = ',' ;
        cat_ea(Instr,pc) ;
}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "ADDX"                          �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC BA9 8 7654   3 210   R/M = 0 -> data register     Dy,Dx
敖陳賃陳賃賃陳陳堕陳堕陳�  R/M = 1 -> adresse register -(Ay),-(Ax)
�####�Rx �#�####�R/M�Ry �  Rx : destination register
青陳珍陳珍珍陳陳祖陳祖陳�  Ry : source register

                .b      .w      .l
        Sizes:  00      01      10

*/
static void disa_Type_ADDX(UWORD Instr, MPTR *pc)
{
        cat_size((Instr>>6)&3) ;
        disa_Type_ABCD(Instr,pc) ;
}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "AND"                           �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC  B  A  9   8 7 6   543   2 1 0                     .b     .w      .l
敖陳賃陳陳陳陳賃陳陳陳賃陳陳堕陳陳陳朕  Op-Modes:       000     001     010     Dn & EA -> Dn
�####�Registre �OP-Mode�Mode�Register�                  100     101     110     EA & Dn -> EA
青陳珍陳陳陳陳珍陳陳陳珍陳陳祖陳陳陳潰
        Dn              \___ E.A. __/

*/

static void disa_Type_AND(UWORD Instr, MPTR *pc)
{
        disa_Type_ADD(Instr,pc) ;       // Identique sauf pour AND Ax,..
}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "i2CCR"                         �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�
        (ANDI to CCR)
*/
static void disa_Type_i2CCR(UWORD Instr, MPTR *pc)
{
        *DesaPtr++='\t' ;
        *DesaPtr++ = '#' ;
        cat_hexa_byte(read_st_byte(*pc+1)) ;
        *DesaPtr++ = ',' ;
        *DesaPtr++ = 'C' ;
        *DesaPtr++ = 'C' ;
        *DesaPtr++ = 'R' ;
        *pc += 2 ;
}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "i2SR"                          �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�
        (ANDI to SR, EORI to SR)
*/

static void disa_Type_i2SR(UWORD Instr, MPTR *pc)
{
        *DesaPtr++='\t' ;
        *DesaPtr++ = '#' ;
        cat_hexa_word(read_st_word(*pc)) ;
        *DesaPtr++ = ',' ;
        *DesaPtr++ = 'S' ;
        *DesaPtr++ = 'R' ;
        *pc += 2 ;

}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "ASL_Dx"                        �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC  BA9    8   76  5   43   2 1 0            dir: (0=right, 1=left)
敖陳賃陳陳陳堕陳堕陳賃陳賃陳堕陳陳陳朕          .b      .w      .l
�####�Nb/Reg�dir�Size�i/r�##�Register�  Size:   00      01      10
青陳珍陳陳陳祖陳祖陳珍陳珍陳祖陳陳陳潰  i/r:    0 = #nb shifts
                                                                                                                                                                        1 = Dx  shifts
*/

static void disa_Type_ASL_Dx(UWORD Instr, MPTR *pc)
{
        int reg = ((Instr>>9)&7) ;
        cat_size((Instr>>6)&3) ;
        *DesaPtr++='\t' ;
        if (Instr&0x20) {               // Dx
                cat_Dreg(reg) ;
        } else {                        // #
                *DesaPtr++ = '#' ;
                cat_hexa_byte(reg) ;
        }
        *DesaPtr++ = ',' ;
        cat_Dreg(Instr&7) ;
}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "ASL_EA"                        �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�
 FEDCBA9  8  76  543   2 1 0
敖陳陳陳堕陳堕賃陳陳堕陳陳陳朕
�#######�dir�##�Mode�Registre�
青陳陳陳祖陳祖珍陳陳祖陳陳陳潰
                \___ E.A. __/
*/

static void disa_Type_ASL_EA(UWORD Instr, MPTR *pc)
{
        cat_size(1)     ; // always on word
        *DesaPtr++='\t' ;
        cat_ea(Instr,pc) ;
}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "BRA"                           �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC     BA98     76543210
敖陳賃陳陳陳陳陳堕陳陳陳陳陳陳�
�####�Conditiong�8 bits offset�
団陳珍陳陳陳陳陳祖陳陳陳陳陳陳�
�        16 bits offset       �
青陳陳陳陳陳陳陳陳陳陳陳陳陳陳�

*/

static void disa_Type_BRA(UWORD Instr, MPTR *pc)
{
        MPTR pc2 = *pc;
        signed int offset ;

        if (Instr&0xff) {               //short branch
                *DesaPtr++ = '.' ;
                *DesaPtr++ = 'S' ;
                offset = (signed int)(signed short)(signed char)(Instr&0xff) ;
        } else {
                offset = (signed int)(signed short)read_st_word(*pc) ;
                *pc += 2 ;
        }
        *DesaPtr++='\t' ;
        cat_hexa_long(pc2+offset) ;
}


/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "BCHG_Dx"                       �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC BA9 876  543  2 1 0
敖陳賃陳賃陳賃陳陳堕陳陳陳朕
�####�Dx �###�Mode�Register�
青陳珍陳珍陳珍陳陳祖陳陳陳潰
              \___ E.A. __/
*/

static void disa_Type_BCHG_Dx(UWORD Instr, MPTR *pc)
{
        *DesaPtr++='\t' ;
        cat_Dreg((Instr>>9)&7) ;
        *DesaPtr ++ = ',' ;
        cat_ea(Instr,pc) ;
}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "BCHG_n"                        �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDCBA9876  543  2 1 0
敖陳陳陳陳賃陳陳堕陳陳陳朕
�##########�Mode�Register�>EA
団陳陳陳陳津陳陳祖陳陳陳調
�##########� bit number  �
青陳陳陳陳珍陳陳陳陳陳陳潰

*/

static void disa_Type_BCHG_n(UWORD Instr, MPTR *pc)
{
        *DesaPtr++='\t' ;
        *DesaPtr++ = '#' ;
        cat_hexa_byte(read_st_byte(1+*pc)) ;
        *DesaPtr++ = ',' ;
        *pc += 2 ;
        cat_ea(Instr,pc) ;
}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "CHK"                           �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC   B A 9  876  543  2 1 0
敖陳賃陳陳陳陳堕陳堕陳賃陳陳陳陳�
�####�Register�###�Mode�Register�
青陳珍陳陳陳陳祖陳祖陳珍陳陳陳陳�
                   \___ E.A. __/
*/

static void disa_Type_CHK(UWORD Instr, MPTR *pc)
{
        int fakesize = (Instr&0x3f);
        *DesaPtr++='\t' ;
        cat_ea(fakesize,pc) ;
        *DesaPtr++ = ',' ;
        cat_Dreg((Instr>>9)&7) ;
}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "CLR"                           �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�
 FEDCBA98  76  543  2 1 0
敖陳陳陳賃陳陳堕陳賃陳陳陳陳� b  00
�########�Size�Mode�Register� w  01
青陳陳陳珍陳陳祖陳珍陳陳陳陳� l  10
               \___ E.A. __/
*/

static void disa_Type_CLR(UWORD Instr, MPTR *pc)
{
        cat_size((Instr>>6)&3) ;
        *DesaPtr++='\t' ;
        cat_ea(Instr,pc) ;
}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "CMP"                           �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�
 FEDC  B  A  9   8 7 6   543   2 1 0
敖陳賃陳陳陳陳賃陳陳陳賃陳陳堕陳陳陳朕
�####�Registre �OP-Mode�Mode�Register� CMP <EA>, Dn
青陳珍陳陳陳陳珍陳陳陳珍陳陳祖陳陳陳潰
        Dn              \___ E.A. __/                   .b      .w      .l
                                       Op-Modes:        000     001     010

*/

static void disa_Type_CMP(UWORD Instr, MPTR *pc)
{
        cat_size((Instr>>6)&3) ;
        *DesaPtr++='\t' ;

        cat_ea(Instr,pc) ;
        *DesaPtr++ = ',' ;
        cat_Dreg((Instr>>9)&7) ;
}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "CMPM"                          �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC BA9 8  76  543 210        CMPM (Ay)+,(Ax)+
敖陳賃陳賃賃陳陳堕陳堕陳�
�####�Ax �#�Size�###�Ay �  Ax : destination register
青陳珍陳珍珍陳陳祖陳祖陳�  Ay : source register

*/

static void disa_Type_CMPM(UWORD Instr, MPTR *pc)
{
        cat_size((Instr>>6)&3) ;
        *DesaPtr++='\t' ;
        cat_Aipi(Instr&7) ;
        *DesaPtr++ = ',' ;
        cat_Aipi((Instr>>9)&7) ;
}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "DBF"                           �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC    BA98    76543    210
敖陳賃陳陳陳陳陳堕陳陳堕陳陳陳朕
�####�Conditiong�#####�Register�
団陳珍陳陳陳陳陳祖陳陳祖陳陳陳調
�        16 bits offset        �
青陳陳陳陳陳陳陳陳陳陳陳陳陳陳潰

*/

static void disa_Type_DBF(UWORD Instr, MPTR *pc)
{
        signed int offset = (signed int)(signed short)read_st_word(*pc) ;
        *DesaPtr++= '\t' ;
        cat_Dreg(Instr&7) ;
        *DesaPtr++ = ',' ;
        cat_hexa_long(offset+*pc) ;
        *pc += 2 ;
}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "EOR"                           �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC  B  A  9   8 7 6   543   2 1 0
敖陳賃陳陳陳陳賃陳陳陳賃陳陳堕陳陳陳朕                   .b     .w      .l
�####�Registre �OP-Mode�Mode�Register� Op-Modes:        100     101     110     EA & Dn -> EA
青陳珍陳陳陳陳珍陳陳陳珍陳陳祖陳陳陳潰
        Dn              \___ E.A. __/

*/

static void disa_Type_EOR(UWORD Instr, MPTR *pc)
{
        disa_Type_AND(Instr,pc) ;       // identique mais TOUJOURS <AE> destination
}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "EXG"                           �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC BA9 8  76543  210
敖陳賃陳賃賃陳陳陳賃陳朕
�####�Rx �#�OP-Mode�Ry �  Rx : data register
青陳珍陳珍珍陳陳陳珍陳潰  Ry : address / data register

 OP-Modes:      01000 -> data registers
                01001 -> address registers
                10001 -> data & address registers
*/

static void disa_Type_EXG(UWORD Instr, MPTR *pc)
{
        *DesaPtr++='\t' ;
        switch((Instr>>3)&0x1f) {       // type des registres
                case 0x08 :     cat_Dreg((Instr>>9)&7) ;
                                *DesaPtr++ = ',' ;
                                cat_Dreg(Instr&7) ;
                                break ;
                case 0x09 :     cat_Areg((Instr>>9)&7) ;
                                *DesaPtr++ = ',' ;
                                cat_Areg(Instr&7) ;
                                break ;
                case 0x11 :     cat_Dreg((Instr>>9)&7) ;
                                *DesaPtr++ = ',' ;
                                cat_Areg(Instr&7) ;
                                break ;
                default:
                                *DesaPtr++ = '?' ;
        }
}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "EXT"                           �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDCBA9   876   543   210
敖陳陳陳堕陳陳陳堕陳堕陳陳陳朕  OP-Modes:       010 byte to word
�#######�OP-Mode�###�Register�            011 word to longword
青陳陳陳祖陳陳陳祖陳祖陳陳陳潰

*/

static void disa_Type_EXT(UWORD Instr, MPTR *pc)
{
        *DesaPtr++ = '.' ;
        if (Instr&0x40) *DesaPtr++='L' ; else *DesaPtr++='W' ;
        *DesaPtr++='\t' ;
        cat_ea(Instr&7,pc) ;
}


/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "JMP"                           �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC BA98 76
敖陳陳陳陳陳賃陳陳堕陳陳陳朕
�#### #### ##�Mode�Register�
青陳陳陳陳陳珍陳陳祖陳陳陳潰
              \___ E.A. __/
*/

static void disa_Type_JMP(UWORD Instr, MPTR *pc)
{
        *DesaPtr++='\t' ;
        cat_ea(Instr,pc) ;
}


/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "LEA"                           �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC  B  A  9  876 543   2 1 0
敖陳賃陳陳陳陳賃陳賃陳陳堕陳陳陳朕
�####�Register �###�Mode�Register�
青陳珍陳陳陳陳珍陳珍陳陳祖陳陳陳潰
        An          \___ E.A. __/
*/

static void disa_Type_LEA(UWORD Instr, MPTR *pc)
{
        *DesaPtr++='\t' ;
        cat_ea(Instr,pc) ;
        *DesaPtr++ = ',' ;
        cat_Areg((Instr>>9)&7) ;
}


/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "LINK"                          �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC BA98 7654 3    210
敖陳陳陳陳陳陳陳賃陳陳陳陳�
�#### #### #### #�Register�
団陳陳陳陳陳陳陳珍陳陳陳陳�
�          Offset         �
青陳陳陳陳陳陳陳陳陳陳陳陳�

*/

static void disa_Type_LINK(UWORD Instr, MPTR *pc)
{
        *DesaPtr++='\t' ;
        cat_Areg(Instr&7) ;
        *DesaPtr++ = ',' ;
        *DesaPtr++ = '#' ;
        cat_hexa_word(read_st_word(*pc)) ;
        *pc += 2 ;
}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "MOVE"                          �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FE   DC  B  A  9   8 7 6   543   2 1 0
敖陳賃陳陳堕陳陳陳賃陳陳堕陳賃陳陳陳陳�
�####�Size�Register�Mode�Mode�Register� Size:  01=b, 11=w, 10=l
青陳珍陳陳祖陳陳陳珍陳陳祖陳珍陳陳陳陳�
           \destination/|   source   /
            \__ E.A.__/  \__ E.A. __/
*/

static void disa_Type_MOVE(UWORD Instr, MPTR *pc)
{
        int mode, reg ;
        int fakeinstr ;

        fakeinstr = Instr ; // on triche pour la taille en bit 8


        *DesaPtr++ = '.' ;
        switch((Instr>>12)&3) {
                case 1: *DesaPtr++ = 'B' ; break ;
                case 2: *DesaPtr++ = 'L' ; fakeinstr|=0x100 ;break ;
                case 3: *DesaPtr++ = 'W' ;
        }
        *DesaPtr++='\t' ;

        if ((Instr&0x3f)==0x3c)         // immediate
        {
                *DesaPtr++ = '#' ;
                switch ((Instr>>12)&3)
                {
                        case 1 : cat_hexa_byte(read_st_byte(1+*pc)) ; *pc += 2 ; break ;
                        case 2 : cat_hexa_long(read_st_long(*pc)) ; *pc += 4 ; break;
                        case 3 : cat_hexa_word(read_st_word(*pc)) ; *pc += 2 ; break ;
                }

        } else         cat_ea(fakeinstr,pc) ;


        *DesaPtr++ = ',' ;
        mode = (Instr>>6)&7 ;
        reg = (Instr>>9)&7 ;
        cat_ea((mode<<3)+reg,pc) ;
}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "2CCR"                          �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC BA98 76
敖陳陳陳陳陳賃陳陳堕陳陳陳朕
�#### #### ##�Mode�Register�
青陳陳陳陳陳珍陳陳祖陳陳陳潰
              \___ E.A. __/

*/

static void disa_Type_2CCR(UWORD Instr, MPTR *pc)
{
        *DesaPtr++='\t' ;
        cat_ea(Instr,pc) ;
        *DesaPtr++ = ',' ;
        *DesaPtr++ = 'C' ;
        *DesaPtr++ = 'C' ;
        *DesaPtr++ = 'R' ;
}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "MOVEA"                         �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FE    DC    BA9    876  543   2 1 0
敖陳賃陳陳堕陳陳陳賃陳賃陳陳堕陳陳陳朕
�####�Size�Registre�###�Mode�Register�   MOVEA <AE>, An
青陳珍陳陳祖陳陳陳珍陳珍陳陳祖陳陳陳潰
                        \___ E.A. __/
*/

static void disa_Type_MOVEA(UWORD Instr, MPTR *pc)
{
        int fakesize = (Instr&0x3f);   // fake instruction to work with <ea>

        *DesaPtr++ = '.' ;
        if ((Instr&0xf000)==0x3000)
                *DesaPtr++ = 'W' ;
                else { *DesaPtr++ = 'L' ; fakesize |=0x100 ; }
        *DesaPtr++='\t' ;
        cat_ea(fakesize,pc) ;
        *DesaPtr++ = ',' ;
        cat_Areg((Instr>>9)&7) ;
}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "2USP"                          �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC BA98 7654 3  2 1 0
敖陳陳陳陳陳陳陳賃陳陳陳陳�
�#### #### #### #�Register�
青陳陳陳陳陳陳陳珍陳陳陳陳�
                                                                                An
*/

static void disa_Type_2USP(UWORD Instr, MPTR *pc)
{
        *DesaPtr++='\t' ;
        cat_Areg(Instr&7) ;
        *DesaPtr++ = ',' ;
        *DesaPtr++ = 'U' ;
        *DesaPtr++ = 'S' ;
        *DesaPtr++ = 'P' ;
}


/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "USP2"                          �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC BA98 7654 3  2 1 0
敖陳陳陳陳陳陳陳賃陳陳陳陳�
�#### #### #### #�Register�
青陳陳陳陳陳陳陳珍陳陳陳陳�
                                                                                An
*/

static void disa_Type_USP2(UWORD Instr, MPTR *pc)
{
        *DesaPtr++='\t' ;
        *DesaPtr++ = 'U' ;
        *DesaPtr++ = 'S' ;
        *DesaPtr++ = 'P' ;
        *DesaPtr++ = ',' ;
        cat_Areg(Instr&7) ;
}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "SR2"                           �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC BA98 76 543    2 1 0
敖陳陳陳陳陳賃陳陳堕陳陳陳朕
�#### #### ##�Mode�Register�
青陳陳陳陳陳珍陳陳祖陳陳陳潰
              \___ E.A. __/
*/

static void disa_Type_SR2(UWORD Instr, MPTR *pc)
{
        *DesaPtr++='\t' ;
        *DesaPtr++ = 'S' ;
        *DesaPtr++ = 'R' ;
        *DesaPtr++ = ',' ;
        cat_ea(Instr,pc) ;
}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "2SR"                           �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC BA98 76 543    2 1 0
敖陳陳陳陳陳賃陳陳堕陳陳陳朕
�#### #### ##�Mode�Register�
青陳陳陳陳陳珍陳陳祖陳陳陳潰
              \___ E.A. __/
*/

static void disa_Type_2SR(UWORD Instr, MPTR *pc)
{
        *DesaPtr++='\t' ;
        cat_ea(Instr,pc) ;
        *DesaPtr++ = ',' ;
        *DesaPtr++ = 'S' ;
        *DesaPtr++ = 'R' ;
}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "MOVEM2mem"                     �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC BA98 7  6    543   2 1 0
敖陳陳陳陳陳堕陳賃陳陳堕陳陳陳朕
�#### #### #�Size�Mode�Register�  Size: 0=word 1=longword
団陳陳陳陳陳祖陳珍陳陳祖陳陳陳調
�       Registers mask         �
青陳陳陳陳陳陳陳陳陳陳陳陳陳陳潰
                  \___ E.A.___/
*/

static void disa_Type_MOVEM2mem(UWORD Instr, MPTR *pc)
{
        int regmask = read_st_word(*pc) ;
        *pc += 2 ;
        *DesaPtr++ = '.' ;
        if (Instr&0x40) //taille
                *DesaPtr++ = 'L' ;
         else
                *DesaPtr++ = 'W' ;
        *DesaPtr++='\t' ;
        cat_regmask(regmask,((Instr&0x38)==0x20)) ; //is it -(Ax) adressing mode?
        *DesaPtr++ = ',' ;
        cat_ea(Instr,pc) ;
}


/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "MOVEMmem2"                     �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC BA98 7  6    543   2 1 0
敖陳陳陳陳陳堕陳賃陳陳堕陳陳陳朕
�#### #### #�Size�Mode�Register� Size: 0=word 1=longword
団陳陳陳陳陳祖陳珍陳陳祖陳陳陳調
�       Registers mask         �
青陳陳陳陳陳陳陳陳陳陳陳陳陳陳潰
                  \___ E.A.___/
*/

static void disa_Type_MOVEMmem2(UWORD Instr, MPTR *pc)
{
        int regmask = read_st_word(*pc) ;
        *pc += 2 ;
        *DesaPtr++ = '.' ;
        if (Instr&0x40) //taille
                *DesaPtr++ = 'L' ;
         else
                *DesaPtr++ = 'W' ;
        *DesaPtr++='\t' ;
        cat_ea(Instr,pc) ;
        *DesaPtr++ = ',' ;
        cat_regmask(regmask,0) ;

}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "MOVEP_2Dx"                     �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

FEDC BA9 87   6  543 210
敖陳賃陳賃陳堕陳賃陳賃陳朕
�####�Dx �10�Size�###�Ay �
団陳珍陳珍陳祖陳珍陳珍陳調
�      16 bits offset    �
青陳陳陳陳陳陳陳陳陳陳陳潰

*/

static void disa_Type_MOVEP_2Dx(UWORD Instr, MPTR *pc)
{
        *DesaPtr++ = '.' ;
        if ((Instr&0x40))
                *DesaPtr++ = 'L' ;
          else *DesaPtr++ = 'W' ;
        *DesaPtr++='\t' ;
        cat_Ad16(Instr&7,pc) ;
        *DesaPtr++ = ',' ;
        cat_Dreg((Instr>>9)&7) ;
}


/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "MOVEP_Dx2"                     �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

敖陳賃陳賃陳堕陳賃陳賃陳朕
�####�Dx �11�Size�###�Ay �
団陳珍陳珍陳祖陳珍陳珍陳調
�      16 bits offset    �
青陳陳陳陳陳陳陳陳陳陳陳潰

*/
static void disa_Type_MOVEP_Dx2(UWORD Instr, MPTR *pc)
{
        *DesaPtr++ = '.' ;
        if ((Instr&0x40))
                *DesaPtr++ = 'L' ;
          else *DesaPtr++ = 'W' ;
        *DesaPtr++='\t' ;
        cat_Dreg((Instr>>9)&7) ;
        *DesaPtr++ = ',' ;
        cat_Ad16(Instr&7,pc) ;
}


/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "MOVEQ"                         �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC    BA9   8 76543210
敖陳賃陳陳陳陳堕堕陳陳陳朕
�####�Register�#�  Data  �
青陳珍陳陳陳陳祖祖陳陳陳潰

*/

static void disa_Type_MOVEQ(UWORD Instr, MPTR *pc)
{
        *DesaPtr++='\t' ;
        *DesaPtr++ = '#' ;
        cat_hexa_byte(Instr&0xff) ;
        *DesaPtr++ = ',' ;
        cat_Dreg((Instr>>9)&7) ;
}


/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "NBCD"                          �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC BA98 76
敖陳陳陳陳陳賃陳陳堕陳陳陳朕
�#### #### ##�Mode�Register�
青陳陳陳陳陳珍陳陳祖陳陳陳潰
              \___ E.A. __/

*/

static void disa_Type_NBCD(UWORD Instr, MPTR *pc)
{
        disa_Type_JMP(Instr,pc) ;// identique sauf pour certains modes d'adr.
}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "STOP"                          �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC  BA98  7654  3210
敖陳陳陳陳陳陳陳陳陳陳陳朕
� ####  ####  ####  #### �
団陳陳陳陳陳陳陳陳陳陳陳調
�      16 bits value     �
青陳陳陳陳陳陳陳陳陳陳陳潰
*/

static void disa_Type_STOP(UWORD Instr, MPTR *pc)
{
        *DesaPtr++='\t' ;
        *DesaPtr++ = '#' ;
//        *DesaPtr++ = '$' ;
        cat_hexa_word(read_st_word(*pc)) ;
        *pc += 2 ;
}

/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "SWAP"                          �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC BA98 7654 3   210
敖陳陳陳陳陳陳陳賃陳陳陳陳�
�#### #### #### #�Register�
青陳陳陳陳陳陳陳珍陳陳陳陳�
*/

static void disa_Type_SWAP(UWORD Instr, MPTR *pc)
{
        *DesaPtr++='\t' ;
        cat_ea(Instr&7,pc) ;
}


/*                              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                                �   D�sassemblage  � Type "TRAP"                          �
                                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

FEDC BA98 7654  3210
敖陳陳陳陳陳陳賃陳陳陳�
�#### #### ####�Vector�
青陳陳陳陳陳陳珍陳陳陳�

*/

static void disa_Type_TRAP(UWORD Instr, MPTR *pc)
{
        *DesaPtr++='\t' ;
        *DesaPtr++ = '#' ;
        *DesaPtr++ = '$' ;
        cat_hexa_nibble(Instr&15) ;

}


/*              浜様様様様様様様様用様様様様様様様様様様様様様様様様様様様�
                �   D�sassemblage  � Type "UNLK"                          �
                藩様様様様様様様様溶様様様様様様様様様様様様様様様様様様様�

 FEDC BA98 7654 3   210
敖陳陳陳陳陳陳陳賃陳陳陳陳�
�#### #### #### #�Register�
青陳陳陳陳陳陳陳珍陳陳陳陳�

*/

static void disa_Type_UNLK(UWORD Instr, MPTR *pc)
{
        *DesaPtr++='\t' ;
        cat_Areg(Instr&7) ;
}

static void disa_Type_DATA(UWORD Instr, MPTR *pc)
{
        *DesaPtr++='\t' ;
        cat_hexa_word(Instr) ;
}

static void disa_Type_PATCH(UWORD Instr, MPTR *pc)
{
        *DesaPtr++='\t' ;
        cat_hexa_word(read_st_word(*pc)) ;
        *pc+=2 ;
}

static void cat_Rc(int RC)
{
        switch(RC) {
                case 0x000 : strcpy(DesaPtr,"SFC ") ;
                             break ;
                case 0x001 : strcpy(DesaPtr,"DFC ") ;
                             break ;
                case 0x002 : strcpy(DesaPtr,"CACR") ;
                             break ;
                case 0x800 : strcpy(DesaPtr,"USP ") ;
                             break ;
                case 0x801 : strcpy(DesaPtr,"VBR ") ;
                             break ;
                case 0x802 : strcpy(DesaPtr,"CAAR") ;
                             break ;
                case 0x803 : strcpy(DesaPtr,"MSP ") ;
                             break ;
                case 0x804 : strcpy(DesaPtr,"ISP ") ;
                             break ;
                default :    strcpy(DesaPtr,"??? ") ;
        }
        DesaPtr+=4 ;
}

static void disa_Type_MOVEC(UWORD Instr, MPTR *pc)
{
        int param ;
        char r ;

        *DesaPtr++='\t' ;
        param = read_st_word(*pc) ;
        *pc+= 2 ;

        if (param&0x8000) r = 'A' ; else r = 'D' ;


        if (Instr&1) {
                        // Rn to Rc
                *DesaPtr++ = r ;
                *DesaPtr++ = ((param>>12)&7)+'0' ;
                *DesaPtr++ = ',' ;
                cat_Rc(param&0xfff) ;
        } else {
                cat_Rc(param&0xfff) ;
                *DesaPtr++ = ',' ;
                *DesaPtr++ = r ;
                *DesaPtr++ = ((param>>12)&7)+'0' ;
        }

}

static void disa_Type_FPU(UWORD Instr, MPTR *pc)
{
        *pc+=2 ;
        *DesaPtr++ = '\t' ;
        cat_ea(Instr&0x3f,pc) ;
}

/////////////////////////////////////////////////////////////////////////////

typedef char *foncd(UWORD, MPTR *) ;
static foncd *Foncs_Desa[] =
 {
        disa_Type_None, disa_Type_ABCD, disa_Type_ADD,
        disa_Type_ADDA, disa_Type_ADDI, disa_Type_ADDQ,
        disa_Type_ADDX, disa_Type_AND, disa_Type_i2CCR,
        disa_Type_i2SR, disa_Type_ASL_Dx, disa_Type_ASL_EA,
        disa_Type_BRA, disa_Type_BCHG_Dx, disa_Type_BCHG_n,
        disa_Type_CHK, disa_Type_CLR, disa_Type_CMP,
        disa_Type_CMPM, disa_Type_DBF,disa_Type_EOR,
        disa_Type_EXG, disa_Type_EXT, disa_Type_JMP,
        disa_Type_LEA, disa_Type_LINK, disa_Type_MOVE,
        disa_Type_2CCR, disa_Type_MOVEA, disa_Type_2USP,
        disa_Type_USP2, disa_Type_SR2, disa_Type_2SR,
        disa_Type_MOVEM2mem, disa_Type_MOVEMmem2, disa_Type_MOVEP_2Dx,
        disa_Type_MOVEP_Dx2, disa_Type_MOVEQ, disa_Type_NBCD,
        disa_Type_STOP, disa_Type_SWAP, disa_Type_TRAP,
        disa_Type_UNLK, disa_Type_DATA, disa_Type_PATCH,
        disa_Type_MOVEC, disa_Type_FPU
 } ;

char DesaString[80] ;


/*敖陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳卍陳陳陳陳陳陳陳�
 *�Void disa_instr(MPTR adr, char *disa_string, int *siz) �               �
 *突様様様様様様様様様様様様様様様様様様様様様様様様様様様�               �
 *�                                                                       �
 *�      adr: Adress of the instruction to disasm                         �
 *�                                                                       �
 *�      Disa_String: ^Disassembly string                                 �
 *�                                                                       �
 *�      size: taille de l'instruction                                    �
 *�                                                                       �
 *青陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳�
 */

void disa_instr(MPTR adr, char *disa_string, int *siz)
 {
  struct Entry_Instr_68000 *p ;
  UWORD opcode ;
  UWORD andmask ;
  MPTR adr0 ;
  int i ;

        adr0 = adr ;
        opcode = read_st_word(adr) ;
        p = Table_Instr_68000 ;

        while ((andmask=p->AndMask)&&((andmask&opcode)!=p->CmpMask)) p++ ;

        adr+=2 ;
        DesaPtr = DesaString ;
        for (i=0;i<6;i++)
                if (p->InstrName[i]!=' ')
                *DesaPtr++=p->InstrName[i] ;
                (*Foncs_Desa[p->Coding])(opcode,&adr) ;
                *DesaPtr++ = 0 ;

        *siz = adr - adr0 ;
        strcpy(disa_string,DesaString) ;
 }

