/* cc65 koala viewer to load and display */

#include <cbm.h>
#include <peekpoke.h>
#include <conio.h>

unsigned char loadtoram(unsigned char lfn, unsigned char* dest, unsigned int length) {
    int l;

    while (length) {
        l = cbm_read(lfn, dest, length); /* try to read length bytes */
        if (l == -1) {
            return(1); /* something went wrong */
        }
        else {
            length -= l; /* decrease length by actually amount read */
            dest += l; /* increase destination pointer by same amount */
        }
    }
    return(0); /* ok */
}

int LoadKoalaPictureAndDisplay(unsigned char* koala_filename) 
{
    unsigned char dev;
    unsigned char addr[2];

    dev = PEEK(0x00ba); /* get current device number */

    /* open the file */
    if (cbm_open(1, dev, 2, koala_filename)) {
        cprintf("Couldn't open %s.\n", koala_filename);
        return(1);
    }

    /* read file load address */
    if (cbm_read(1, &addr, 2) != 2) {
        cbm_close(1);
        cprintf("Couldn't read load address.\n");
        return(1);
    }

    // make sure load address is $4400 or $6000
    if (addr[0] != 0 || (addr[1] != 0x44 && addr[1] != 0x60)) {
        cbm_close(1);
        cprintf("This doesn't look like a koala picture.\n");
        return(2);
    }

    /* load bitmap data */
    if (loadtoram(1, (unsigned char*)0x2000, 8000)) {
        cbm_close(1);
        cprintf("Error while reading bitmap.\n");
        return(1);
    }

    /* load screen data */
    if (loadtoram(1, (unsigned char*)0x0400, 1000)) {
        cbm_close(1);
        clrscr();
        cprintf("Error while reading screen ram.\n");
        return(1);
    }

    /* load colour ram */
    if (loadtoram(1, (unsigned char*)0xd800, 1000)) {
        cbm_close(1);
        clrscr();
        cprintf("Error while reading colour ram.\n");
        return(1);
    }

    /* load background colour into $d021 */
    if (loadtoram(1, (unsigned char*)0xd021, 1)) {
        cbm_close(1);
        clrscr();
        cprintf("Error while reading background colour.\n");
        return(1);
    }

    /* done */
    cbm_close(1);

    POKE(0xd011, 0x3b); /* enable bitmap mode */
    POKE(0xd016, 0x18); /* enable multicolour */
    POKE(0xd018, 0x1f); /* screen at $0400 bitmap at $2000 */
    POKE(0xd020, 0x00); /* black border */

    while (!kbhit()) { ; } /* wait for key */

    POKE(0xd011, 0x1b);
    POKE(0xd016, 0x08);
    POKE(0xd018, 0x17);
    POKE(0xd020, 0x0e);
    POKE(0xd021, 0x06);

    clrscr();
    POKE(198, 0); /* clear keyboard queue */
}

void main() 
{
    cprintf("KoalaScope Starting...\n\r");
    //LoadKoalaPictureAndDisplay("http://192.168.7.114/test/random.koa");
    LoadKoalaPictureAndDisplay("cyberwing.koa");
}