/*
        这个程式是由 Firebird BBS 所专用的 Editor ，欢迎各位使用
        如果有任何问题请 Mail 给 SmallPig.bbs@bbs.cs.ccu.edu.tw
        或是到 140.123.101.78(bbs.cs.ccu.edu.tw) Post 问题。
*/

#include "bbs.h"
#include "edit.h"

struct textline *firstline = NULL ;
struct textline *lastline = NULL ;

extern int temp_numposts;/*Haohmaru.99.4.02.让爱灌水的人哭去吧//grin*/

void vedit_key();
struct textline *currline = NULL ;
int first_mark_line;
int currpnt = 0 ;
extern int local_article;
#ifndef VEDITOR
extern char BoardName[];
#endif
char searchtext[80];
int        editansi = 0;
int        scrollen=2;
int        marknum;
int        moveln=0;
int        shifttmp=0;
int        ismsgline;
int        tmpline;
struct textline *top_of_win = NULL ;
int curr_window_line ,currln;
int redraw_everything ;
int insert_character = 1;
#ifdef VEDITOR
char bkfname[STRLEN];
char currfname[STRLEN];
int  bkfile=0;
#endif
/* for copy/paste */
#define CLEAR_MARK()  mark_on = 0; mark_begin = mark_end = NULL;
struct textline *mark_begin, *mark_end;
int    mark_on;
/* copy/paste */

void
msgline()
{
    char buf[256],buf2[STRLEN*2];
    void display_buffer();
#ifndef VEDITOR
    extern int talkrequest;
#else
    char ptr[6];
#endif
    int tmpshow;
    time_t now;
    if(ismsgline<=0)
        return;
    now=time(0);
    tmpshow=showansi;
    showansi=1;
#ifndef VEDITOR
    if(talkrequest)
    {
        talkreply();
        clear();
        showansi=0;
        display_buffer();
        showansi=1;
    }
    strcpy(buf,"[33m[44m");
    if(chkmail())
        strcat(buf,"【[32m信[33m】");
    else
        strcat(buf,"【  】");

    /* Leeward 98.07.30 Change hot key for msgX */
    /*strcat(buf," [31mCtrl-Z[33m 求救         ");*/
    strcat(buf," [31mCtrl-Q[33m 求救         ");
    sprintf(buf2," 状态 [%s][%4.4d,%3.3d]      时间",insert_character ? "插入":"替换", currln+1, currpnt+1);
    strcat(buf,buf2);
    sprintf(buf2,"[33m[44m【%.16s】[m",ctime(&now));
#else
    /*此软体虽为 Freeware ，但请不要修改以下的字样，谢谢*/
    strcpy(buf,"[36m[44m中正资工 BBS 设计 [31mCtrl-Z[33m 求救");
    sprintf(buf2," [%s][%6d,%3d]",insert_character ? "Ins":"Rep", currln+1, currpnt+1);
    strcat(buf,buf2);
    sprintf(ptr,"%.5s",ctime(&now)+11);
    sprintf(buf2,"[33m[44m %22.22s [m%.2s:%.2s][m",currfname,ptr,ptr+3);
#endif
    strcat(buf,buf2);
    move(t_lines-1,0);
    clrtoeol();
    prints("%s",buf);
    showansi=tmpshow;
}

void domsg()
{
    int x,y;
    time_t now=time(0);
    int tmpansi;
    tmpansi=showansi;
    showansi=1;
    getyx(&x,&y);
    msgline();

    move(x,y);
    refresh();
    showansi=tmpansi;
    return;
}

void
indigestion(i)
int     i;
{
    /* change by KCN 1999.09.08
        fprintf(stderr,"SERIOUS INTERNAL INDIGESTION CLASS %d\n",i) ;
    */
    prints("SERIOUS INTERNAL INDIGESTION CLASS %d\n",i) ;
    oflush();
}

struct textline *
            back_line(pos,num)
            struct textline *pos ;
int     num;
{
    moveln=0;
    while(num-- > 0)
        if(pos && pos->prev){
            pos = pos->prev ;
            moveln++;}

    return pos ;
}

struct textline *
            forward_line(pos,num)
            struct textline *pos ;
int     num;
{
    moveln=0;
    while(num-- > 0)
        if(pos && pos->next)
        {
            pos = pos->next ;
            moveln++;
        }
    return pos ;
}

void
countline()
{
    struct textline *pos;
    pos=firstline;
    moveln=0;
    while(pos!=lastline)
    {
        pos = pos->next ;
        moveln++;
    }
}

int
getlineno()
{
    int cnt = 0 ;
    struct textline *p = currline ;

    while(p != top_of_win) {
        if(p == NULL)
            break ;
        cnt++ ;
        p = p->prev ;
    }
    return cnt ;
}
char *
killsp(s)
char *s ;
{
    while(*s == ' ')
        s++ ;
    return s ;
}

struct textline *
            alloc_line()
{
    register struct textline *p ;

    p = (struct textline *) malloc(sizeof(*p)) ;
    if(p == NULL) {
        indigestion(13) ;
        abort_bbs() ;
    }
    p->next = NULL ;
    p->prev = NULL ;
    p->data[0] = '\0' ;
    p->len = 0 ;
    p->attr = 0 ; /* for copy/paste */
    return p ;
}

/*
  Appends p after line in list.  keeps up with last line as well.
 */

void
goline(n)
int n;
{
    register struct textline *p=firstline;
    int count;

    if(n<0)
        n=1;
    if(n==0)
        return;
    for(count=1;count<n;count++)
    {
        if(p)
        {
            p=p->next;
            continue;
        }
        else
            break;
    }
    if(p)
    {
        currln=n-1;
        curr_window_line=0;
        top_of_win=p;
        currline=p;
    }else
    {
        top_of_win=lastline;
        currln=count-2;
        curr_window_line=0;
        currline=lastline;
    }
    if(Origin(currline))
    {
        currline=currline->prev;
        top_of_win=currline;
        curr_window_line=0;
        currln--;
    }
    if(Origin(currline->prev)){
        currline=currline->prev->prev;
        top_of_win=currline;
        curr_window_line=0;
        currln-=2;
    }


}

void
go()
{
    char tmp[8];
    int line;
    signal(SIGALRM, SIG_IGN);
    getdata( 23,0,"请问要跳到第几行: ",tmp, 7, DOECHO, NULL );
    domsg();
    if(tmp[0]=='\0')
        return;
    line=atoi(tmp);
    goline(line);
    return;
}


void
searchline(text)
char text[STRLEN];
{
    int tmpline;
    int addr;
    int tt;

    register struct textline *p=currline;
    int count=0;
    tmpline=currln;
    for(;;p=p->next)
    {
        count++;
        if(p)
        {
            if(count==1)
                tt=currpnt;
            else
                tt=0;
            if(strstr(p->data+tt,text))
            {
                addr=(int)(strstr(p->data+tt,text) - p->data)+strlen(text);
                currpnt=addr;
                break;
            }
        }
        else
            break;
    }
    if(p)
    {
        currln=currln+count-1;
        curr_window_line=0;
        top_of_win=p;
        currline=p;
    }else
    {
        goline(currln+1);
    }
    if(Origin(currline))
    {
        currline=currline->prev;
        top_of_win=currline;
        curr_window_line=0;
        currln--;
    }
    if(Origin(currline->prev)){
        currline=currline->prev->prev;
        top_of_win=currline;
        curr_window_line=0;
        currln-=2;
    }

}

