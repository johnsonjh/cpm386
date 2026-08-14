/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * Copyright (c) 1975-1984 Digital Research, Inc.
 * SPDX-License-Identifier: MIT
 * scspell-id: 60bd7cfe-82b4-11f1-8575-80ee73e9b8e7
 */

/*****************************************************************************/

/*--------------------------------------------------------------*\
 |      ccp.c         CONSOLE COMMAND PROCESSOR          v1.1   |
 |                    =========================                 |
 |                                                              |
 |      CP/M-386:  A CP/M-68K derived operating system          |
 |                                                              |
 |      Description:                                            |
 |      -----------                                             |
 |                      The Console Command Processor is a      |
 |                      distinct program which references       |
 |                      the BDOS to provide a human-oriented    |
 |                      interface for the console user to the   |
 |                      information maintained by the BDOS on   |
 |                      disk storage.                           |
 |                                                              |
 |      created by :    Tom Saulpaugh   Date created: 7/13/82   |
 |      ----------                      ------------            |
 |      last modified:  03/17/84 sw     St. Patrick's Day!!!    |
 |      -------------   Chain hack      --------------------    |
 |                                                              |
 |      (c) COPYRIGHT  Digital Research, Inc. 1983, 1984        |
 |                                                              |
\*--------------------------------------------------------------*/

/*****************************************************************************/

/*--------------------------------------------------------------*\
 |                   CCP Macro Definitions                      |
\*--------------------------------------------------------------*/
#include        "ccpdef.h"        /* include CCP defines        */

/*****************************************************************************/

/*--------------------------------------------------------------*\
 |                  CP/M Builtin Command Table                  |
\*--------------------------------------------------------------*/
struct _cmd_tbl
{
        BYTE    *ident;         /* command identifier field     */
        UWORD   cmd_code;       /* command code field           */
};

struct _cmd_tbl cmd_tbl [] =
{
        {"DIR",DIRCMD},
        {"DIRS",DIRSCMD},
        {"TYPE",TYPECMD},
        {"REN",RENCMD},
        {"ERA",ERACMD},
        {"USER",UCMD},
        {"SUBMIT",SUBCMD},
        {"GO",GOCMD},           /* re-exec last .386 still in the TPA */
        {"QUIET",QUIETCMD},     /* submit echo control; no-op interactive */
        {0, (UWORD)-1}
};

/*****************************************************************************/

/*--------------------------------------------------------------*\
 |              CP/M-386 COMMAND FILE LOADER TABLE              |
\*--------------------------------------------------------------*/
extern struct _filetyps
{
        BYTE *typ;
        UWORD (*loader) ();
        BYTE user_c;
        BYTE user_0;
}
load_tbl [];

/*****************************************************************************/

/*--------------------------------------------------------------*\
 |               Table of User Prompts and Messages             |
\*--------------------------------------------------------------*/
BYTE      msg  [] = "NON-SYSTEM FILE(S) EXIST$";
BYTE      msg2 [] = "Enter Filename: $";
BYTE      msg3 [] = "Enter Old Name: $";
BYTE      msg4 [] = "Enter New Name: $";
BYTE      msg5 [] = "File already exists$";
BYTE      msg6 [] = "No file$";
BYTE      msg7 [] = "No wildcard filenames$";
BYTE      msg8 [] = "Syntax: REN Newfile=Oldfile$";
BYTE      msg9 [] = "Confirm(Y/N)? $";
BYTE     msg10 [] = "Enter User No: $";
BYTE     msg11 [] = ".SUB file not found$";
BYTE     msg12 [] = "User # range is [0-15]$";
BYTE     msg13 [] = "Too many arguments: $";
BYTE  lderrmem [] = "Insufficient memory; $";
BYTE  lderrreq [] = "K required, $";
BYTE  lderravl [] = "K available$";
BYTE  lderrhdr [] = "Bad file header$";
#if 0
BYTE  lderr2 [] = "Read error on program load$";
BYTE  lderr3 [] = "Bad relocation information bits$";
#endif
BYTE  gonomsg [] = "No program$";

/*****************************************************************************/

/*--------------------------------------------------------------*\
 |                  Global Arrays & Variables                   |
\*--------------------------------------------------------------*/

                                /********************************/
BYTE load_try;                  /* flag to mark a load try      */
BYTE first_sub;                 /* flag to save current cmd ptr */
BYTE submit = 0;                /* submit file flag             */
BYTE quiet = 0;                 /* submit echo suppress (QUIET) */
BYTE end_of_file;               /* submit end of file flag      */
BYTE dirflag;                   /* used by fill_fcb(? or blanks)*/
BYTE subprompt;                 /* submit file was prompted for */
BYTE morecmds = 0;              /* command after warmboot flag  */
UWORD sub_index;                /* index for subdma buffer      */
UWORD index;                    /* index into cmd argument array*/
UWORD sub_user;                 /* submit file user number      */
UWORD user;                     /* current default user number  */
UWORD cur_disk;                 /* current default disk drive   */
BYTE subcom [CMD_LEN+1];        /* submit command buffer        */
BYTE subdma [CMD_LEN];          /* buffer to fill from sub file */
BYTE usercmd [CMD_LEN+2] = {0}; /* user command buffer          */
BYTE *user_ptr;                 /* next user command to execute */
BYTE *glb_index;                /* points to current command    */
BYTE save_sub [CMD_LEN+1];      /* saves cur cmd line for submit*/
BYTE subfcb [FCB_LEN];          /* global fcb for sub files     */
BYTE cmdfcb [FCB_LEN];          /* global fcb for 386 files     */
BYTE *tail;                     /* pointer to command tail      */
BYTE autost = 0;                /* autostart flag               */
BYTE autorom;                   /* needed for ROM system autost */
BYTE dma [DMA_LEN+3];           /* 128 byte dma buffer          */
BYTE parm [MAX_ARGS] [ARG_LEN]; /* cmd argument array           */
BYTE *chainp;                   /*sw -> User-specified command  */
BYTE del [] =                   /* CP/M-386 set of delimiters   */
{'>','<','.',',','=','[',']',';','|','&','/','(',')','+','-','\\'};
                                /********************************/

/*****************************************************************************/

/*--------------------------------------------------------------*\
 |                    Function Definitions                      |
\*--------------------------------------------------------------*/

UWORD bdos(WORD func, LONG parm);  /* 32-bit safe proto for ptr/word param */
UWORD delim(BYTE *ch);
UWORD fill_fcb(UWORD which_parm, BYTE *fcb);
#if 0
UWORD (*ldrpgm)(BYTE *); /* ptr to load func, inited? or stub */
#endif
/*BYTE *scan_cmd();             // bare */
UWORD strcmp(BYTE *s1, BYTE *s2);       /* this returns a word      */
BYTE *skip_at(BYTE *cmd);               /* step over the '@' prefix */
/*UWORD decode();                       // bare, use implicit + def */
/*UWORD delim();                        // bare */
/*BYTE true_char();             // bare */
/*UWORD too_many();             // bare */
/*UWORD find_colon();           // bare */
/*UWORD chk_colon();            // bare */
/*UWORD user_cmd();             // bare */
/*UWORD cmd_file();             // bare */
/*UWORD sub_read();             // bare */
/*UWORD dollar();                       // bare */
/*UWORD comments();             // bare */
/*UWORD submit_cmd();           // bare */
/*BYTE true_char();*/
/*UWORD fill_fcb();             // bare */
/*UWORD too_many();*/
/*UWORD find_colon();*/
/*UWORD chk_colon();*/
/*UWORD user_cmd();*/
/*UWORD cmd_file();*/
/*UWORD sub_read();*/
/*UWORD dollar();*/
/*UWORD comments();*/
/*UWORD submit_cmd();*/
                                /********************************/
VOID cr_lf()                    /*   print a CR and a Linefeed  */
                                /********************************/
{
        bdos(CONSOLE_OUTPUT,CR);
        bdos(CONSOLE_OUTPUT,LF);
}
                                /********************************/
