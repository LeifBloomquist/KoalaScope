/* cc65 koala viewer to load and display */

#include <cbm.h>
#include <peekpoke.h>
#include <conio.h>
#include <time.h>

//#define BASE_URL "http://192.168.7.114/test/"
#define BASE_URL "http://vortex.jammingsignal.com:8064/ml/koala/"

#define COLOR_TEMP_ADDR 0xC100

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
    if (cbm_open(1, dev, 2, (const char *)koala_filename)) {
        cprintf("Couldn't open %s.\n", koala_filename);
        return(1);
    }

    /* read file load address */
    if (cbm_read(1, &addr, 2) != 2) {
        cbm_close(1);
        cprintf("Couldn't read load address.\n");
        return(1);
    }

    // make sure load address is $4400 or $6000 - also allow $2000 and $0000 for images from Tom's Gallery
    if (addr[0] != 0 || (addr[1] != 0x44 && addr[1] != 0x60 && addr[1] != 0x20 && addr[1] != 0x00)) {
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

    /* load background+border colour into $C100 temporarily */
    if (loadtoram(1, (unsigned char*)COLOR_TEMP_ADDR, 1)) {
        cbm_close(1);
        clrscr();
        cprintf("Error while reading background colour.\n");
        return(1);
    }

    /* load background colour into $d021 */
    POKE(0xD021, PEEK(COLOR_TEMP_ADDR) & 0x0F);

    /* load border colour (upper nybble of last byte) into $d020 */
    POKE(0xD020, (PEEK(COLOR_TEMP_ADDR) & 0xF0) >> 4);

    /* done */
    cbm_close(1);
    return(0);
}

void koala_screen()
{
    POKE(0xd011, 0x3b); /* enable bitmap mode */
    POKE(0xd016, 0x18); /* enable multicolour */
    POKE(0xd018, 0x1f); /* screen at $0400 bitmap at $2000 */
}
    
void text_screen()
{
    POKE(0xd011, 0x1b);
    POKE(0xd016, 0x08);
    POKE(0xd018, 0x17);
    POKE(0xd020, 0x0e);
    POKE(0xd021, 0x06);
}

int sleep_or_key(unsigned wait)
{
    clock_t goal = clock() + ((clock_t)wait) * CLOCKS_PER_SEC;
    while ((long)(goal - clock()) > 0)
    {
        if (kbhit())
        {
            return cgetc();
        }
    }
    return 0;
}

void pause_on_shift()
{
    while (PEEK(0x028D))
    {
        ; // Pause
    }
}

void main() 
{
    int dev = 8;
    int count = 0;
    int result = 0;
    int timeout = 5;
    int loop = 1;
    int blank_on_load = 0;

    dev = PEEK(0x00ba); /* get current device number */

    while (1)
    {
        /* Get # of images available */
        cbm_load(BASE_URL"count.prg", dev, NULL);
        count = PEEKW(0xC000);

        text_screen();
        clrscr();
        cprintf("KoalaScope starting...\n\r\n\r");
        cprintf("%d Koala images on server.\n\r\n\r", count);
        cprintf("Keys during display:\n\r\n\r SPACE to advance to next picture\n\r SHIFT to pause\n\r F1    to return to this screen\n\r STOP  to exit\n\r\n\r");
        cprintf("Press any key to start.");
        cgetc();

        loop = 1;
        while (loop)
        {
            // TODO, blank screen, or eventually use double buffering for smooth transition        
            koala_screen();
            result = LoadKoalaPictureAndDisplay((unsigned char*)BASE_URL"random.koa");

            if (result != 0)
            {
                text_screen();
                bordercolor(2);
                sleep_or_key(timeout);
                continue;
            }

            result = sleep_or_key(timeout);

            switch (result)
            {
                case ' ':
                    continue;

                case 3:  // RUN-STOP
                    goto cleanup;                   

                case 133: // F1
                    loop = 0;
                    break;

                default:                    
                    break;
            }

            pause_on_shift();
        }
    }

cleanup:
    // Clean up
    text_screen();
    clrscr();
    cprintf("KoalaScope Exited\n\r");
    POKE(198, 0); /* clear keyboard queue */
}