void
search()
{
    char tmp[STRLEN];

    signal(SIGALRM, SIG_IGN);
    getdata( 23,0,"搜寻字串: ",tmp, 65, DOECHO, NULL );
    domsg();
    if(tmp[0]=='\0')
        return;
    else
        strcpy(searchtext,tmp);

    searchline(searchtext);
    return;
}


void
append(p,line)
register struct textline *p, *line ;
{
    p->next = line->next ;
    if(line->next)
        line->next->prev = p ;
    else
        lastline = p ;
    line->next = p ;
    p->prev = line ;
}

/*
  delete_line deletes 'line' from the list and maintains the lastline, and 
  firstline pointers.
 */

void
delete_line(line)
register struct textline *line ;
{
    /* if single line */
    if(!line->next && !line->prev) {
        line->data[0] = '\0' ;
        line->len = 0 ;
        CLEAR_MARK();
        return ;
    }

#define ADJUST_MARK(p, q) if(p == q) p = (q->next) ? q->next : q->prev

    ADJUST_MARK( mark_begin, line );
    ADJUST_MARK( mark_end, line );

    if(line->next)
        line->next->prev = line->prev ;
    else
        lastline = line->prev ;  /* if on last line */

    if(line->prev)
        line->prev->next = line->next ;
    else
        firstline = line->next ; /* if on first line */

    free(line) ;
}

/*
  split splits 'line' right before the character pos
 */

void
split(line,pos)
register struct textline *line ;
register int pos ;
{
    register struct textline *p = alloc_line() ;

    if(pos > line->len) {
        free(p) ;
        return ;
    }

    p->len = line->len - pos ;
    line->len = pos ;
    strcpy(p->data,(line->data + pos)) ;
    p->attr = line->attr;              /* for copy/paste */
    *(line->data + pos) = '\0' ;
    append(p,line) ;
    if(line == currline && pos <= currpnt) {
        currline = p ;
        currpnt -= pos ;
        curr_window_line++ ;
        currln++;
    }
    redraw_everything = YEA ;
}

/*
  join connects 'line' and the next line.  It returns true if:
  
  1) lines were joined and one was deleted
  2) lines could not be joined
  3) next line is empty

  returns false if:

  1) Some of the joined line wrapped
 */

int
join(line)
register struct textline *line ;
{
    register int ovfl ;
    if(!line->next)
        return YEA ;
    /*if(*killsp(line->next->data) == '\0')
      return YEA ;*/
    ovfl = line->len + line->next->len - WRAPMARGIN ;
    if(ovfl < 0) {
        strcat(line->data, line->next->data) ;
        line->len += line->next->len ;
        delete_line(line->next) ;
        return YEA ;
    } else {
        register char *s ;
        register struct textline *p = line->next ;

        s = p->data + p->len - ovfl -1 ;
        while(s!=p->data && *s == ' ')
            s-- ;
        while(s!=p->data && *s != ' ')
            s-- ;
        if(s == p->data)
            return YEA ;
        split(p,(s - p->data) + 1) ;
        if(line->len + p->len >= WRAPMARGIN) {
            indigestion(0) ;
            return YEA ;
        }
        join(line) ;
        p = line->next ;
        if(p->len >= 1 && p->len+1 < WRAPMARGIN) {
            if(p->data[p->len-1] != ' ') {
                strcat(p->data," ") ;
                p->len++ ;
            }
        }
        return NA ;
    }
}

void
insert_char(ch)
register int ch ;
{
    register int i ;
    register char *s ;
    register struct textline *p = currline ;
    int wordwrap = YEA ;

    if(currpnt > p->len) {
        indigestion(1) ;
        return ;
    }
    if( currpnt < p->len && !insert_character ) {
        p->data[currpnt++] = ch ;
    } else {
        for(i = p->len; i >= currpnt; i--)
            p->data[i+1] = p->data[i] ;
        p->data[currpnt] = ch ;
        p->len++ ;
        currpnt++ ;
    }
    if(p->len < WRAPMARGIN)
        return ;
    s = p->data + (p->len - 1) ;
    while(s!=p->data && *s == ' ')
        s-- ;
    while(s!=p->data && *s != ' ')
        s-- ;
    if(s==p->data) {
        wordwrap = NA ;
        s = p->data + (p->len -2) ;
    }

    { /* Leeward 98.07.28 */
        int ich, lln;

        if (*s > 127) { /* 避免在汉字中间折行 */
            for (ich = 0, lln = s - p->data + 1; lln > 0; lln --)
                    if (p->data[lln - 1] < 128) break; else ich ++;
            if (ich % 2) s --;
        }
    }

    split(p,(s - p->data) + 1) ;
    p = p->next ;
    if(wordwrap && p->len >= 1) {
        i = p->len ;
        if(p->data[i-1] != ' ') {
            p->data[i] = ' ' ;
            p->data[i+1] = '\0' ;
            p->len++ ;
        }{
        }
    }
    /*while(!join(p)) { Leeward 98.07.29 避免折行后覆盖下一行，乱码
    p = p->next ;
        if(p == NULL) {
            indigestion(2) ;
            break ;
        }
}*/
    if(Origin(currline)){
        currline=p->prev;
        curr_window_line--;
        currln--;
    }
}

void
ve_insert_str(str)
char    *str;
{
    while(*str) insert_char(*(str++));
}

void
delete_char()
{
    register int i ;

    if(currline->len == 0)
        return ;
    if(currpnt >= currline->len) {
        indigestion(1) ;
        return ;
    }
    for(i=currpnt;i!=currline->len;i++)
        currline->data[i] = currline->data[i+1] ;
    currline->len-- ;
}

void
vedit_init()
{
    register struct textline *p = alloc_line() ;
    first_mark_line=0;
    firstline = p ;
    lastline = p ;
    currline = p ;
    currpnt = 0 ;
    marknum=0;
    process_ESC_action('M', '0');
    top_of_win = p ;
    curr_window_line = 0 ;
    currln=0;
    redraw_everything = NA ;
    CLEAR_MARK();
}

void
insert_to_fp(fp)
FILE *fp;
{
    int ansi = 0;
    struct textline *p;

    for ( p = firstline; p; p = p->next )
        if ( p->data[0] ) {
            fprintf(fp,"%s\n",p->data) ;
            if (strchr(p->data, '\033')) ansi++;
        }
    if (ansi) fprintf( fp, "%s\n", ANSI_RESET );
}

void
insert_from_fp(fp)
FILE *fp;
{
    int ch;
#ifdef VEDITOR    
    bkfile=1;
#endif
    while((ch = getc(fp)) != EOF)
        if(isprint2(ch) || ch == 27) {
            if ( currpnt < 254)  insert_char(ch) ;
            else if (currpnt < 255) insert_char('.');
        } else if(ch == Ctrl('I')) {
            do { insert_char(' ');
            } while (currpnt & 0x7);
        } else if(ch == '\n')
            split(currline,currpnt) ;
}