VOID cpy(source,dest)           /*  copy source to destination  */
                                /********************************/
REG BYTE *source;
REG BYTE *dest;
{
        while((*dest++ = *source++) != 0);
}
                                /********************************/
UWORD strcmp(s1,s2)             /*    compare 2 char strings    */
                                /********************************/
REG BYTE *s1,*s2;
{
        while(*s1)
        {
                if(*s1 > *s2)
                        return(1);
                if(*s1 < *s2)
                        return(-1);
                s1++; s2++;
        }
        return((*s2 == NULL) ? 0 : -1);
}
                                /********************************/
VOID copy_cmd(com_index)        /*  Save the command which      */
                                /*  started a submit file       */
                                /*  Parameter substitution will */
                                /*  need this command tail.     */
                                /*  The buffer save_sub is used */
                                /*  to store the command.       */
                                /********************************/
REG BYTE *com_index;
{
        REG BYTE *t1,*temp;

        temp = save_sub;
        if(subprompt)
        {
                t1 = (void *)parm;
                while(*t1)
                        *temp++ = *t1++;
                *temp++ = ' ';
                subprompt = FALSE;
        }
        while(*com_index && *com_index != EXLIMPT)
                *temp++ = *com_index++;
        *temp = NULL;
}
                                /********************************/
VOID put_dec(n)                 /*  unsigned decimal to console */
                                /********************************/
REG unsigned long n;
{
        BYTE d[12];
        REG WORD i;

        i = 0;
        if(!n)
        {
                bdos(CONSOLE_OUTPUT,(long)'0');
                return;
        }
        while(n && i < 12)
        {
                d[i++] = (BYTE)('0' + (n % 10));
                n /= 10;
        }
        while(i > 0)
                bdos(CONSOLE_OUTPUT,(long)d[--i]);
}
                                /********************************/
VOID load_error(rc)             /*  report why a load failed    */
                                /********************************/
REG UWORD rc;
{
        extern unsigned long cpm386_load_req_kb();
        extern unsigned long cpm386_load_avail_kb();

        /*-------------------------------------------------------*/
        /* A program that asked for more memory than the TPA has  */
        /* is worth being specific about; everything else is a    */
        /* malformed or unloadable file.                          */
        /*-------------------------------------------------------*/
        if(rc == (UWORD)CPMLD_NOMEM)
        {
                bdos(PRINT_STRING,lderrmem);
                put_dec(cpm386_load_req_kb());
                bdos(PRINT_STRING,lderrreq);
                put_dec(cpm386_load_avail_kb());
                bdos(PRINT_STRING,lderravl);
        }
        else
                bdos(PRINT_STRING,lderrhdr);

        cr_lf();
}
                                /********************************/
VOID prompt()                   /*   print the CCP prompt       */
                                /********************************/
{
        REG UWORD cur_drive,cur_user_no;
        BYTE      buffer[3];

        cur_user_no = bdos(GET_USER_NO,(long)255);
        cur_drive  = bdos(RET_CUR_DISK,(long)0);
        cur_drive += 'A';
        cr_lf();
        if(cur_user_no)
        {
                if(cur_user_no >= 10)
                {
                        buffer[0] = '1';
                        buffer[1] = ((cur_user_no-10) + '0');
                        buffer[2] = '$';
                }
                else
                {
                        buffer[0] = (cur_user_no + '0');
                        buffer[1] = '$';
                }
                bdos(PRINT_STRING,buffer);
        }
        bdos(CONSOLE_OUTPUT,(long)cur_drive);
        bdos(CONSOLE_OUTPUT,ARROW);
}



                                /******************************/
BYTE *skip_at(cmd)              /* step over the ZCPR-3 style */
                                /* '@' echo-suppress prefix.  */
                                /* Returns cmd unchanged when */
                                /* there is no prefix, so the */
                                /* caller can also use the    */
                                /* result to test for one.    */
                                /******************************/
REG BYTE *cmd;
{
        REG BYTE *c;

        c = cmd;
        while(*c == ' ' || *c == TAB)
                c++;
        if(*c != QUIETPFX)
                return(cmd);
        c++;
        while(*c == ' ' || *c == TAB)
                c++;
        return(c);
}
                                /******************************/
VOID echo_cmd(cmd,mode)         /* echo any multiple commands */
                                /* or any illegal commands    */
                                /******************************/
REG BYTE *cmd;
REG UWORD mode;
{
        if(mode == GOOD && submit && (quiet || skip_at(cmd) != cmd))
                return;
        if(mode == GOOD && (!(autost && autorom)))
                prompt();
        while(*cmd && *cmd != EXLIMPT)
                bdos(CONSOLE_OUTPUT,(long)*cmd++);
        if(mode == BAD)
                bdos(CONSOLE_OUTPUT,(long)'?');
        else
                cr_lf();
}

                                /********************************/
UWORD decode(cmd)               /* Recognize the command as:    */
                                /* ---------                    */
                                /* 1. Builtin                   */
                                /* 2. File                      */
                                /********************************/
REG BYTE *cmd;
{
        REG UWORD i,n;


        /****************************************/
        /* Check for a CP/M builtin command     */
        /****************************************/
#if 0
        for(i = 0; i < 7;i++)
#endif
        for(i = 0; cmd_tbl[ i ].ident != (BYTE *)0; i++ )
                if (strcmp(cmd,cmd_tbl[i].ident) == MATCH)
                        return(cmd_tbl[i].cmd_code);
        /********************************************************/
        /*      Check for a change of disk drive command        */
        /********************************************************/
        i = 0;
        while(i < (ARG_LEN-1) && parm[0][i] != ':')
                i++;
        if(i == 1 && parm[0][2] == NULL && parm[1][0] == NULL)
                if((parm[0][0] >= 'A') && (parm[0][0] <= 'P'))
                                return(CH_DISK);
        if(i == 1 && ((parm[0][0] < 'A') || (parm[0][0] > 'P')))
                return(-1);
        if(i != 1 && parm[0][i] == ':')
                return(-1);
        /*****************************************************/
        /* Check for Wildcard Filenames                      */
        /* Check Filename for a Delimiter                    */
        /*****************************************************/
        if(fill_fcb(0,cmdfcb) > 0)
                return(-1);
        if(i == 1)
                i = 2;
        else
                i = 0;
        if(delim(&parm[0][i]))
                return(-1);
        for(n = 0;n < ARG_LEN-1 && parm[0][n];n++)
                if(parm[0][n] < ' ')
                        return(-1);
        return(FILE);
}
                                        /************************/
VOID check_cmd(tcmd)                    /*  Check end of cmd    */
                                        /*  for an '!' which    */
                                        /*  starts another cmd  */
REG BYTE *tcmd;                         /************************/
{
        while(*tcmd && *tcmd != EXLIMPT)
                tcmd++;
        /*----------------------------*/
        /* check for multiple command */
        /*   in case of a warmboot    */
        /*----------------------------*/
        if(*tcmd++ == EXLIMPT && *tcmd)
        {
                morecmds = TRUE;
                while(*tcmd == ' ')
                        tcmd++;
                user_ptr = tcmd;
        }
        else
                if(submit)      /* check original cmd line */
                {
                        if(!(end_of_file))
                                morecmds = TRUE;
                                /*--------------------------*/
                        else    /* restore cmd to where user*/
                                /* ptr points to. User_ptr  */
                                /* always points to next    */
                                /* console command to exec  */
                        {       /*--------------------------*/
                                submit = FALSE;
                                quiet = FALSE;  /* echo back on at end */
                                if(*user_ptr)
                                        morecmds = TRUE;
                        }
                }
                else
                        morecmds = FALSE;
}
                                        /************************/
VOID get_cmd(cmd,max_chars)             /*   Read in a command  */
                                        /*Strip off extra blanks*/
                                        /************************/
