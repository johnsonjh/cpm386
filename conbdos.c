/*
 * CP/M-386 - conbdos.c
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * Copyright (c) 1975-1984 Digital Research, Inc.
 * SPDX-License-Identifier: MIT
 * scspell-id: 719aa4ac-82b4-11f1-8ffa-80ee73e9b8e7
 */

/*****************************************************************************/

/********************************************************
*                                                       *
*       CP/M-386 BDOS Character I/O Routines            *
*               Derived from CP/M-68K                   *
*                                                       *
*       This module does BDOS functions 1 thru 11       *
*                                                       *
*       It contains the following functions which       *
*       are called from the BDOS main routine:          *
*               constat();                              *
*               conin();                                *
*               tabout();                               *
*               rawconio();                             *
*               prt_line();                             *
*               readline();                             *
*                                                       *
*       Modified 2/5/84 sw Allow typeahead              *
*                          ^C warmboot modifications    *
*       Again   3/17/84 sw Chain hack                   *
*                                                       *
********************************************************/

#include "bdosinc.h"

#include "bdosdef.h"

#include "biosdef.h"


#define ctrlc 0x03
#define ctrle 0x05
#define ctrlp 0x10
#define ctrlq 0x11
#define ctrlr 0x12
#define ctrls 0x13
#define ctrlu 0x15
#define ctrlx 0x18

#define cr 0x0d
#define lf 0x0a
#define tab 0x09
#define rub 0x7f
#define bs 0x08
#define space 0x20


EXTERN  void warmboot(WORD);    /* External function definition */


/******************/
/* console status */
/******************/

BOOLEAN constat()
{
    BSETUP

    return( GBL.kbchar ? TRUE : bconstat() );
}

/********************/
/* check for ctrl/s */
/* used internally  */
/********************/

/*
 * Called before every cooked console character (BDOS 2).
 *
 * Ctrl-S pauses output.  Classic CP/M resumes on *any* following key
 * (not only Ctrl-Q).  That matters here because host serial PTYs often
 * enable IXOFF and inject XOFF (Ctrl-S) when a flood of dump output
 * fills the line discipline - users then hit Enter to continue.
 *
 * The wake-up key after a pause is discarded (not typed ahead), matching
 * typical CP/M 2 behavior so it does not pollute the next CCP command.
 */

conbrk()
{
    REG UBYTE ch;
    REG BOOLEAN stop;
    BSETUP

    stop = FALSE;
    if ( ! bconstat() )
        return;

    do
    {
        ch = bconin();

        if ( ch == ctrlc )
            warmboot(2);            /*sw from (1) */

        if ( ch == ctrls ) {
            stop = TRUE;            /* XOFF / Ctrl-S: pause output */

            continue;
        }

        if ( stop ) {
            /* Any key (incl. Ctrl-Q or CR) resumes; drop the wake-up char. */
            if ( ch == ctrlp )
                GBL.lstecho = !GBL.lstecho;
            stop = FALSE;

            continue;
        }

        if ( ch == ctrlq )
            continue;               /* lone Ctrl-Q: ignore */

        if ( ch == ctrlp ) {
            GBL.lstecho = !GBL.lstecho;

            continue;
        }

        /* Type-ahead for normal keys seen during output */
        if ( GBL.kbchar < TBUFSIZ ) {
            *GBL.insptr++ = ch;
            GBL.kbchar++;
        }
    } while ( stop || bconstat() );
}


/******************/
/* console output */
/* used internally*/
/******************/

conout(ch)
REG UBYTE ch;
{
    BSETUP

    conbrk();                   /* check for control-s break */
    bconout(ch);                /* output character to console */
    if (GBL.lstecho) blstout(ch);       /* if ctrl-p on, echo to list dev */
    if ((UWORD)ch >= (UWORD)' ')
        GBL.column++;           /* keep track of screen column */
    else if (ch == cr) GBL.column = 0;
    else if (ch == bs) GBL.column--;
}


/*************************************/
/* console output with tab expansion */
/*************************************/

tabout(ch)
REG UBYTE ch;           /* character to output to console       */
{
    BSETUP

    if (ch == tab) do
        conout(' ');
    while (GBL.column & 7);
    else conout(ch);
}

/*******************************/
/* console output with tab and */
/* control character expansion */
/*******************************/

cookdout(ch)
REG UBYTE ch;           /* character to output to console       */
{
    if (ch == tab) tabout(ch);  /* if tab, expand it    */
    else
    {
        if ( (UWORD)ch < (UWORD)' ' )
        {
            conout( '^' );
            ch |= 0x40;
        }
    conout(ch);                 /* output the character */
    }
}


/*****************/
/* console input */
/*****************/

UBYTE getch()           /* Get char from buffer or bios */
                        /* For internal use only        */
{
    REG UBYTE temp;
    BSETUP

    if(GBL.kbchar)
    {
        temp = *GBL.remptr++;           /* Fetch the character    */
        GBL.kbchar--;                   /* Decrement the count    */
        if(!GBL.kbchar)                 /* Gone to zero?          */
                GBL.remptr = GBL.insptr = &(GBL.t_buff [0]);
        return(temp);
    }
    return( bconin() );                 /* else get char from bios */
}

UBYTE conin()           /* BDOS console input function */
{
    REG UBYTE ch;
    BSETUP

    conout( ch = getch() );
    if (ch == ctrlp) GBL.lstecho = !GBL.lstecho;
    return(ch);
}

