/* KoalaClient.c */
/* cc65 koala viewer to load and display */

#include <cbm.h>
#include <peekpoke.h>
#include <conio.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#ifdef LOCAL
#define BASE_URL "http://192.168.7.114/test/"
#else
#define BASE_URL "http://vortex.jammingsignal.com:8064/ml/koala/"
#endif

#define COUNT_ADDR      0xC000
#define COLOR_TEMP_ADDR 0xC010

enum Mode 
{
    RANDOM,
    LATEST,
    INDEX
};


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

int LoadKoalaPictureAndDisplay(char* koala_filename) 
{
    unsigned char dev;
    unsigned char addr[2];

    dev = PEEK(0x00BA); /* get current device number */

    /* open the file */
    if (cbm_open(1, dev, 2, (const char *)koala_filename)) {
        clrscr();
        cprintf("Couldn't open %s.\n", koala_filename);
        return(1);
    }

    /* read file load address */
    if (cbm_read(1, &addr, 2) != 2) {
        cbm_close(1);
        clrscr();
        cprintf("Couldn't read load address.\n");
        return(1);
    }

    // make sure load address is $4400 or $6000 - also allow $2000 and $0000 for images from Tom's Editor Gallery
    if (addr[0] != 0 || (addr[1] != 0x44 && addr[1] != 0x60 && addr[1] != 0x20 && addr[1] != 0x00)) {
        cbm_close(1);
        clrscr();
        cprintf("This doesn't look like a koala picture.\n");
        return(2);
    }

    /* load bitmap data */
    if (loadtoram(1, (unsigned char*)0x2000, 8000)) {
        cbm_close(1);
        clrscr();
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
    int index = 0;    
    int blank_on_load = 0;
    char path[100] = "";
    char c = ' ';
    enum Mode mode = RANDOM;

    dev = PEEK(0x00ba); /* get current device number */

    while (1)
    {
        /* Get # of images available */
        cbm_load(BASE_URL"count.prg", dev, NULL);
        count = PEEKW(COUNT_ADDR);  // TODO, use long once more than 65536 images :-)

        text_screen();
        clrscr();
        cprintf("KoalaScope starting...\n\r\n\r");
        cprintf("%d Koala images on server.\n\r\n\r", count);
        cprintf("Keys during display:\n\r\n\r SPACE to advance to next picture\n\r +/-   to move forward/backward*\n\r SHIFT to pause\n\r F1    to return to this screen\n\r STOP  to exit\n\r\n\r");
        cprintf("Select Mode to Start:\r\n\r\n R=Random, L=Latest, I=Index*\n\r");
        c = cgetc();

        switch (c)
        {
            case 'l':
                mode = LATEST;
                strcpy(path, BASE_URL"latest.koa");
                break;

            case 'i': 
                mode = INDEX;
                strcpy(path, BASE_URL"index.koa");
                break;

            case 3:  // RUN-STOP
                goto cleanup;

            case ' ':
            case 'r':
            default:
                mode = RANDOM;
                strcpy(path, BASE_URL"random.koa");
                break;
        }

        loop = 1;
        while (loop)
        {
            if (mode == INDEX)
            {
                sprintf(path, BASE_URL"index.koa?index=%d", index);
            }

            // TODO, blank screen, or eventually use double buffering for smooth transition        
            koala_screen();
            result = LoadKoalaPictureAndDisplay(path);

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
                case 0:  // Timeout    
                    continue;

                case ' ':
                    if (mode == INDEX)
                    {
                        index++;
                        if (index >= count)
                        {
                            index = 0;  // Wrap
                        }
                    }
                    continue;

                case 3:  // RUN-STOP
                    goto cleanup;                   

                case 133: // F1
                    loop = 0;
                    break;

                case '+':
                    index++;
                    if (index >= count)
                    {
                        index = 0;  // Wrap
                    }
                    break;

                case '-':
                    index--;
                    if (index < 0)
                    {
                        index = count - 1;  // Wrap
                    }
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