REG BYTE  *cmd;
REG long max_chars;
{
        REG BYTE *c;

        max_chars += ((int)cmd - 1);
        dma[0] = (BYTE)CMD_LEN;   /* set maximum chars to read  */
        bdos(READ_CONS_BUF,dma);  /* then read console          */
        if(dma[1] != 0 && dma[2] != ';')
                cr_lf();
        dma[((UWORD)dma[1] & 0xFF)+2] = '\n'; /* tack on end of line char   */
        if(dma[2] == ';')         /* ';' denotes a comment      */
                dma[2] = '\n';
        c = &dma[2];
        while(*c == ' ' || *c == TAB)
                c++;
        while(*c != '\n' && (int)cmd < max_chars)
        {
                *cmd++ = toupper(*c);
                if(*c == ' ' || *c == TAB)
                        while(*++c == ' ' || *c == TAB);
                else
                        c++;
        }
        *cmd = NULL;         /* tack a null character on the end*/
}
                                        /************************/
BYTE *scan_cmd(com_index)               /* move ptr to next cmd */
                                        /* in the command line  */
                                        /************************/
REG BYTE *com_index;
{
        while((*com_index != EXLIMPT) &&
              (*com_index))
                 com_index++;
        while(*com_index == EXLIMPT || *com_index == ' ' ||
              *com_index == TAB)
                com_index++;
        return(com_index);
}

                                        /************************/
VOID get_parms(cmd)                     /* extract cmd arguments*/
                                        /* from command line    */
REG BYTE *cmd;                          /************************/
{



        /************************************************/
        /*      This function parses the command line   */
        /* read in by get_cmd().  The expected command  */
        /* from that line is put into parm[0].  All     */
        /* parmeters associated with the command are put*/
        /* in in sequential order in parm[1],parm[2],   */
        /* and parm[3]. A command ends at a NULL or     */
        /* an exlimation point.                         */
        /************************************************/


        REG BYTE *line;         /* pointer to parm array   */
        REG UWORD    i;         /* Row Index               */
        REG UWORD    j;         /* Column Index            */

        line = (void *)parm;
        for(i = 0; i < (MAX_ARGS * ARG_LEN); i++)
                *line++ = NULL;

        i = 0;
        /***************************************************/
        /*  separate command line at blanks,exlimation pts */
        /***************************************************/

        while(*cmd != NULL    &&
              *cmd != EXLIMPT &&
              i < MAX_ARGS)
        {
                j = 0;
                while(*cmd != EXLIMPT &&
                      *cmd != ' '     &&
                      *cmd != TAB     &&
                      *cmd)
                {
                        if(j < (ARG_LEN-1))
                                parm[i][j++] = *cmd;
                        cmd++;
                }
                parm[i++][j] = NULL;
                if(*cmd == ' ' || *cmd == TAB)
                        cmd++;
                if(i == 1)
                        tail = cmd; /* mark the beginning of the tail */
        }
}

                                        /************************/
UWORD delim(ch)                         /* check ch to see      */
                                        /* if it's a delimiter  */
                                        /************************/
REG BYTE *ch;
{
        REG UWORD i;

        if(*ch <= ' ')
                return(TRUE);
        for(i = 0;i < sizeof (del);i++)
                if(*ch == del[i]) return(TRUE);
        return(FALSE);
}



                                        /************************/
BYTE true_char(ch)                      /* return the desired   */
                                        /* character for fcb    */
                                        /************************/

REG BYTE *ch;
{
        if(*ch == '*') return('?');     /* wildcard             */

        if(!delim(ch))                  /* ascii character      */
        {
                index++;                /* increment cmd index  */
                return(*ch);
        }

        return(' ');                    /* pad field with blank */
}
                                        /************************/
UWORD fill_fcb(REG UWORD which_parm,    /* fill the fields of   */
               REG BYTE *fcb)           /* the file control blk */
                                        /************************/
{
        REG BYTE  *ptr;
        REG BYTE  fillch;
        REG UWORD j,k;

        *fcb = 0;
        for(k = 12;k <= 35; k++)        /* fill fcb with zero   */
                fcb[k] = ZERO;
        for(k = 1;k <= 11;k++)
                fcb[k] = BLANK; /* blank filename+type  */

        /*******************************************/
        /* extract drivecode,filename and filetype */
        /*       from parameter blk                */
        /*******************************************/

        if(dirflag)
                fillch = '?';
        else
                fillch = ' ';

        index = ZERO;
        ptr = fcb;
        if(parm[which_parm][index] == NULL) /* no parameters  */
        {
                ptr++;
                for(j = 1;j <= 11;j++)
                        *ptr++ = fillch;
                *fcb = (bdos(RET_CUR_DISK,(long)0)+1);
                if(dirflag)
                        return(11);
                else
                        return(0);
        }
        if(parm[which_parm][index+1] == ':')
        {
                *ptr = parm[which_parm][index] - 'A' + 1;
                index += 2;
                if(parm[which_parm][index] == NULL)
                {
                        ptr = &fcb[1];
                        for(j = 1;j <= 11;j++)
                                *ptr++ = fillch;
                        if(dirflag)
                                return(11);
                        else
                                return(0);
                }
        }
        else    /* fill drivecode with the default disk         */
                     *fcb = (bdos(RET_CUR_DISK,(long)0) + 1);

        ptr = fcb;
        ptr++;                  /* set pointer to fcb filename */
        for(j = 1;j <= 8;j++)   /* get filename */
          *ptr++ = true_char(&parm[which_parm][index]);
        while((!(delim(&parm[which_parm][index])))) index++;
        if(parm[which_parm][index] == PERIOD)
        {
                index++;
                for(j = 1;j <= 3;j++)   /* get extension */
                        *ptr++ = true_char(&parm[which_parm][index]);
        }
        k = 0;
        for(j = 1;j <= 11;j++)
                if(fcb[j] == '?') k++;

        return(k);      /* return the number of question marks  */
}
                                        /************************/
UWORD too_many()                        /* too many args ?      */
{                                       /************************/
        if(parm[2][0])
        {
                bdos(PRINT_STRING,msg13);
                echo_cmd(&parm[2][0],BAD);
                return(TRUE);
        }
        return(FALSE);
}
                                        /************************/
UWORD find_colon()                      /*  search for a colon  */
{                                       /************************/
        REG UWORD i;

        i = 0;
        while(parm[1][i] && parm[1][i] != ':') i++;
        return(i);
}
                                        /************************/
UWORD chk_colon(j)                      /* check the position of*/
UWORD j;                                /* the colon and for    */
{                                       /* a legal drive letter */
        if(parm[1][j] == ':')           /************************/
        {
                if(j != 1 || parm[1][0] < 'A' || parm[1][0] > 'P')
                {
                        echo_cmd(&parm[1][0],BAD);
                        return(FALSE);
                }
        }
        return(TRUE);
}



                                        /************************/
VOID dir_cmd(attrib)                    /*     print out a      */
                                        /*  directory listing   */
                                        /*----------------------*/
                                        /* attrib->1 (sysfiles) */
                                        /* attrib->0 (dirfiles) */
                                        /************************/