/******************
* raw console i/o *
******************/

UBYTE rawconio(parm)    /* BDOS raw console I/O function */

REG UWORD parm;
{
    BSETUP

    if (parm == 0xff) return(getch());
    else if (parm == 0xfe) return(constat());
    else if (parm == 0xfd) return(getch()); /* CP/M 3: wait, no echo */
    else bconout(parm & 0xff);
}


/****************************************************/
/* print line up to delimiter($) with tab expansion */
/****************************************************/

prt_line(p)
REG UBYTE *p;
{
    BSETUP
    while( *p != GBL.delim ) tabout( *p++ );
}


/**********************************************/
/* read line with editing and bounds checking */
/**********************************************/

/* Two subroutines first */

newline(startcol)
REG UWORD startcol;
{
    BSETUP

    conout(cr);                 /* go to new line */
    conout(lf);
    while(startcol)
    {
        conout(' ');
        startcol -= 1;          /* start output at starting column */
    }
}


backsp(bufp, col)
/* backspace one character position     */
REG struct conbuf *bufp;        /* pointer to console buffer    */
REG WORD col;                   /* starting console column      */
{
    REG UBYTE   ch;             /* current character            */
    REG WORD    i;
    REG UBYTE   *p;             /* character pointer            */
    BSETUP

    if (bufp->retlen) --(bufp->retlen);
                                /* if buffer non-empty, decrease it by 1 */
    i = UBWORD(bufp->retlen);   /* get new character count      */
    p = &(bufp->cbuf [0]);      /* point to character buffer    */
    while (i--)                 /* calculate column position    */
    {                           /*  across entire char buffer   */
        ch = *p++;              /* get next char                */
        if ( ch == tab )
        {
            col += 8;
            col &= ~7;          /* for tab, go to multiple of 8 */
        }
        else if ( (UWORD)ch < (UWORD)' ' ) col += 2;
                                /* control chars put out 2 printable chars */
        else col += 1;
    }
    while (GBL.column > col)
    {
        conout(bs);             /* backspace until we get to proper column */
        conout(' ');
        conout(bs);
    }
}


readline(p)                     /* BDOS function 10 */
REG struct conbuf *p;

{
    REG UBYTE ch;
    REG UWORD i;
    UWORD stcol;
    UWORD retlen;
#ifdef  NFG                     /*sw This didn't work for SUBMIT files...*/
    REG UWORD j;
    REG UBYTE *q;
#endif

    BSETUP

    stcol = GBL.column;         /* set up starting column */

#ifdef  NFG                     /*sw This didn't work for SUBMIT files...*/
    if (GBL.chainp != NULL)     /* chain to program code  */
    {
        i = UBWORD(*(GBL.chainp++));
        j = UBWORD(p->maxlen);
        if (j < i) i = j;               /* don't overflow console buffer! */
        p->retlen = (UBYTE)i;
        q = p->cbuf;
        while (i)
        {
            cookdout( *q++ = *(GBL.chainp++) );
            i -= 1;
        }
        GBL.chainp = NULL;
        return;
    }
#endif                          /*sw NFG chain code             */

    p->retlen = 0;              /* start out with empty buffer */
    while ( UBWORD(p->retlen) < UBWORD(p->maxlen) )
    {                           /* main loop for read console buffer */

        if ( ((ch=getch()) == ctrlc) && !(p->retlen) )
        {
            cookdout(ctrlc);
            warmboot(2);        /*sw From warmboot(1)   */
        }

        else if ( (ch == cr) || (ch == lf) )
        {                               /* if cr or lf, exit */
            conout(cr);
            break;
        }

        else if (ch == bs) backsp(p, stcol);    /* backspace */

        else if (ch == rub)                     /* delete character */
        {
            if (GBL.echodel)
            {
                if (p->retlen)
                {
                    i = UBWORD(--(p->retlen));
                    conout( p->cbuf [i] );
                }
            }
            else backsp(p, stcol);
        }

        else if (ch == ctrlp) GBL.lstecho = !GBL.lstecho;
                                                /* control-p */
        else if (ch == ctrlx)                   /* control-x */
            do backsp(p,stcol); while (p->retlen);

        else if (ch == ctrle) newline(stcol);   /* control-e */

        else if (ch == ctrlu)                   /* control-u */
        {
            conout('#');
            newline(stcol);
            p->retlen = 0;
        }

        else if (ch == ctrlr)                   /* control-r */
        {
            conout('#');
            newline(stcol);
            retlen = UBWORD(p->retlen);
            for (i=0; i < retlen; i++)
                    cookdout( p->cbuf [i] );
        }

        else                                    /* normal character */
            cookdout( p->cbuf [UBWORD((p->retlen)++)] = ch );
    }
}

/*****************************************************************************/

/*
 * Local Variables:
 * mode: c
 * indent-tabs-mode: nil
 * tab-width: 2
 * c-basic-offset: 2
 * fill-column: 80
 * eval: (setq-local display-fill-column-indicator-column 80)
 * eval: (display-fill-column-indicator-mode 1)
 * End:
 */

/*****************************************************************************/
/* vim: set ft=c ts=2 sw=2 tw=0 ai expandtab cc=80 : */
/*****************************************************************************/