void
read_file(filename)
char *filename ;
{
    FILE *fp ;

    if(currline == NULL)
        vedit_init() ;
    if((fp = fopen(filename,"r+")) == NULL) {
        if((fp = fopen(filename,"w+")) != NULL) {
            fclose(fp) ;
            return ;
        }
        indigestion(4) ;
        abort_bbs() ;
    }
    insert_from_fp( fp );
    fclose(fp);
}

char save_title[STRLEN] ;
char save_filename[4096] ;
int in_mail ;

#ifndef VEDITOR
int
write_posts()
{
    char *ptr;
    time_t now;
    struct
    {
        char author[IDLEN + 1];
        char board[IDLEN + 1];
        char title[66];
        time_t date;
        int number;
    } postlog, pl;

    if(junkboard()||normal_board(currboard)!=1||!strcmp(currboard,"blame"))
        return ;
    now = time(0) ;
    strcpy(postlog.author, currentuser->userid);
    strcpy(postlog.board, currboard);
    ptr = save_title;
    if (!strncmp(ptr, "Re: ", 4))
        ptr += 4;
    strncpy(postlog.title, ptr, 65);
    postlog.date = now;
    postlog.number = 1;

    { /* added by Leeward 98.04.25 */
        char buf[STRLEN];
        int  log = 1;
        FILE *fp = fopen(".Xpost", "r");

        if (fp)
        {
            while (!feof(fp))
            {
                fread(&pl, sizeof(pl), 1, fp);
                if (feof(fp)) break;

                if (!strcmp(pl.title, postlog.title)
                        && !strcmp(pl.author, postlog.author)
                        && !strcmp(pl.board, postlog.board))
                {
                    log = 0;
                    break;
                }
            }
            fclose(fp);
        }

        if (log)
        {
            append_record(".Xpost", &postlog, sizeof(postlog));
            append_record(".post", &postlog, sizeof(postlog));
        }
    }

    append_record(".post.X", &postlog, sizeof(postlog));
}
#endif


#ifndef VEDITOR
void
write_header(fp,mode)
FILE *fp ;
int mode;
{
    int  noname;
    extern char BoardName[];
    extern int Anony;
    char uid[20] ;
    char uname[40] ;
    time_t now;

    now = time(0) ;
    strncpy(uid,currentuser->userid,20) ;
    uid[19] = '\0' ;
    if (in_mail)
#if defined(MAIL_REALNAMES)
        strncpy(uname,currentuser->realname,NAMELEN) ;
#else
strncpy(uname,currentuser->username,NAMELEN) ;
#endif
    else
#if defined(POSTS_REALNAMES)
        strncpy(uname,currentuser->realname,NAMELEN) ;
#else
        strncpy(uname,currentuser->username,NAMELEN) ;
#endif
    /* uid[39] = '\0' ; SO FUNNY:-) 定义的 20 这里却用 39 !
                        Leeward: 1997.12.11 */
    uname[39] = 0; /* 其实是写错变量名了! 嘿嘿 */
    save_title[STRLEN-10] = '\0' ;
    noname=seek_in_file("etc/anonymous",currboard);
    if(in_mail)
        fprintf(fp,"寄信人: %s (%s)\n",uid,uname) ;
    else
    {
        if(mode==0&&!(noname&&Anony))
        {
            write_posts();
        }

        if (!strcmp(currboard,"Announce"))
            /* added By Bigman */
            fprintf(fp,"发信人: %s (%s), 信区: %s       \n","SYSOP",
                    "System Operator",currboard) ;
        else
            fprintf(fp,"发信人: %s (%s), 信区: %s       \n",(noname&&Anony)?"Anonymous":uid,
                    (noname&&Anony)?"我是匿名天使":uname,currboard) ;
    } /***** 该语句增加阅读次数计数项 Add by Luzi 1998/1/8 ****/

    fprintf(fp,"标  题: %s\n",save_title) ;
    fprintf(fp,"发信站: %s (%24.24s)\n",BoardName,ctime(&now)) ;
    if(in_mail)
        fprintf(fp,"来  源: %s \n",currentuser->lasthost) ;
    fprintf(fp,"\n");

}
#endif

#ifndef VEDITOR
void
addsignature(fp,blank)
FILE *fp;
int blank;
{
    FILE *sigfile;
    int  i,valid_ln=0;
    char tmpsig[MAXSIGLINES][256];
    char inbuf[256];
    char fname[STRLEN];
    char tmp[STRLEN];

    setuserfile( fname, "signatures" );
    if ((sigfile = fopen(fname, "r"))== NULL)
    {return;}
    if ( blank ) fputs("\n", fp);
    fputs("--\n", fp);
    for (i=1; i<=(currentuser->signature-1)*MAXSIGLINES&currentuser->signature!=1; i++)
    {
        if (!fgets(inbuf, sizeof(inbuf), sigfile)){
            fclose(sigfile);
            return;}
    }
    for (i=1; i<=MAXSIGLINES; i++) {
        if (fgets(inbuf, sizeof(inbuf), sigfile))
        {
            if(inbuf[0]!='\n')
                valid_ln=i;
            strcpy(tmpsig[i-1],inbuf);
        }
        else break;
    }
    fclose(sigfile);
    for(i=1;i<=valid_ln;i++)
        fputs(tmpsig[i-1], fp);
    /*fclose(sigfile); Leeward 98.03.29: Extra fclose is a BUG! */
}
#endif

#define KEEP_EDITING -2

int
valid_article( pmt, abort )
char    *pmt, *abort;
{
    struct textline *p = firstline ;
    char        ans[3], ch;
    int         total, lines, len, sig, y;
    int		temp;
#ifndef VEDITOR
    if(uinfo.mode == POSTING)
    {
        total = lines = len = sig = 0;
        while( p != NULL ) {
            if( !sig ) {
                ch = p->data[0];
                if( strcmp( p->data, "--" ) == 0 )
                    sig = 1;
                else if(( ch != ':' && ch != '>' && ch != '=' )&&(strlen(p->data)>2)) {
                    lines++;
                    len += strlen( p->data );
                }
            }
            total++;
            p = p->next;
        }
        y = 2;
        if( total > 20 + lines * 3 ) {
            move( y, 0 );
            prints( "本篇文章的引言与签名档行数远超过本文长度.\n" );
            y += 3;
        }
        temp = 0;
        if( len < 8 || lines < 2 ) {
            move( y, 0 );
            prints( "本篇文章非常简短, 系统认为灌水文章.\n" );
            /*Haohmaru.99.4.02.让爱灌水的人哭去吧//grin*/
            y += 3;
            temp = 1;
        }
    }
#endif

    getdata( 0, 0, pmt, abort, 3, DOECHO, NULL ,YEA);
    switch(abort[0]){
case 'A': case 'a':     /* abort */
case 'E': case 'e':     /* keep editing */
        return temp;}
    return temp;

}