REG UWORD attrib;
{
        BYTE            needcr_lf;
        REG UWORD       dir_index,file_cnt;
        REG UWORD       save,j,curdrive,exist;

        exist = FALSE; needcr_lf = FALSE;
        if(too_many()) return;
        j = find_colon();
        if(!chk_colon(j)) return;
        fill_fcb(1,cmdfcb);
        curdrive = (cmdfcb[0] + 'A' - 1);
        dir_index = bdos(SEARCH_FIRST,cmdfcb);
        if(dir_index == 255)
                bdos(PRINT_STRING,msg6);
        save = (32 * dir_index) + 1;
        file_cnt = 0;
        while(dir_index != 255)
        {
                /* Only list the first directory entry of each file.  Multi-extent
                 * files (BIG.386) have further entries with extent 1,2,... or
                 * non-zero module (s2).  save->fname[0]; extent at save+11;
                 * s2 at save+13.  cpmtools writes one logical extent per dirent. */
                if ( (dma[save + 11] & 0x1f) != 0 || (dma[save + 13] & 0x3f) != 0 )
                {
                        dir_index = bdos(SEARCH_NEXT, 0);
                        save = (32 * dir_index) + 1;

                        continue;
                }
                if(((attrib) && (dma[save+9] & 0x80)) ||
                  (!(attrib) && (!(dma[save+9] & 0x80))))
                {
                        if(needcr_lf)
                        {
                                cr_lf();
                                needcr_lf = FALSE;
                        }
                        if(file_cnt == 0)
                                bdos(CONSOLE_OUTPUT,(long)curdrive);
                }
                else
                        {
                                exist = TRUE;
                                dir_index = bdos(SEARCH_NEXT, 0);
                                save = (32 * dir_index) + 1;

                                continue;
                        }
                dir_index = (32 * dir_index) + 1;
                bdos(CONSOLE_OUTPUT,COLON);
                bdos(CONSOLE_OUTPUT,BLANKS);
                j = 1;
                while(j <= 11)
                {
                        if(j == 9)
                                bdos(CONSOLE_OUTPUT,BLANKS);
                        bdos(CONSOLE_OUTPUT,(long)(dma[dir_index++] & CMASK));
                        j++;
                }
                bdos(CONSOLE_OUTPUT,BLANKS);
                dir_index = bdos(SEARCH_NEXT, 0);
                if(dir_index == 255)
                        break;
                file_cnt++;
                save = (32 * dir_index) + 1;
                if(file_cnt == 5)
                {
                        file_cnt = 0;
                        if((attrib && (dma[save+9] & 0x80)) ||
                          (!(attrib) && (!(dma[save+9] & 0x80))))
                                cr_lf();
                        else
                                needcr_lf = TRUE;
                }

        }               /*----------------------------------------*/
        if(exist)       /* if files exist that were not displayed */
                        /* print out a message to the console     */
        {               /*----------------------------------------*/
                cr_lf();
                if(attrib)
                        bdos(PRINT_STRING,msg);
                else
                        bdos(PRINT_STRING,&msg[4]);
        }
}




                                        /************************/
VOID type_cmd()                         /*   type out a file    */
                                        /*   to the console     */
                                        /************************/
{
        REG     UWORD i;

        if(parm[1][0] == NULL)          /*prompt user for filename*/
        {
                bdos(PRINT_STRING,msg2);
                get_cmd(&parm[1][0],(long)(ARG_LEN-1));
        }
        if(too_many())
                return;
        i = find_colon();
        if(!chk_colon(i)) return;
        i = fill_fcb(1,cmdfcb);         /*fill a file control block*/
        if(i == 0 && parm[1][0] && (bdos(OPEN_FILE,cmdfcb) <= 3))
        {
                while(bdos(READ_SEQ,cmdfcb) == 0)
                {
                        for(i = 0;i <= 127;i++)
                                if(dma[i] != EOF)
                                    bdos(CONSOLE_OUTPUT,(long)dma[i]);
                                else
                                        break;
                }
                bdos(RESET_DRIVE,(long)cmdfcb[0]);
        } else
                if(parm[1][0])
                {
                        if(i > 0)
                                bdos(PRINT_STRING,msg7);
                        else
                                bdos(PRINT_STRING,msg6);
                }
}


                                        /************************/
VOID ren_cmd()                          /*    rename a file     */
                                        /************************/

{
        BYTE            new_fcb[FCB_LEN];
        REG UWORD       i,j,k,bad_cmd;

        bad_cmd = FALSE;               /*-------------------------*/
        if(parm[1][0] == NULL)         /*prompt user for filenames*/
        {                              /*-------------------------*/
                bdos(PRINT_STRING,msg3);
                get_cmd(&parm[3][0],(long)(ARG_LEN-1));
                if(parm[3][0] == NULL)
                        return;
                bdos(PRINT_STRING,msg4);
                get_cmd(&parm[1][0],(long)(ARG_LEN-1));
                parm[2][0] = '=';
        }                       /*--------------------------------*/
         else                   /*check for correct command syntax*/
         {                      /*--------------------------------*/
                i = 0;
                while(parm[1][i] != '=' && parm[1][i]) i++;
                if(parm[1][i] == '=')
                {
                        if(!(i > 0 && parm[1][i+1] &&
                             parm[2][0] == NULL))
                                bad_cmd = TRUE;
                }
                else
                        if(!(parm[2][0] == '='  &&
                             parm[2][1] == NULL &&
                             parm[3][0]))
                                bad_cmd = TRUE;
                if(!bad_cmd && parm[1][i] == '=')
                {
                        parm[1][i] = NULL;
                        i++;
                        j = 0;
                        while((parm[3][j++] = parm[1][i++]) != NULL);
                        parm[2][0] = '=';
                }
        }
        for(j = 1;j < 4;j += 2)
        {
                k = 0;
                while(parm[j][k] != ':' && parm[j][k])
                        k++;
                if(k > 1 && parm[j][k] == ':')
                        bad_cmd = TRUE;
                for(i = 0;i < sizeof del;i++)
                        if(parm[j][0] == del[i])
                        {
                                echo_cmd(&parm[j][0],BAD);
                                return;
                        }
        }
        if(!bad_cmd && parm[1][0] && parm[3][0])
        {
                i = fill_fcb(1,new_fcb);
                j = fill_fcb(3,cmdfcb);
                if(i == 0 && j == 0)
                {
                        if(new_fcb[0] != cmdfcb[0])
                        {
                                if(parm[1][1] == ':' && parm[3][1] != ':')
                                        cmdfcb[0] = new_fcb[0];
                                else
                                if(parm[1][1] != ':' && parm[3][1] == ':')
                                        new_fcb[0] = cmdfcb[0];
                                else
                                bad_cmd = TRUE;
                        }
                        if(new_fcb[0] < 1 || new_fcb[0] > 16)
                                bad_cmd = TRUE;
                        if(!(bad_cmd) && bdos(SEARCH_FIRST,new_fcb) != 255)
                                bdos(PRINT_STRING,msg5);
                        else{
                                k = 0;
                                for(i = 16;i <= 35;i++)
                                        cmdfcb[i] = new_fcb[k++];
                                if(cmdfcb[0] < 0 || cmdfcb[0] > 15)
                                        bad_cmd = TRUE;
                                if(!(bad_cmd) &&
                                bdos(RENAME_FILE,cmdfcb) > 0)
                                        bdos(PRINT_STRING,msg6);
                            }
                }
                else
                 bdos(PRINT_STRING,msg7);
        }
        if(bad_cmd)
                 bdos(PRINT_STRING,msg8);
}

                                        /************************/
VOID era_cmd()                          /*  erase a file from   */
                                        /*  the directory       */
                                        /************************/
{
        REG     UWORD i;
                                        /*----------------------*/
        if(parm[1][0] == NULL)          /* prompt for a file    */
        {                               /*----------------------*/
                bdos(PRINT_STRING,msg2);
                get_cmd(&parm[1][0],(long)(ARG_LEN-1));
        }
        if(parm[1][0] == NULL)
                return;
        if(too_many())
                return;
        i = find_colon();
        if(!chk_colon(i))
                return;
        else
                if(parm[1][1] == ':' && parm[1][2] == NULL)
                {
                        echo_cmd(&parm[1][0],BAD);
                        return;
                }
        i = fill_fcb(1,cmdfcb);         /* fill an fcb     */
        if(i > 0 && !(submit))          /* no confirmation */
        {                               /* if submit file  */
                bdos(PRINT_STRING,msg9);
                parm[2][0] = bdos(CONIN,(long)0);
                parm[2][0] = toupper(parm[2][0]);
                cr_lf();
                if(parm[2][0] != 'N' && parm[2][0] != 'Y')
                        return;
        }
        if(parm[2][0] != 'N')
                if(bdos(DELETE_FILE,cmdfcb) > 0)
                        bdos(PRINT_STRING,msg6);
}
                                        /************************/
UWORD user_cmd()                        /* change user number   */
                                        /************************/

