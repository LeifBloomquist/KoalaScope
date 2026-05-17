/* KoalaClient.c */
/* cc65 koala viewer to load and display */

#include <c64.h>
#include <peekpoke.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef LOCAL
#define BASE_URL "http://192.168.7.99/test/"
#else
#define BASE_URL "http://vortex.jammingsignal.com:8064/ml/koala/"
#endif

#define COUNT_ADDR      0xC000
#define COLOR_TEMP_ADDR 0xC010

enum Mode 
{
    RANDOM,
    SYNCED,
    INDEX
};

/* Global Variables */
unsigned char dev = 8;
int image_count = 0;  // Signed for comparisons
int index = 0;
char index_direction = 1;
char blank_on_load = 1;
char quit = 0;
enum Mode mode = RANDOM;
char* path;
char numbuf[10];
int result;
int timeout = 5;

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

void error(char* message)
{
    clrscr();
    textcolor(COLOR_WHITE);
    cprintf("%s\n", message);
}

int LoadKoalaPictureAndDisplay(char* koala_filename) 
{
    unsigned char addr[2];

    /* open the file */
    if (cbm_open(1, dev, 2, (const char *)koala_filename)) {
        error("Couldn't open file.");
        return(1);
    }

    /* read file load address */
    if (cbm_read(1, &addr, 2) != 2) {
        cbm_close(1);
        error("Couldn't read load address.\n");
        return(1);
    }

    // make sure load address is $4400 or $6000 - also allow $2000 and $0000 for images from Tom's Editor Gallery
    if (addr[0] != 0 || (addr[1] != 0x44 && addr[1] != 0x60 && addr[1] != 0x20 && addr[1] != 0x00)) {
        cbm_close(1);
        error("This doesn't look like a koala picture.\n");
        return(2);
    }

    /* load bitmap data */
    if (loadtoram(1, (unsigned char*)0x2000, 8000)) {
        cbm_close(1);        
        return(1);
    }

    /* load screen data */
    if (loadtoram(1, (unsigned char*)0x0400, 1000)) {
        cbm_close(1);      
        return(1);
    }

    /* load colour ram */
    if (loadtoram(1, (unsigned char*)0xd800, 1000)) {
        cbm_close(1);
        return(1);
    }

    /* load background+border colour into $C100 temporarily */
    if (loadtoram(1, (unsigned char*)COLOR_TEMP_ADDR, 1)) {
        cbm_close(1);
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

void blank_screen()
{
    POKE(0xd020, COLOR_BLACK);
    POKE(0xd011, 0x2b); /* disable screen */ 
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
}

unsigned int input_int()   // Do it the hard way since we don't have memory for cscanf etc.
{    
    unsigned int value_input = 0;
    char ch = ' ';
    cursor(1);

    while (ch != '\n')
    {
        if (ch >= '0' && ch <= '9')
        {
            value_input = value_input * 10 + (ch - '0');
            cprintf("%c", ch);
        }
        ch = cgetc();
    }

    cursor(0);
    return value_input;
}

char sleep_or_key(unsigned wait)
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

void update_index()
{
    index += index_direction;

    if (index >= image_count)
    {
        index = 0;  // Wrap
    }
    if (index < 0)
    {
        index = image_count - 1;  // Wrap
    }
}

void init_count()
{
    // Init Loop
    while (1)
    {
        /* Get # of images available */
        result = cbm_load(BASE_URL"count.prg", dev, NULL);

        if (result == 0)
        {    
            clrscr();
            textcolor(COLOR_RED);
            cprintf("\n\r\n\rERROR: No response from server.");
            sleep_or_key(10);
            continue;
        }

        image_count = PEEKW(COUNT_ADDR);  // TODO, use long once more than 65536 images :-)
        break;
    }
}

void show_menu()
{
    char c;

    /* Initialize Screen */
    clrscr();
    text_screen();

    while (1)
    {
        gotoxy(0, 0);
        bgcolor(COLOR_BLACK);
        bordercolor(COLOR_BLUE);
        textcolor(COLOR_LIGHTGREEN);
        cprintf("\n\r             Koala");
        textcolor(COLOR_YELLOW);
        cprintf("Scope!");
#ifdef LOCAL
        textcolor(COLOR_RED);
        cprintf(" LOCAL");
#endif 

        textcolor(COLOR_WHITE);
        cprintf("\n\r\n\r     %d ", image_count);
        textcolor(COLOR_LIGHTRED);

        cprintf("Koala images on server.\n\r\n\r");
        textcolor(COLOR_GRAY3);
        cprintf("Keys during display:\n\r\n\r SPACE to advance to next picture\n\r +/-   to move forward/backward");
        textcolor(COLOR_YELLOW);
        cprintf("*");
        textcolor(COLOR_GRAY3);
        cprintf("\n\r SHIFT to pause\n\r F1    to return to this screen\n\r STOP  to exit\n\r\n\r\n\r");
        textcolor(COLOR_LIGHTBLUE);
        cprintf("Options:\r\n\r\n B=Blank during load: ");

        if (blank_on_load)
        {
            cprintf("Yes");
        }
        else
        {
            cprintf("No ");
        }

        textcolor(COLOR_LIGHTGREEN);
        cprintf("\n\r\n\r\n\rSelect mode to start:\r\n\r\n R=Random  S=Synchronized  I=Indexed");
        textcolor(COLOR_YELLOW);
        cprintf("*\n\r\n\r");
        c = cgetc();

        switch (c)
        {
            case CH_F1:
                continue;

            case CH_F5:
                init_count();
                continue;

            case 3:  // RUN-STOP
                quit = 1;
                return;

            case 'b':
                blank_on_load = !blank_on_load;
                continue;

            case 's':
                mode = SYNCED;
                path = BASE_URL"synced.koa";
                return;

            case 'i':
                mode = INDEX;
                path = BASE_URL"index.koa?index=000000000";

                cprintf("Starting index? ");
                index = input_int();
                if (index >= image_count) index = image_count - 1;
                return;

            case ' ':
            case 'r':
            default:
                mode = RANDOM;
                path = BASE_URL"random.koa";
                return;
        }
    }
}

void display_loop()
{
    index_direction = 1;    

    while (1)
    {
        if (mode == INDEX)
        {
            itoa(index, numbuf, 10);
            strcpy(path, BASE_URL"index.koa?index=");
            strcat(path, numbuf);
        }

        
        if (blank_on_load)
        {
            blank_screen();
        }
        result = LoadKoalaPictureAndDisplay(path);
        koala_screen();

        if (result != 0)   // Show error message briefly
        {
            text_screen();
            bordercolor(2);
        }

        result = sleep_or_key(timeout);

        switch (result)
        {
            case 0:    // Timeout                    
            case ' ':
                if (mode == INDEX)
                {
                    update_index();
                }
                break;

            case 3:  // RUN-STOP
                quit = 1;
                return;

            case CH_F1:
                return;

            case '+':
                index_direction = 1;
                update_index();
                continue;

            case '-':
                index_direction = -1;
                update_index();
                continue;

            default:
                break;
        }

        pause_on_shift();
    }
}

void main() 
{
    dev = PEEK(0x00BA);   // Always use current drive#, as it's guaranteed to be enabled.

    init_count();

    while (1)
    {
        show_menu();
        if (quit) break;

        display_loop();
        if (quit) break;
    }
   
    // Clean up
    text_screen();
    clrscr();
    bordercolor(COLOR_LIGHTBLUE);
    bgcolor(COLOR_BLUE);
    textcolor(COLOR_LIGHTBLUE);
    cprintf("KoalaScope Exited\n\r");
    POKE(198, 0); /* clear keyboard queue */
}