int
write_file(filename,saveheader)
char *filename ;
int saveheader ;
{
    struct textline *p = firstline ;
    FILE        *fp ;
    char        abort[6];
    int         aborted = 0;
    char buf[256];
    int temp;

#ifndef VEDITOR
    char p_buf[100];
    signal(SIGALRM, SIG_IGN);
    clear() ;

    if(uinfo.mode!=CCUGOPHER)
    {
        /*
            if ( uinfo.mode == EDITUFILE||uinfo.mode==CSIE_ANNOUNCE||
                uinfo.mode == EDITSFILE)
                strcpy(p_buf,"(S)储存档案, (A)放弃编辑, (E)继续编辑? [S]: " );
            else if ( uinfo.mode == SMAIL )
                strcpy(p_buf,"(S)寄出, (A)取消, or (E)再编辑? [S]: " );
            else if ( local_article == 1 )
                strcpy(p_buf,"(L)不转信, (S)转信, (A)取消, (T)更改标题 or (E)再编辑? [L]: ");
            else
                strcpy(p_buf,"(S)转信, (L)不转信, (A)取消, (T)更改标题 or (E)再编辑? [S]: ");
        */
        if(uinfo.mode == POSTING)
        {
            /*if ( local_article == 1 )
                    strcpy(p_buf,"(L)不转信, (S)转信, (A)取消, (T)更改标题 or (E)再编辑? [L]: ");
            else
                    strcpy(p_buf,"(S)转信, (L)不转信, (A)取消, (T)更改标题 or (E)再编辑? [S]: ");*/
            strcpy(p_buf,"(S)发表, (F)自动换行发表, (A)取消, (T)更改标题 or (E)再编辑? [S]: ");
            move(4, 0); /* Haohmaru 99.07.17 */
            prints("请注意：本站站规规定：同样内容的文章严禁在 5 (含)个以上讨论区内重复张贴。\n\n违反者除所贴文章会被删除之外，还将被剥夺继续发表文章的权力。详细规定请参照：\n\n    Announce 版的站规：“关于转贴和张贴文章的规定”。\n\n请大家共同维护 BBS 的环境，节省系统资源。谢谢合作。\n\n");
        }
        else if ( uinfo.mode == SMAIL )
            strcpy(p_buf,"(S)寄出, (A)取消, or (E)再编辑? [S]: " );
        /*    sprintf(p_buf,"给 %s 的信：(S)寄出, (F)自动换行寄出, (A)取消, or (E)再编辑? [S]: ", lookupuser->userid ); 
         Leeward 98.01.17 Prompt whom you are writing to */
        else
            strcpy(p_buf,"(S)储存档案, (F)自动换行存储, (A)放弃编辑, (E)继续编辑? [S]: " );
        temp = valid_article( p_buf, abort );
        if(abort[0]!='T' && abort[0]!='t' && abort[0]!='F' && abort[0]!='f' && abort[0]!='A' && abort[0]!='a' &&abort[0]!='E' &&abort[0]!='e')
            abort[0]='s';
    }else
        abort[0] = 'a';
#else
    valid_article( "(S)储存, (A)取消, or (E)再编辑? [S]: ", abort );
    if(abort[0]!='A' && abort[0]!='a' &&abort[0]!='E' &&abort[0]!='e')
        abort[0]='s';
#endif

    if (abort[0] == 'a' || abort[0] == 'A') {
        struct stat stbuf;
        clear();
#ifndef VEDITOR
        if(uinfo.mode!=CCUGOPHER)
        {
#endif
            prints("取消...\n");
            refresh();
            sleep(1);
#ifndef VEDITOR
        }
#endif
        if (stat(filename, &stbuf) || stbuf.st_size == 0)
            unlink(filename);
        aborted = -1;
    } else if (abort[0] == 'e' || abort[0] == 'E') {
        domsg();
        return KEEP_EDITING;
    } else if (abort[0] == 't' || abort[0] == 'T' ) {
        char    buf[ STRLEN ];

        buf[0] = 0; /* Leeward 98.08.14 */
        move(1,0);
        prints("旧标题: %s", save_title );
        getdata(2,0,"新标题: ",buf,STRLEN,DOECHO,NULL);
        if ( strcmp( save_title, buf ) )  local_article = 0;
        strncpy(save_title,buf,STRLEN) ;
    } else if (abort[0] == 's' || abort[0] == 'S'
               || abort[0] == 'f' || abort[0] == 'F') {
        local_article = 0;
#ifdef VEDITOR
        if(bkfile==1)
        {
            sprintf(buf,"cp %s %s",filename,bkfname);
            system(buf);
        }
#endif
    }
    firstline = NULL ;
    if (!aborted) {
        if((fp = fopen(filename,"w")) == NULL) {
            indigestion(5) ;
            abort_bbs() ;
        }
#ifndef VEDITOR
        if(saveheader)
            write_header(fp,0) ;
#endif
    }
    while(p != NULL) {
        struct textline *v = p->next ;
        if (!aborted)
            if(p->next != NULL || p->data[0] != '\0')
                if (abort[0] == 'f' || abort[0] == 'F')
                { /* Leeward 98.07.27 支持自动换行 */
                    char *ppt = p->data; /* 折行处 */
                    char *pp  = ppt; /* 行首 */
                    int  LLL = 78; /* 折行位置 */
                    char *ppx, cc;
                    int  ich, lll;

                    while (strlen(pp) > LLL) {
                        lll = 0; ppx = pp; ich = 0;
                        do {
                            if (ppx = strstr(ppx, "[")) {
                                ich = strchr(ppx, 'm') - ppx;
                                if (ich > 0) ich ++; else ich = 2;
                                lll += ich; ppx += 2; ich = 0;
                            }
                        } while (ppx);
                        ppt += LLL + lll;

                        if (*ppt > 127) { /* 避免在汉字中间折行 */
                            for (ppx = ppt - 1, ich = 0; ppx >= pp; ppx --)
                                    if (*ppx < 128) break; else ich ++;
                            if (ich % 2) ppt --;
                        }
                        else if (*ppt) {
                            for (ppx = ppt - 1, ich = 0; ppx >= pp; ppx --)
                                    if (*ppx > 127 || ' ' == *ppx) break; else ich ++;
                            if (ppx > pp && ich < 16) ppt -= ich;
                        }

                        cc = *ppt; *ppt = 0;
                        if (':' == p->data[0] && ':' != *pp) fprintf(fp, ": ");
                        fprintf(fp,"%s",pp) ;
                        if (cc) fprintf(fp, "\n");
                        *ppt = cc; pp = ppt;
                    }
                    if (':' == p->data[0] && ':' != *pp) fprintf(fp, ": ");
                    fprintf(fp,"%s\n",pp) ;
                }
                else
                    fprintf(fp,"%s\n",p->data) ;

        free(p) ;
        p = v ;
    }
    if (!aborted) fclose(fp) ;
    currline = NULL ;
    lastline = NULL ;
    firstline = NULL ;
#ifndef VEDITOR
    if ( abort[0] == 'l' || abort[0] == 'L' || local_article == 1 ) {
        sprintf( genbuf, "local_article = %u", local_article );
        report( genbuf );
        local_article = 0;
        if( aborted != -1 )  aborted = 1;
    }
#endif
    if((uinfo.mode == POSTING)&&strcmp(currboard,"test"))/*Haohmaru.99.4.02.让爱灌水的人哭去吧//grin*/ {
        if (temp)
            temp_numposts++;
        if (temp_numposts>20)
            sleep((temp_numposts-20)*1+1);
    }
    return aborted;
}