{
        REG UWORD i;

        if(parm[1][0] == NULL)          /* prompt for a number  */
        {
                bdos(PRINT_STRING,msg10);
                get_cmd(&parm[1][0],(long)(ARG_LEN-1));
        }
        if(parm[1][0] == NULL)
                return(TRUE);
        if(too_many())
                return(TRUE);
        if(parm[1][0] < '0' || parm[1][0] > '9')
                return(FALSE);
        i = (parm[1][0] - '0');
        if(i > 9)
                return(FALSE);
        if(parm[1][1])
                i = ((i * 10) + (parm[1][1] - '0'));
        if(i < 16 && parm[1][2] == NULL)
                bdos(GET_USER_NO,(long)i);
        else
                return(FALSE);
        return(TRUE);
}

#define MAX_SUB_LEVEL 8
static struct sub_context {
    BYTE  subprompt;
    BYTE  quiet;
    UWORD sub_index;
    UWORD sub_user;
    BYTE  *user_ptr;
    BYTE  subcom[CMD_LEN+1];
    BYTE  subdma[CMD_LEN];
    BYTE  subfcb[FCB_LEN];
    BYTE  save_sub[CMD_LEN+1];
} sub_stack[MAX_SUB_LEVEL];

static UWORD sub_level = 0;

static void push_submit(void)
{
    if (sub_level < MAX_SUB_LEVEL) {
        struct sub_context *c = &sub_stack[sub_level++];
        int i;
        c->subprompt = subprompt;
        c->quiet = quiet;
        c->sub_index = sub_index;
        c->sub_user = sub_user;
        c->user_ptr = user_ptr;
        for (i = 0; i <= CMD_LEN; i++) c->subcom[i] = subcom[i];
        for (i = 0; i < CMD_LEN; i++) c->subdma[i] = subdma[i];
        for (i = 0; i < FCB_LEN; i++) c->subfcb[i] = subfcb[i];
        for (i = 0; i <= CMD_LEN; i++) c->save_sub[i] = save_sub[i];
    }
}

static int pop_submit(void)
{
    if (sub_level > 0) {
        struct sub_context *c = &sub_stack[--sub_level];
        int i;
        subprompt = c->subprompt;
        quiet = c->quiet;
        sub_index = c->sub_index;
        sub_user = c->sub_user;
        user_ptr = c->user_ptr;
        for (i = 0; i <= CMD_LEN; i++) subcom[i] = c->subcom[i];
        for (i = 0; i < CMD_LEN; i++) subdma[i] = c->subdma[i];
        for (i = 0; i < FCB_LEN; i++) subfcb[i] = c->subfcb[i];
        for (i = 0; i <= CMD_LEN; i++) save_sub[i] = c->save_sub[i];
        submit = TRUE;
        end_of_file = FALSE;
        return 1;
    }
    return 0;
}

UWORD cmd_file(mode)
                                        /************************/
                                        /*                      */
                                        /*      SEARCH ORDER    */
                                        /*      ============    */
                                        /*                      */
                                        /* 1. 386 type on the   */
                                        /*    current user #    */
                                        /* 2. BLANK type on     */
                                        /*    current user #    */
                                        /* 3. SUB type on the   */
                                        /*    current user #    */
                                        /* 4. 386 type on       */
                                        /*    user 0            */
                                        /* 5. BLANK type on the */
                                        /*    user 0            */
                                        /* 6. SUB type on       */
                                        /*    user 0            */
                                        /*                      */
                                        /*----------------------*/
                                        /*                      */
                                        /*   If a filetype is   */
                                        /*   specified then I   */
                                        /*   search the current */
                                        /*   user # then user 0 */
                                        /*                      */
                                        /************************/
UWORD mode;
{
        BYTE done,open,sub_open;
        BYTE found;
        REG UWORD i,n;
        REG BYTE *top;
        REG struct _filetyps *p;

        dirflag = FALSE;
        load_try = TRUE;
        done = FALSE;
        found = FALSE;
        sub_open = FALSE;
        open = FALSE;

        user = bdos(GET_USER_NO,(long)255);
        cur_disk = bdos(RET_CUR_DISK,(long)0);
        if(mode == SEARCH) i = 0; else i = 1;
        i = fill_fcb(i,cmdfcb);
        if(i > 0)
        {
                bdos(PRINT_STRING,msg7);
                return(FALSE);
        }
        p = (void *)&load_tbl;
        top = p->typ;
        if(cmdfcb[9] == ' ')
        {
           while(*(p->typ)) /* clear flags in table */
           {
                p->user_c = p->user_0 = FALSE;
                p++;
           }
           bdos(SELECT_DISK,(long)(cmdfcb[0]-1));
           cmdfcb[0] = '?'; cmdfcb[12] = NULL;
           /* wild type so scan returns candidates of all load_tbl types (e.g. .386); strcmp inside filters */
           cmdfcb[9] = cmdfcb[10] = cmdfcb[11] = '?';
           i = bdos(SEARCH_FIRST,cmdfcb);
           while(i != 255 && !(done))
           {
                i *= 32;
                if(dma[i] == 0 || dma[i] == user)
                {
                   for(n = 9;n <= 11;n++) dma[i+n] &= CMASK;
                   dma[i+12] = NULL;
                   p = (void *)&load_tbl;
                   while(*(p->typ))
                   {
                      cpy(p->typ,&cmdfcb[9]);
                      if(strcmp(&cmdfcb[1],&dma[i+1]) == MATCH)
                      {
                         found = TRUE;
                         if(dma[i] == user) p->user_c = TRUE;
                         else              p->user_0 = TRUE;
                         if(mode == SEARCH &&
                           strcmp(p->typ,top) == MATCH && p->user_c)
                                done = TRUE;
                      }
                      p++;
                   }
                }
                i = bdos(SEARCH_NEXT, 0);
           }
           if(!(found))
           {
                if(mode == SUB_FILE) bdos(PRINT_STRING,msg11);
                dirflag = TRUE; load_try = FALSE;
                bdos(SELECT_DISK,(long)cur_disk); return(FALSE);
           }
           if(mode == SEARCH)
                fill_fcb(0,cmdfcb);
           else
                fill_fcb(1,cmdfcb);
           p = (void *)&load_tbl;
           while(*(p->typ))
           {
                if(p->user_c || p->user_0)
                {
                   if(mode == SEARCH) break;
                   if(strcmp(p->typ,"SUB") == MATCH) break;
                }
                p++;
           }
           if(*(p->typ))
           {
                if(!(p->user_c)) bdos(GET_USER_NO,(long)0);
                cpy(p->typ,&cmdfcb[9]);
           }
        }
        bdos(SELECT_DISK,(long)cur_disk);
        while(1)
        {
           if(bdos(OPEN_FILE,cmdfcb) <= 3)
           {
                for(n = 9;n <= 11;n++) cmdfcb[n] &= CMASK;
                if(cmdfcb[9] == 'S' && cmdfcb[10] == 'U' && cmdfcb[11] == 'B')
                {
                   sub_open = TRUE;
                   sub_user = bdos(GET_USER_NO,(long)255);
                   if(submit) push_submit();
                   first_sub = TRUE;
                   for(i = 0;i < FCB_LEN;i++)
                        subfcb[i] = cmdfcb[i];
                   if(mode == SEARCH) subprompt = FALSE;
                   submit = TRUE;
                   quiet = FALSE; /* each script starts with echoing on */
                   end_of_file = FALSE;
                }
                else if(mode != SUB_FILE)
                   open = TRUE;
                break;
           }
           else if(bdos(GET_USER_NO,(long)255) == 0) break;
                else bdos(GET_USER_NO,(long)0);
        }
        if(open)
        {
           check_cmd(glb_index);
           if(!(found))
           {
                p = (void *)&load_tbl;
                while(*(p->typ))
                   if(strcmp(p->typ,&cmdfcb[9]) == MATCH) break;
                   else p++;
           }
           /* load happens after cmd_file in execute_cmd for FILE */
        }
        if(!(sub_open) && mode == SUB_FILE)
           bdos(PRINT_STRING,msg11);
        bdos(GET_USER_NO,(long)user);
        dirflag = TRUE;
        load_try = FALSE;
        morecmds = FALSE;
        return((sub_open || open));
}
                                        /************************/
UWORD sub_read()                        /* Read the submit file */
{                                       /************************/
        if(bdos(READ_SEQ,subfcb))
        {
                end_of_file = TRUE;
                return(FALSE);
        }
        return(TRUE);
}
                                        /************************/
UWORD dollar(k,mode,com_index)          /*    Translate $n to   */
                                        /*  nth argument on the */
                                        /*  command line        */
                                        /************************/
REG UWORD k;
REG UWORD mode;
REG BYTE *com_index;
{
        REG UWORD n,j,p_index;
        REG BYTE *p1;

        j = sub_index;
        if(k >= CMD_LEN)
        {
                k = 0;
                if(!sub_read())
                        return(k);
        }
        if((subdma[k] >= '0') && (subdma[k] <= '9'))
        {
                p_index = (subdma[k] - '0');
                p1 = com_index;
                if(*p1++ == 'S' &&
                   *p1++ == 'U' &&
                   *p1++ == 'B' &&
                   *p1++ == 'M' &&
                   *p1++ == 'I' &&
                   *p1++ == 'T' &&
                   *p1   == ' ')
                        p_index++;
                p1 = com_index;
                for(n = 1; n <= p_index; n++)
                {
                        while(*p1 != ' ' && *p1)
                                p1++;
                        if(*p1 == ' ')
                                p1++;
                }
                while(*p1 != ' ' && *p1 && j < CMD_LEN)
                        if(mode == FILL)
                                subcom[j++] = *p1++;
                        else
                        {       /* NOFILL is the comment echo path */
                                if(!quiet)
                                        bdos(CONSOLE_OUTPUT,(long)*p1);
                                p1++;
                        }
                k++;
        }
        else
        {
                if(mode == FILL)
                        subcom[j++] = '$';
                else
                        bdos(CONSOLE_OUTPUT,(long)'$');
                if(subdma[k] == '$')
                        k++;
        }
        sub_index = j;
        if(k >= CMD_LEN)
        {
                k = 0;
                sub_read();
        }
        return(k);
}
                                        /************************/
UWORD comments(k,com_index)             /* Strip and echo submit*/
                                        /* file comments        */
                                        /************************/
REG UWORD k;
REG BYTE *com_index;
{
        REG UWORD done;

        done = FALSE;

        if(!quiet)
                prompt();
        do
        {
                while(k < CMD_LEN     &&
                     subdma[k] != EOF &&
                     subdma[k] != Cr)
                {
                        if(subdma[k] == '$')
                        {
                                k++;
                                k = dollar(k,NOFILL,com_index);
                        }
                        else
                        {
                                if(!quiet)
                                        bdos(CONSOLE_OUTPUT,(long)subdma[k]);
                                k++;
                        }
                }
                /* k==CMD_LEN: full sector consumed without Cr/EOF; refill.
                 * Do not index subdma[CMD_LEN] (array is [0..CMD_LEN-1]). */
                if(k == CMD_LEN)
                {
                        k = 0;
                        if(!sub_read()) done = TRUE;
                }
                else
                {
                        if(subdma[k] == Cr)
                        {
                                k += 2;
                                if(k >= CMD_LEN)
                                {
                                        k = 0;
                                        sub_read();
                                }
                        }
                        else
                                end_of_file = TRUE;
                        done = TRUE;
                }
        }while(!(done));
        return(k);
}
                                        /************************/
VOID translate(com_index)               /* TRANSLATE the subfile*/
                                        /* and fill sub buffer. */
                                        /************************/
REG BYTE *com_index;
{
        REG UWORD j,k;

        j = 0;
        k = sub_index;
        while(!(end_of_file) && j < CMD_LEN
                && subdma[k] != Cr && subdma[k] != EXLIMPT)
        {
                switch(subdma[k])
                {
                        case ';': k = comments(k,com_index);
                                  break;
                        case TAB:
                        case ' ': if(j > 0)
                                        subcom[j++] = subdma[k++];
        blankout:                 while(k < CMD_LEN &&
                                       (subdma[k] == ' ' || subdma[k] == TAB))
                                                k++;
                                  if(k >= CMD_LEN)
                                  {
                                        k = 0;
                                        if(!sub_read());
                                        else
                                                goto blankout;
                                  }
                                  break;
                        case '$': k++;
                                  sub_index = j;
                                  k = dollar(k,FILL,com_index);
                                  j = sub_index;
                                  break;
                        case Lf:  k++;
                                  if(k >= CMD_LEN)
                                  {
                                        k = 0;
                                        sub_read();
                                  }
                                  break;
                        case EOF:
                                  end_of_file = TRUE;
                                  break;
                         default: subcom[j++] = subdma[k++];
                                  if(k >= CMD_LEN)
                                  {
                                        k = 0;
                                        sub_read();
                                  }
                }
        }
        /*------------------------------------------------------------------*/
        /*             TRANSLATION OF A COMMAND IS COMPLETE                 */
        /*             -Now move sub_index to next command-                 */
        /*------------------------------------------------------------------*/

        if(subdma[k] == Cr || subdma[k] == EXLIMPT)
        do
        {
                while(k < CMD_LEN &&
                     (subdma[k] == Cr ||
                      subdma[k] == Lf ||
                      subdma[k] == EXLIMPT))
                                k++;
                if(k == CMD_LEN)
                {
                        k = 0;
                        if(!sub_read())
                                break;
                }
                else
                {
                        if(subdma[k] == EOF)
                                end_of_file = TRUE;
                        break;
                }
        }while(TRUE);
        sub_index = k;
}
                                        /************************/
UWORD submit_cmd(com_index)             /* fill up the subcom   */
                                        /*      buffer          */
                                        /************************/
/*--------------------------------------------------------------*\
 |                                                              |
 |      Submit_Cmd is a Procedure that returns exactly          |
 |      one command from the submit file.  Submit_Cmd is        |
 |      called only when the end of file marker has not         |
 |      been read yet.  Upon leaving submit_cmd,the variable    |
 |      sub_index points to the beginning of the next command   |
 |      to translate and execute.  The buffer subdma is used    |
 |      to hold the UN-translated submit file contents.         |
 |      The buffer subcom holds a translated command.           |
 |      Comments are echoed to the screen by the procedure      |
 |      "comments".  Parameters are substituted in comments     |
 |      as well as command lines.                               |
 |                                                              |
\*--------------------------------------------------------------*/

REG BYTE *com_index;
{
        REG UWORD i,cur_user;

        for(i = 0;i <= CMD_LEN;i++)
                subcom[i] = NULL;
        cur_user = bdos(GET_USER_NO,(long)255);
        bdos(GET_USER_NO,(long)sub_user);
        bdos(SET_DMA_ADDR,subdma);
        if(first_sub)
        {
                for(i = 0;i < CMD_LEN;i++)
                        subdma[i] = NULL;
                sub_read();
                sub_index = 0;
        }
        if(!(end_of_file))
                translate(com_index);
        for(i = 0;i < CMD_LEN;i++)
                subcom[i] = toupper(subcom[i]);
        bdos(SET_DMA_ADDR,dma);
        bdos(GET_USER_NO,(long)cur_user);
}
                                        /************************/
VOID execute_cmd(cmd)                   /*    branch to         */
                                        /* appropriate routine  */
                                        /************************/