keep_fail_post()
{
    char filename[STRLEN];
    char buf[STRLEN];
    char tmpbuf[30];
    time_t now;
    struct textline *p = firstline ;
    FILE        *fp ;

#ifndef VEDITOR
    sethomepath(tmpbuf, currentuser->userid);
    sprintf(filename,"%s/%s.deadve", tmpbuf, currentuser->userid);
#else
    sprintf(filename,"DEADVE");
#endif
#ifndef VEDITOR
    if((fp = fopen(filename,"w")) == NULL) {
#else
    if((fp = fopen(filename,"a")) == NULL) {
#endif
        indigestion(5) ;
        return;
    }
#ifdef VEDITOR
    now=time(0);
    sprintf(buf,"*** Modified files in VE when it aborted on %s",ctime(&now));
    fprintf(fp,"%s",buf);
    sprintf(buf,"*** File Name is %s\n\n",currfname);
    fprintf(fp,"%s",buf);
#endif
    while(p != NULL) {
        struct textline *v = p->next ;
        if(p->next != NULL || p->data[0] != '\0')
            fprintf(fp,"%s\n",p->data) ;
        free(p) ;
        p = v ;
    }
    return ;
}


void    strnput(str)
char    *str;
{
    int count=0;

    while ((*str != '\0') && (++count < STRLEN))
    {
        if (*str == KEY_ESC)
        {
            outc('*');
            str++;
            continue;
        }
        outc(*str++);
    }
}

void
cstrnput(str)
char    *str;
{
    int count=0;

    prints("%s",ANSI_REVERSE);
    while ((*str != '\0') && (++count < STRLEN)){
        if (*str == KEY_ESC){
            outc('*');
            str++;
            continue;
        }
        outc(*str++);
    }
    while(++count < STRLEN)
        outc(' ');
    clrtoeol();
    prints("%s",ANSI_RESET);
}


/*Function Add by SmallPig*/
int
Origin(text)
struct textline *text;
{
#ifndef VEDITOR
    char tmp[STRLEN];

    if(uinfo.mode!=EDIT)
        return 0;
    if(!text)
        return 0;
    sprintf(tmp,"※ 来源:·%s %s·[FROM:",BoardName,email_domain());
    if(strstr(text->data,tmp)&&*text->data!=':')
        return 1;
    else
        return 0;
#else
    return 0;
#endif          
}

#ifndef VEDITOR
int
Origin2(text)
char text[256];
{
    char tmp[STRLEN];

    sprintf(tmp,"※ 来源:·%s %s·[FROM:",BoardName,email_domain());
    if(strstr(text,tmp))
        return 1;
    else
        return 0;
}
#endif
void
display_buffer()
{
    register struct textline *p ;
    register int i ;
    int      shift;
    int      temp_showansi;

    temp_showansi = showansi;

    for(p=top_of_win,i=0;i < t_lines-1;i++) {
        move(i,0) ;
        if(p)
        {
            shift = (currpnt+2 > STRLEN) ?
                    (currpnt / (STRLEN - scrollen)) * (STRLEN - scrollen) : 0;
            if (editansi) {
                showansi = 1;
                prints( "%s", p->data );
            } else if ((p->attr & M_MARK)) {
                showansi = 1; clear_whole_line( i );
                cstrnput(p->data+shift);
            } else {
                if(p->len>= shift){
                    showansi = 0 ; strnput(p->data+shift); }
                else
                    clrtoeol();
            }
            p = p->next ;
        } else prints( "%s~", editansi ? ANSI_RESET : "");
        clrtoeol();
    }

    showansi = temp_showansi;
    msgline();
    return ;
}

int
vedit_process_ESC(arg)
int     arg;   /* ESC + x */
{
    int         ch2, action;

#define WHICH_ACTION_COLOR "(M)区块处理 (I/E)读取/写入剪贴簿 (C)使用彩色 (F/B/R)前景/背景/还原色彩"
#define WHICH_ACTION_MONO  "(M)区块处理 (I/E)读取/写入剪贴簿 (C)使用单色 (F/B/R)前景/背景/还原色彩"

#define CHOOSE_MARK     "(0)取消标记 (1)设定开头 (2)设定结尾 (3)复制标记内容 "
#define FROM_WHICH_PAGE "读取剪贴簿第几页? (0-7) [预设为 0]"
#define SAVE_ALL_TO     "把整篇文章写入剪贴簿第几页? (0-7) [预设为 0]"
#define SAVE_PART_TO    "把整篇文章写入剪贴簿第几页? (0-7) [预设为 0]"
#define FROM_WHICH_SIG  "取出签名簿第几页? (0-7) [预设为 0]"
#define CHOOSE_FG       "前景颜色? 0)黑 1)红 2)绿 3)黄 4)深蓝 5)粉红 6)浅蓝 7)白 "
#define CHOOSE_BG       "背景颜色? 0)黑 1)红 2)绿 3)黄 4)深蓝 5)粉红 6)浅蓝 7)白 "
#define CHOOSE_ERROR    "选项错误"

    switch( arg ) {
case 'M': case 'm':
        ch2 = ask(CHOOSE_MARK);     action = 'M'; break;
case 'I': case 'i': /* import */
        ch2 = ask(FROM_WHICH_PAGE); action = 'I'; break;
case 'E': case 'e': /* export */
        ch2 = ask( mark_on ? SAVE_PART_TO : SAVE_ALL_TO );
        action = 'E'; break;
case 'S': case 's': /* signature */
        ch2 = '0';  action = 'S'; break;
case 'F': case 'f':
        ch2 = ask(CHOOSE_FG);       action = 'F'; break;
case 'B': case 'b':
        ch2 = ask(CHOOSE_BG);       action = 'B'; break;
case 'R': case 'r':
        ch2 = '0'; /* not used */   action = 'R'; break;
case 'D': case 'd':
        ch2='4';                    action = 'M'; break;
case 'N': case 'n':
        ch2='0';                    action = 'N'; break;
case 'G': case 'g':
        ch2='1';                    action = 'G'; break;
case 'L': case 'l':
        ch2='0';/* not used */      action = 'L'; break;
case 'C': case 'c':
        ch2 = '0'; /* not used */   action = 'C'; break;
case 'Q': case 'q': /* Leeward 98.07.30 Change hot key for msgX */
        marknum=0;
        ch2 = '0'; /* not used */   action = 'M'; break;
    default:
        return;
    }

    if ( strchr( "IES", action) &&
            (ch2 == '\n' || ch2 == '\r')) ch2 = '0';

    if (ch2 >= '0' && ch2 <= '7') return process_ESC_action(action, ch2);
    else {
        return ask( CHOOSE_ERROR );
    }
}

int
mark_block()
{
    struct      textline *p;
    int         pass_mark = 0;

    first_mark_line=0;
    if ( mark_begin == NULL && mark_end == NULL ) return 0;
    if ( mark_begin == mark_end )
    {
        mark_begin->attr |= M_MARK;
        return 1;
    }
    if ( mark_begin == NULL || mark_end == NULL )
    {
        if(mark_begin!=NULL)
            mark_begin->attr |= M_MARK;
        else
            mark_end->attr |= M_MARK;
        return 1;
    }
    else {
        for(p = firstline; p != NULL; p = p->next) {
            if ( p == mark_begin || p == mark_end )
            {
                pass_mark++;
                p->attr |= M_MARK;
                continue;
            }
            if( pass_mark ==1 )  p->attr |= M_MARK;
        else {first_mark_line++; p->attr &= ~(M_MARK); }
            if(pass_mark==2)
                first_mark_line--;
        }
        return 1;
    }
}

void
process_MARK_action(arg, msg)
int    arg;    /* operation of MARK */
char   *msg;   /* message to return */
{
    struct textline *p,*p2;
    int i,dele_1line;

    switch( arg ) {
    case '0': /* cancel */
        for(p = firstline; p != NULL; p = p->next) p->attr &= ~(M_MARK);
        CLEAR_MARK();
        break;
    case '1': /* mark begin */
        mark_begin = currline;
        mark_on = mark_block();
        if (mark_on) strcpy( msg, "标记已设定完成");
        else strcpy( msg,"已设定开头标记, 尚无结尾标记");
        break;
    case '2': /* mark end */
        mark_end = currline; mark_on = mark_block();
        if (mark_on) strcpy( msg, "标记已设定完成");
        else strcpy( msg,"已设定结尾标记, 尚无开头标记");
        break;
    case '3': /* copy mark */
        if (mark_on&&!(currline->attr & M_MARK))
        {
            for(p = firstline; p != NULL; p = p->next)
            {
                if (p->attr & M_MARK) {
                    ve_insert_str( p->data );
                    split(currline, currpnt) ;
                }
            }
        }else
            bell();
        strcpy( msg,"标记复制完成");
        break;
    case '4': /* delete mark */
        dele_1line=0;
        if (mark_on&&(currline->attr & M_MARK))
            if(currline==firstline)
                dele_1line=1;
            else
                dele_1line=2;
        for(p = firstline; p != NULL; p = p->next)
        {
            if (p->attr & M_MARK)
            {
                currline=p;
                vedit_key( Ctrl('Y') );
            }
        }
        process_ESC_action('M', '0');
        marknum=0;
        if(dele_1line==0||dele_1line==2){
            if(first_mark_line==0)
                first_mark_line=1;
            goline(first_mark_line);
        }else
            goline(1);
        break;
    default:
        strcpy(msg, CHOOSE_ERROR );
    }
    strcpy(msg,"\0");
}

int
process_ESC_action(action, arg)
int     action, arg;
/* valid action are I/E/S/B/F/R/C */
/* valid arg are    '0' - '7' */
{
    int         newch = 0;
    char        msg[ 80 ], buf[ 80 ];
    char        filename[ 80 ];
    FILE        *fp;

    msg[ 0 ] = '\0';
    switch( action ) {
    case 'L':
        if(ismsgline>=1)
        {
            ismsgline=0;
            move(t_lines-1,0);
            clrtoeol();
            refresh();
        }
        else
            ismsgline=1;
        break;
    case 'M':
        process_MARK_action(arg, msg); break;
    case 'I':
        sprintf( filename, "/tmp/%s_clip_%c", currentuser->userid, arg);
        if( (fp = fopen( filename, "r" )) != NULL ) {
            insert_from_fp( fp ); fclose( fp );
            sprintf( msg, "已取出剪贴簿第 %c 页", arg );
        } else sprintf( msg, "无法取出剪贴簿第 %c 页", arg );
        break;
    case 'G':
        go();
        redraw_everything = YEA;
        break;
    case 'E':
        sprintf( filename, "/tmp/%s_clip_%c", currentuser->userid, arg);
        if( (fp = fopen( filename, "w" )) != NULL ) {
            if ( mark_on ) {
                struct textline *p;
                for(p = firstline; p != NULL; p = p->next)
                    if (p->attr & M_MARK) fprintf(fp, "%s\n", p->data);
            } else insert_to_fp( fp );
            fclose( fp );
            sprintf( msg, "已贴至剪贴簿第 %c 页", arg );
        } else sprintf( msg, "无法贴至剪贴簿第 %c 页", arg );
        break;
    case 'N':
        searchline(searchtext);
        redraw_everything = YEA;
        break;
    case 'S':
        search();
        redraw_everything = YEA;
        break;
    case 'F':
        sprintf( buf, "%c[3%cm", 27, arg ); ve_insert_str( buf ); break;
    case 'B':
        sprintf( buf, "%c[4%cm", 27, arg ); ve_insert_str( buf ); break;
    case 'R':
        ve_insert_str( ANSI_RESET );  break;
    case 'C':
        editansi = showansi = 1; redraw_everything = YEA;
        clear();  display_buffer(); redoscr();
        strcpy( msg, "已显示彩色编辑成果，即将切回单色模式" );
    }

    if (strchr("FBRCM", action)) redraw_everything = YEA;

    if (msg[ 0 ] != '\0') {
        if ( action == 'C' ) {   /* need redraw */
            move( t_lines-2, 0 ) ;
            clrtoeol() ;
            prints( "[1m%s%s%s[m", msg, ", 请按任意键返回编辑画面...", ANSI_RESET );
            igetkey() ;
            newch = '\0';
            editansi = showansi = 0;
            clear(); display_buffer();
        } else newch = ask(strcat( msg, "，请继续编辑。"));
    } else newch = '\0';
    return newch;
}
#ifdef VEDITOR
void
show_helpmenu( helptext )
char *helptext[];
{
    char *str;
    int i;

    showansi=1;
    clear() ;
    for( i = 0; (str = helptext[i]) != NULL; i++ ) {
        if( *str == '\01' || *str == '\02' ) {
            prints("[44m");
            /*          if( *str == '\02' )
                            printdash( str + 1 );
                        else   
            */              prints( "%s\n", str + 1 );
            prints("[m");
        } else {
            prints( "%s\n", str );
        }
    }
    move( t_lines-1, 0 ) ;
    prints("Press any key to continue...");
    igetkey();
    clear() ;
}

char *vedithelp[] = {
    "\01编辑功能指令说明",
    "",
    "\01一般命令:",
    " ^W,^X    存档後结束编辑         ^L,^G    重绘屏幕",
    " ^Q       显示辅助画面           ^Z       显示辅助画面",
    "",
    "\01游标移动指令:",
    " ^P,↑   向上    ｜ ^N,↓   向下    | ^R,←    向左   | ^V,→   向右  ",
    " ^A,Home 至行首  ｜ ^E, End 至行末  | ^B,PgUp  上一页 | ^F,PgDn 下一页",
    " ^S      档案开头｜ ^T      档案结尾| Esc-s    搜寻   | Esc-l    继续搜寻",
    " Esc-g   跳到##行│",
    "",
    "\01删除命令:",
    " ^O,Ins 切换插入/置换模式 ｜ ^H,BS   删除左方字元  ｜ ^Y    删除一行  ",
    " ^D,DEL 删除目前字元      ｜ ^K      删至行末      ｜                 ",
    "",
    "\01区块处理:",
    " ^U   区块标定    ^C   复制区块      Esc-d   清除区块    Esc-Q  清除标定",
    "",
    "\01特殊命令: (请先按 ESC, 再选择以下项目)",
    " (I)汇入档案     (E)汇出档案      (C)观看彩色编辑成果",
    " (B)设定背景色彩 (F)设定前景色彩  (R)还原色彩      ",
    " (ESC)彩色控制码 (M)区块处理      (L)开或关讯息栏",
    NULL } ;

#endif
void
vedit_key( ch )
int     ch;
{
    int i;
    char tmp[STRLEN];
#define NO_ANSI_MODIFY  if(no_touch) { warn++; break; }

    static int lastindent = -1 ;
    int no_touch, warn,shift;

    if( ch == Ctrl('P') || ch == KEY_UP ||
            ch == Ctrl('N') || ch == KEY_DOWN ) {
        if( lastindent == -1 )
            lastindent = currpnt;
    } else
        lastindent = -1;

    no_touch = (editansi && strchr(currline->data, '\033')) ? 1 : 0;
    warn = 0;


    if(ch < 0x100 && isprint2(ch)) {
        if (no_touch) warn++; else insert_char(ch) ;
    } else switch(ch) {
        case Ctrl('Z'): r_lastmsg(); /* Leeward 98.07.30 support msgX */
            break;
        case Ctrl('I'):
                        NO_ANSI_MODIFY;
    do {  insert_char(' ');
            } while (currpnt & 0x7);
            break;
    case '\r': case '\n':
            NO_ANSI_MODIFY;
            split(currline,currpnt) ;
            break ;
        case Ctrl('G'): /* redraw screen */
                        clear() ;
            redraw_everything = YEA ;
            break ;
            /* Leeward 98.07.30 Change hot key for msgX */
            /*case Ctrl('Z'):  call help screen */
        case Ctrl('Q'): /* call help screen */
#ifndef VEDITOR
                        show_help( "help/edithelp" );
#else
                        show_helpmenu( vedithelp );
#endif
            redraw_everything = YEA ;
            break ;
    case Ctrl('R'): case KEY_LEFT: /* backward character */
    if(currpnt > 0 ){
                currpnt--;
            }
            else if( currline->prev) {
                curr_window_line-- ;
                currln--;
                currline = currline->prev ;
                currpnt = currline->len ;
            }
            break;
            /*case Ctrl('Q'):  Leeward 98.07.30 Change hot key for msgX
                    process_ESC_action('M', '0');
                    marknum=0;            
                    break;*/
        case Ctrl('C'):
                        process_ESC_action('M', '3');
            break;
        case Ctrl('U'):
                if(marknum==0){
                    marknum=1;
                    process_ESC_action('M', '1');
                }else
                    process_ESC_action('M', '2');
            clear();
            break;
    case Ctrl('V'): case KEY_RIGHT: /* forward character */
            if(currline->len != currpnt){
                currpnt++;
            }
            else if( currline->next ) {
                currpnt = 0 ;
                curr_window_line++ ;
                currln++;
                currline = currline->next ;
                if(Origin(currline))
                {
                    curr_window_line-- ;
                    currln--;
                    currline = currline->prev ;
                }
            }
            break ;
    case Ctrl('P'): case KEY_UP: /* Previous line */
            if( currline->prev ) {
                currln--;
                curr_window_line-- ;
                currline = currline->prev ;
                currpnt = (currline->len>lastindent)?lastindent:currline->len;
            }
            break;
    case Ctrl('N'): case KEY_DOWN: /* Next line */
            if( currline->next ) {
                currline = currline->next ;
                curr_window_line++ ;
                currln++;
                if(Origin(currline))
                {
                    currln--;
                    curr_window_line-- ;
                    currline = currline->prev;
                }
                currpnt = (currline->len>lastindent)?lastindent:currline->len;
            }
            break ;
    case Ctrl('B'): case KEY_PGUP: /* previous page */
            top_of_win = back_line( top_of_win,22 ) ;
            currline = back_line( currline, 22 ) ;
            currln-=moveln;
            curr_window_line = getlineno() ;
            if( currpnt > currline->len )  currpnt = currline->len;
            redraw_everything = YEA ;
            break ;
    case Ctrl('F'): case KEY_PGDN: /* next page */
            top_of_win = forward_line( top_of_win, 22 ) ;
            currline = forward_line( currline, 22 ) ;
            currln+=moveln;
            curr_window_line = getlineno() ;
            if( currpnt > currline->len )  currpnt = currline->len;
            if(Origin(currline->prev))
            {
                currln-=2;
                curr_window_line=0;
                currline=currline->prev->prev;
                top_of_win=lastline->prev->prev;
            }
            if(Origin(currline))
            {
                currln--;
                curr_window_line--;
                currline=currline->prev;
            }
            redraw_everything = YEA ;
            break ;
    case Ctrl('A'): case KEY_HOME: /* begin of line */
            currpnt = 0 ;
            break ;
    case Ctrl('E'): case KEY_END: /* end of line */
            currpnt = currline->len ;
            break ;
        case Ctrl('S'): /* start of file */
                        top_of_win = firstline ;
            currline = top_of_win ;
            currpnt = 0 ;
            curr_window_line = 0 ;
            currln=0;
            redraw_everything = YEA ;
            break ;
        case Ctrl('T'): /* tail of file */
                        top_of_win = back_line(lastline,22) ;
            countline();
            currln=moveln;
            currline = lastline ;
            curr_window_line = getlineno() ;
            currpnt = 0 ;
            if(Origin(currline->prev))
    {
                currline=currline->prev->prev;
                currln-=2;
                curr_window_line-=2;
            }
            redraw_everything = YEA ;
            break ;
    case Ctrl('O'): case KEY_INS: /* Toggle insert/overwrite */
            insert_character = !insert_character;
            /*move(0,73);
            prints( " [%s] ", insert_character ? "Ins" : "Rep" );*/
            break;
    case Ctrl('H'): case '\177':  /* backspace */
            NO_ANSI_MODIFY;
            if(currpnt == 0) {
                struct textline *p ;

                if(!currline->prev) {
                    break ;
                }
                currln--;
                curr_window_line-- ;
                currline = currline->prev ;
                currpnt = currline->len ;

                /* Modified by cityhunter on 1999.10.22 */
                /* for the bug of edit two page article */

                if(curr_window_line < 0) /*top_of_win = top_of_win->next;*/
                {
                    top_of_win = currline;
                    curr_window_line = 0;
                }

                /* end of this modification             */
                if(*killsp(currline->next->data) == '\0') {
                    delete_line(currline->next) ;
                    redraw_everything = YEA ;
                    break;
                }
                p = currline ;
                while(!join(p)) {
                    p = p->next ;
                    if(p == NULL) {
                        indigestion(2) ;
                        abort_bbs() ;
                    }
                }
                redraw_everything = YEA ;
                break ;
            }
            currpnt-- ;
            delete_char() ;
            break ;
    case Ctrl('D'): case KEY_DEL: /* delete current character */
            NO_ANSI_MODIFY;
            if(currline->len == currpnt) {
                struct textline *p = currline ;
                if(!Origin(currline->next))
                {
                    while(!join(p))
                    {
                        p = p->next ;
                        if(p == NULL)
                        {
                            indigestion(2) ;
                            abort_bbs() ;
                        }
                    }
                }else if(currpnt==0)
                    vedit_key( Ctrl('K') );
                redraw_everything = YEA ;
                break;
            }
            delete_char() ;
            break ;
        case Ctrl('Y'): /* delete current line */
                        /* STONGLY coupled with Ctrl-K */
                        no_touch = 0;  /* ANSI_MODIFY hack */
            currpnt = 0;
    if(currline->next){
                if(Origin(currline->next)&&!currline->prev){
                    currline->data[0]='\0';
                    currline->len=0;
                    break;
                }
            }else if(currline->prev!=NULL)
            {   currline->len=0;
            }else
            {
                currline->len = 0;
                currline->data[0]='\0';
                break;
            }
            currline->len = 0;
            vedit_key( Ctrl('K') );
            break;
        case Ctrl('K'): /* delete to end of line */
                        NO_ANSI_MODIFY;
            if(currline->prev==NULL&&currline->next==NULL)
    {
                currline->data[0]='\0';
                currpnt=0;
                break;
            }
            if(currline->next)
            {
                if(Origin(currline->next)&&currpnt==currline->len&&currpnt!=0)
                    break;
                if(Origin(currline->next)&&currline->prev==NULL)
                {
                    vedit_key(Ctrl('Y'));
                    break;
                }
            }
            if(currline->len == 0) {
                struct textline *p = currline->next ;
                if(!p) {
                    p = currline->prev ;
                    if(!p) {
                        break ;
                    }
                    if( curr_window_line > 0 )
                        curr_window_line-- ;
                    currln--;
                }
                if(currline == top_of_win)
                    top_of_win = p ;
                delete_line(currline) ;
                currline = p ;
                if(Origin(currline))
                {
                    currline=currline->prev;
                    curr_window_line--;
                    currln--;
                }
                redraw_everything = YEA ;
                break ;
            }
            if(currline->len == currpnt) {
                struct textline *p = currline ;

                while(!join(p)) {
                    p = p->next ;
                    if(p == NULL) {
                        indigestion(2) ;
                        abort_bbs() ;
                    }
                }
                redraw_everything = YEA ;
                break ;
            }
            currline->len = currpnt ;
            currline->data[currpnt] = '\0' ;
            break ;
        default:
            break ;
        }

    if(curr_window_line < 0) {
        curr_window_line = 0 ;
        if(!top_of_win->prev) {
            indigestion(6) ;
        } else {
            top_of_win = top_of_win->prev ;
            /*            redraw_everything = YEA ;
                        move(t_lines-2,0);
                        clrtoeol();
                        refresh();*/
            rscroll();
        }
    }
    if(curr_window_line >= t_lines-1) {
        for(i=curr_window_line-t_lines+1;i>=0;i--)
        {
            curr_window_line --;
            if(!top_of_win->next) {
                indigestion(7) ;
            } else {
                top_of_win = top_of_win->next ;
                /*          redraw_everything = YEA ;
                          move(t_lines-1,0);
                          clrtoeol();
                          refresh();*/
                scroll() ;
            }
        }
    }

    if ( editansi /*|| mark_on*/ ) redraw_everything = YEA;
    shift = (currpnt+2 > STRLEN) ?
            (currpnt/(STRLEN-scrollen))*(STRLEN-scrollen) : 0;
    msgline();
    if(shifttmp!=shift||redraw_everything ==YEA)
    {
        redraw_everything = YEA ;
        shifttmp=shift;
    }
    else
        redraw_everything = NA ;

    move(curr_window_line,0) ;
    if(currline->attr & M_MARK)
    {
        showansi=1;
        cstrnput(currline->data+shift);
        showansi=0;
    }
    else
        strnput(currline->data+shift) ;
    clrtoeol() ;
}

int
raw_vedit(filename, saveheader, headlines)
char *filename ;
int saveheader ;
int headlines ;
{
    int newch, ch=0, foo ,shift;
    struct textline *st_tmp,*st_tmp2;
    read_file(filename) ;
    top_of_win = firstline;
    for(newch=0;newch<headlines;newch++)
        if (top_of_win->next) top_of_win=top_of_win->next;
    /* 跳过headlines 指定行数的头部信息 Luzi 1999/1/8 */
    currline = top_of_win;
    st_tmp2=firstline;
    st_tmp=currline->prev;  /* 保存链表指针，并修改编辑第一行的的指针*/
    currline->prev=NULL;
    firstline=currline;

    curr_window_line = 0 ;
    currln=0;
    currpnt = 0 ;
    clear() ;
    display_buffer() ;
    msgline();
    while( ch != EOF ) {
        newch = '\0';
        switch( ch ) {
    case Ctrl('W'): case Ctrl('X'): /* Save and exit */
                            if (headlines)
                    {
                        st_tmp->next=firstline; /* 退出时候恢复原链表 */
                        firstline->prev=st_tmp;
                        firstline=st_tmp2;
                    }
            foo = write_file(filename,saveheader);
            if( foo != KEEP_EDITING ) return foo;
            if (headlines)
            {
                firstline=st_tmp->next; /* 继续编辑则再次修改第一行的指针*/
                firstline->prev=NULL;
            }
            redraw_everything = YEA ;
            break;
            /*
                      case Ctrl('Q'):
            #ifdef VEDITOR
                           ch = ask("Quit without saving (Y/N)? ");
                           if( ch == 'y' || ch == 'Y' ) {
                               ch = ask("Will you continue edit (Y/N)? ");
                               if( ch == 'n' || ch == 'N' ) {
                                   clear();
                                   return 0;
                               }
                           }
                           redraw_everything = YEA ;
            #endif*/
            break;
        case KEY_ESC:
            if ( KEY_ESC_arg == KEY_ESC ) insert_char(KEY_ESC);
            else {
                newch = vedit_process_ESC(KEY_ESC_arg);
                clear();
            }
            redraw_everything = YEA;
            break;
        default:
            vedit_key( ch );
        }
        if(redraw_everything){
            display_buffer() ;
        }
        redraw_everything = NA ;
        shift = (currpnt+2> STRLEN) ?
                (currpnt/(STRLEN-scrollen))*(STRLEN-scrollen) : 0;
        move(curr_window_line,currpnt-shift) ;

        ch = (newch != '\0') ? newch : igetkey();
    }
    return 1;
}

int
vedit(filename, saveheader)
char *filename ;
int saveheader ;
{
    int ans,t;

    t = showansi;
    showansi=0;
#ifndef VEDITOR
    ismsgline=(DEFINE(DEF_EDITMSG))?1:0;
#else
    ismsgline=1;
#endif
    domsg();
#ifdef VEDITOR
    sprintf(bkfname,"%s~",filename);
    sprintf(currfname,"%s",filename);
#endif
    ans = raw_vedit(filename, saveheader, 0);
    showansi = t;
    return ans;
}

int
vedit_post(filename, saveheader)
char *filename ;
int saveheader ;
{
    int ans,t;

    t = showansi;
    showansi=0;
#ifndef VEDITOR
    ismsgline=(DEFINE(DEF_EDITMSG))?1:0;
#else
    ismsgline=1;
#endif
    domsg();
#ifdef VEDITOR
    sprintf(bkfname,"%s~",filename);
    sprintf(currfname,"%s",filename);
#endif
    ans = raw_vedit(filename, saveheader, 4);/*Haohmaru.99.5.5.应该保留一个空行*/
    showansi = t;
    return ans;
}