REG BYTE *cmd;
{
        REG UWORD i,flag;

        switch( decode(cmd) )
        {
                case  DIRCMD:   dirflag=TRUE;dir_cmd(0);
                                break;
                case DIRSCMD:   dirflag=TRUE;dir_cmd(1);
                                break;
                case TYPECMD:   type_cmd();
                                break;
                case  RENCMD:   ren_cmd();
                                break;
                case  ERACMD:   era_cmd();
                                break;
                case    UCMD:   if(!(user_cmd()))
                                        bdos(PRINT_STRING,msg12);
                                break;
                case CH_DISK:   bdos(SELECT_DISK,(long)(parm[0][0]-'A'));
                                break;
                case  SUBCMD:   flag = SUB_FILE;
                                if(parm[1][0] == NULL)
                                {
                                        bdos(PRINT_STRING,msg2);
                                        get_cmd(subdma,(long)(CMD_LEN-1));
                                        i = 0;
                                        while(subdma[i] != ' ' &&
                                              subdma[i] &&
                                              i < ARG_LEN-1 ) {
                                                parm[1][i] = subdma[i];
                                                i++;
                                        }
                                        parm[1][i] = NULL;
                                        if(i != 0) subprompt = TRUE;
                                        else break;
                                }
                                else
                                        subprompt = FALSE;
                                cmd_file(flag);
                                break;

                /*
                 * QUIET - suppress the CCP's echo of submit file commands and
                 * comments the way way that ZCPR3 QUIET or DOS ECHO OFF works
                 */
                case QUIETCMD:
                        {
                                REG BYTE *a;

                                a = &parm[1][0];
                                if(strcmp(a,"ON") == MATCH   ||
                                   strcmp(a,"TRUE") == MATCH ||
                                   strcmp(a,"1") == MATCH)
                                {
                                        if(submit)
                                                quiet = TRUE;
                                }
                                else if(strcmp(a,"OFF") == MATCH   ||
                                        strcmp(a,"FALSE") == MATCH ||
                                        strcmp(a,"0") == MATCH)
                                {
                                        if(submit)
                                                quiet = FALSE;
                                }
                                else
                                        echo_cmd(a,BAD);
                        }
                        break;

                /*
                 * GO - ZCPR-style one-step recall: re-enter the last .386 still
                 * resident in the TPA without re-reading the disk.  Optional
                 * args after GO become the new default FCBs / command tail
                 * (e.g.  GO README.TXT).  Fails with "No program" if nothing
                 * has been loaded successfully yet, or if a failed load
                 * invalidated the previous image.
                 */
                case GOCMD:
                        {
                                BYTE argfcb1[FCB_LEN];
                                BYTE argfcb2[FCB_LEN];
                                extern void bios_setup_basepage(const void *,
                                                               const void *,
                                                               const char *);
                                extern UWORD pgm_go(void);
                                dirflag = FALSE;
                                fill_fcb(1, argfcb1);
                                fill_fcb(2, argfcb2);
                                bios_setup_basepage(argfcb1, argfcb2,
                                                    (const char *)tail);
                                bdos(P_CODE, 0);
                                {
                                        UWORD grc = pgm_go();
                                        bdos(CLEAN_DISK, 0);
                                        if (grc == (UWORD)0xFFFF) {
                                                bdos(PRINT_STRING, gonomsg);
                                                cr_lf();
                                        } else if (grc != 0) {
                                                load_error(grc);
                                        } else {
                                                UWORD prc = bdos(P_CODE,
                                                                 (LONG)0xFFFF);
                                                if (prc != 0) {
                                                        static BYTE rcmsg[] =
                                                            "Return code $";
                                                        BYTE digs[6];
                                                        WORD ni = 0;
                                                        UWORD n = prc;
                                                        bdos(PRINT_STRING,
                                                             rcmsg);
                                                        while (n && ni < 6) {
                                                                digs[ni++] =
                                                                    (BYTE)('0' +
                                                                           (n % 10));
                                                                n /= 10;
                                                        }
                                                        while (ni > 0)
                                                                bdos(CONSOLE_OUTPUT,
                                                                     (LONG)digs[--ni]);
                                                        cr_lf();
                                                }
                                        }
                                }
                        }
                        break;

                case    FILE:
                        /*
                         * Resolve command name:
                         *   name.386 / bare name with .386 present -> load program
                         *   name.SUB (explicit)                    -> SUBMIT script
                         *   bare name, only .SUB on disk           -> SUBMIT script
                         *   other extensions                       -> error
                         * Bare name prefers .386 when both exist (unambiguous SUB
                         * only when no .386).
                         */
                        {
                                BYTE blank_type, is_386, is_sub;
                                BYTE try_sub;
                                UWORD sav_user, sav_disk, n, oi;
                                BYTE opened_sub;

                                fill_fcb(0, cmdfcb);
                                blank_type = (BYTE)(cmdfcb[9] == ' ' &&
                                                    cmdfcb[10] == ' ' &&
                                                    cmdfcb[11] == ' ');
                                is_386 = (BYTE)(cmdfcb[9] == '3' &&
                                                cmdfcb[10] == '8' &&
                                                cmdfcb[11] == '6');
                                is_sub = (BYTE)(cmdfcb[9] == 'S' &&
                                                cmdfcb[10] == 'U' &&
                                                cmdfcb[11] == 'B');

                                if (!blank_type && !is_386 && !is_sub) {
                                        echo_cmd(parm, BAD);
                                        break;
                                }

                                try_sub = is_sub; /* explicit .SUB */

                                /* --- prefer .386 for bare name or explicit .386 --- */
                                if (blank_type || is_386) {
                                        if (blank_type) {
                                                cmdfcb[9] = '3';
                                                cmdfcb[10] = '8';
                                                cmdfcb[11] = '6';
                                        }
                                        sav_disk = bdos(RET_CUR_DISK, (LONG)0);
                                        bdos(SELECT_DISK, (LONG)(cmdfcb[0] - 1));
                                        if (bdos(OPEN_FILE, cmdfcb) <= 3) {
                                                BYTE argfcb1[FCB_LEN];
                                                BYTE argfcb2[FCB_LEN];
                                                extern void bios_setup_basepage(
                                                    const void *, const void *,
                                                    const char *);
                                                dirflag = FALSE;
                                                fill_fcb(1, argfcb1);
                                                fill_fcb(2, argfcb2);
                                                bios_setup_basepage(
                                                    argfcb1, argfcb2,
                                                    (const char *)tail);
                                                bdos(P_CODE, 0);
                                                {
                                                        UWORD rc = bdos(
                                                            LOAD_PROGRAM,
                                                            (LONG)cmdfcb);
                                                        bdos(CLEAN_DISK, 0);
                                                        if (rc) {
                                                                load_error(rc);
                                                        } else {
                                                                UWORD prc = bdos(
                                                                    P_CODE,
                                                                    (LONG)0xFFFF);
                                                                if (prc != 0) {
                                                                        static BYTE
                                                                            rcmsg[] =
                                                                                "Return code $";
                                                                        BYTE digs[6];
                                                                        WORD ni = 0;
                                                                        UWORD n2 = prc;
                                                                        bdos(PRINT_STRING,
                                                                             rcmsg);
                                                                        while (n2 &&
                                                                               ni < 6) {
                                                                                digs[ni++] =
                                                                                    (BYTE)('0' +
                                                                                           (n2 % 10));
                                                                                n2 /= 10;
                                                                        }
                                                                        while (ni > 0)
                                                                                bdos(CONSOLE_OUTPUT,
                                                                                     (LONG)digs[--ni]);
                                                                        cr_lf();
                                                                }
                                                        }
                                                }
                                                bdos(SELECT_DISK,
                                                     (LONG)sav_disk);
                                                break;
                                        }
                                        bdos(SELECT_DISK, (LONG)sav_disk);
                                        /* explicit .386 missing */
                                        if (is_386) {
                                                echo_cmd(parm, BAD);
                                                break;
                                        }
                                        /* bare name: fall through to try .SUB */
                                        try_sub = TRUE;
                                }

                                /* --- SUBMIT: explicit .SUB or sole bare-name match --- */
                                if (try_sub) {
                                        fill_fcb(0, cmdfcb);
                                        cmdfcb[9] = 'S';
                                        cmdfcb[10] = 'U';
                                        cmdfcb[11] = 'B';
                                        sav_user = bdos(GET_USER_NO, (LONG)255);
                                        sav_disk = bdos(RET_CUR_DISK, (LONG)0);
                                        opened_sub = FALSE;
                                        bdos(SELECT_DISK,
                                             (LONG)(cmdfcb[0] - 1));
                                        while (1) {
                                                if (bdos(OPEN_FILE, cmdfcb) <=
                                                    3) {
                                                        for (n = 9; n <= 11; n++)
                                                                cmdfcb[n] &= CMASK;
                                                        if (cmdfcb[9] == 'S' &&
                                                            cmdfcb[10] == 'U' &&
                                                            cmdfcb[11] == 'B') {
                                                                sub_user =
                                                                    bdos(GET_USER_NO,
                                                                         (LONG)255);
                                                                if (submit)
                                                                        push_submit();
                                                                first_sub = TRUE;
                                                                for (oi = 0;
                                                                     oi < FCB_LEN;
                                                                     oi++)
                                                                        subfcb[oi] =
                                                                            cmdfcb[oi];
                                                                subprompt = FALSE;
                                                                submit = TRUE;
                                                                quiet = FALSE;
                                                                end_of_file =
                                                                    FALSE;
                                                                opened_sub = TRUE;
                                                        }
                                                        break;
                                                } else if (
                                                    bdos(GET_USER_NO,
                                                         (LONG)255) == 0)
                                                        break;
                                                else
                                                        bdos(GET_USER_NO,
                                                             (LONG)0);
                                        }
                                        bdos(GET_USER_NO, (LONG)sav_user);
                                        bdos(SELECT_DISK, (LONG)sav_disk);
                                        if (!opened_sub)
                                                echo_cmd(parm, BAD);
                                        /* else: main loop drives submit_cmd */
                                        break;
                                }

                                echo_cmd(parm, BAD);
                        }
                        break;
                default     :   echo_cmd(parm,BAD);
        }
}



/*
 * Cold boot only: if A:PROFILE.SUB exists, inject it as the first command
 * via the existing autost/autorom path (same as typing PROFILE.SUB).
 * Missing file is silent — no error message.
 * A static flag ensures warmboot re-entry to ccp() does not re-run it.
 */
static VOID try_cold_profile(void)
{
        static BYTE done;
        BYTE fcb[FCB_LEN];
        UWORD r, sav_user, sav_disk;
        REG UWORD i;
        REG BYTE *p;
        static BYTE cmd[] = "PROFILE.SUB";

        if (done)
                return;
        done = TRUE;

        for (i = 0; i < FCB_LEN; i++)
                fcb[i] = 0;
        fcb[0] = 1; /* drive A: */
        /* name PROFILE, type SUB */
        fcb[1] = 'P'; fcb[2]  = 'R'; fcb[3]  = 'O'; fcb[4] = 'F';
        fcb[5] = 'I'; fcb[6]  = 'L'; fcb[7]  = 'E'; fcb[8] = ' ';
        fcb[9] = 'S'; fcb[10] = 'U'; fcb[11] = 'B';

        sav_user = bdos(GET_USER_NO, (long)255);
        sav_disk = bdos(RET_CUR_DISK, (long)0);
        bdos(SELECT_DISK, (long)0); /* A: */
        bdos(GET_USER_NO, (long)0); /* user 0 */

        r = bdos(OPEN_FILE, (long)(unsigned long)fcb);

        bdos(GET_USER_NO, (long)sav_user);
        bdos(SELECT_DISK, (long)sav_disk);

        if (r > 3)
                return; /* not found — silent */

        /* Seed usercmd and enable one-shot cold autostart */
        p = usercmd;
        for (i = 0; cmd[i]; i++)
                *p++ = cmd[i];
        *p = NULL;
        autost = TRUE;
        autorom = TRUE;
}

#if 0
main()
#endif
ccp()
{                                        /*---------------------*/
        REG BYTE *com_index;             /* cmd execution ptr   */
                                         /*---------------------*/

        try_cold_profile();              /* PROFILE.SUB once on cold boot */

        for (;;) {                       /* loop forever so built-in */
                                         /* commands (DIR etc) return */
                                         /* to A> prompt. (Real     */
                                         /* program loads don't     */
                                         /* return; warmboot re-calls*/
                                         /* ccp for next prompt.)   */
        dirflag = TRUE;                  /* init fcb fill flag  */
        bdos(SET_DMA_ADDR,dma);          /* set system dma addr */
                                         /*---------------------*/
        if(load_try)
        {
                bdos(SELECT_DISK,(long)cur_disk);
                bdos(GET_USER_NO,(long)user);
                load_try = FALSE;
        }
        if(chainp)                       /*sw Chain?            */
        {                                /*sw Yes.              */
          com_index = chainp + 1;        /*sw Set-um pointer    */
          com_index[((WORD)(*chainp))&0xff] = NULL; /*sw Add null*/
          chainp = 0;                    /*sw Clear chain flag  */
        }                                /*sw *******************/
        else                             /*sw Submit or normal  */
        {                                /*---------------------*/
          if(morecmds)                   /* if a warmboot       */
          {                              /* occurred & there were*/
                                         /* more cmds to do     */
                if(submit)               /*---------------------*/
                {
                        com_index = subcom;
                        submit_cmd(save_sub);
                        while(!*com_index)
                        {
                                if(end_of_file)
                                {
                                   com_index = user_ptr;
                                   submit = FALSE;
                                   quiet = FALSE;
                                   break;
                                }
                                else submit_cmd(save_sub);
                        }
                }
                else
                        com_index = user_ptr;
                morecmds = FALSE;
                if(*com_index)
                        echo_cmd(com_index,GOOD);
          }
          else
          {                             /*----------------------*/
               prompt();                /* prompt for command   */
               com_index = usercmd;     /* set execution pointer*/
               if(autost && autorom)    /* check for COLDBOOT cm*/
               {                        /*                      */
                echo_cmd(usercmd,GOOD); /* echo the command     */
                autorom = FALSE;        /* turn off .bss flag   */
               }                        /*                      */
               else                     /* otherwise.......     */
                get_cmd(usercmd,(long)(CMD_LEN));/*read a cmd   */
          }                             /************************/
        }                               /*sw if(chainp)         */
                                        /************************/

/*--------------------------------------------------------------*\
 |                                                              |
 |                     MAIN CCP PARSE LOOP                      |
 |                     ===================                      |
 |                                                              |
\*--------------------------------------------------------------*/


        while(*com_index)
        {                                 /*--------------------*/
                com_index = skip_at(com_index); /* drop '@' so  */
                                          /* the command itself */
                                          /* still executes; the*/
                                          /* echo was already   */
                                          /* suppressed above   */
                glb_index = com_index;    /* save for use in    */
                                          /* check_cmd call     */
                get_parms(com_index);     /* parse command line */
                if(parm[0][0] && parm[0][0] != ';')/*-----------*/
                execute_cmd(parm);        /* execute command    */
                                          /*--------------------*/
                if(!(submit))
                com_index = scan_cmd(com_index);/* inc pointer  */
                else
                {
                        com_index = subcom;
                        if(first_sub)
                        {
                                if(subprompt)
                                        copy_cmd(subdma);
                                else
                                        copy_cmd(glb_index);
                                user_ptr = scan_cmd(save_sub);
                                submit_cmd(save_sub);
                                first_sub = FALSE;
                        }
                        else
                                *com_index = NULL;
                        while(*com_index == NULL)
                        {
                                if(end_of_file)
                                {
                                        if (!pop_submit())
                                        {
                                                com_index = user_ptr;
                                                submit = FALSE;
                                                quiet = FALSE;
                                                break;
                                        }
                                        com_index = user_ptr;
                                        if (*com_index == NULL) {
                                                com_index = subcom;
                                                *com_index = NULL;
                                        }
                                }
                                else
                                        submit_cmd(save_sub);
                        }
                }
                if(*com_index)
                        echo_cmd(com_index,GOOD);
        }
        /* end of one command batch; loop to re-prompt for next interactive cmd */
        }
}

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
