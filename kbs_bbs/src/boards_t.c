/*
		scan complete for global variable
*/

#include "bbs.h"

extern time_t login_start_time;
extern int *zapbuf;
extern int zapbuf_changed;
extern int brdnum;
extern int yank_flag;
static int choose_board(int newflag, char *boardprefix);        /* 选择 版， readnew或readboard */
static int check_newpost(struct newpostdata *ptr);

void EGroup(cmd)
    char *cmd;
{
    char buf[STRLEN];
    char *boardprefix;

    sprintf(buf, "EGROUP%c", *cmd);
    boardprefix = sysconf_str(buf);
    choose_board(DEFINE(currentuser, DEF_NEWPOST) ? 1 : 0, boardprefix);
}

static int clear_all_board_read_flag_func(struct boardheader *bh)
{
    brc_initial(currentuser->userid, bh->filename);
    brc_clear();
}

int clear_all_board_read_flag()
{
    char save_board[BOARDNAMELEN], ans[4];

    getdata(t_lines - 1, 0, "清除所有的未读标记么(Y/N)? [N]: ", ans, 2, DOECHO, NULL, true);
    if (ans[0] == 'Y' || ans[0] == 'y') {

        strncpy(save_board, currboard, BOARDNAMELEN);
        save_board[BOARDNAMELEN - 1] = 0;

        apply_boards(clear_all_board_read_flag_func);
        strcpy(currboard, save_board);
    }
    return 0;
}

void Boards()
{
    choose_board(0, NULL);
}

void New()
{
    choose_board(1, NULL);
}

int cmpboard(brd, tmp)          /*排序用 */
    struct newpostdata *brd, *tmp;
{
    register int type = 0;

    if (!(currentuser->flags[0] & BRDSORT_FLAG)) {
        type = brd->title[0] - tmp->title[0];
        if (type == 0)
            type = strncasecmp(brd->title + 1, tmp->title + 1, 6);

    }
    if (type == 0)
        type = strcasecmp(brd->name, tmp->name);
    return type;
}


int unread_position(dirfile, ptr)
    char *dirfile;
    struct newpostdata *ptr;
{
    struct fileheader fh;
    char filename[STRLEN];
    int fd, offset, step, num;

    num = ptr->total + 1;
    if (ptr->unread && (fd = open(dirfile, O_RDWR)) > 0) {
        if (!brc_initial(currentuser->userid, ptr->name)) {
            num = 1;
        } else {
            offset = (int) ((char *) &(fh.filename[0]) - (char *) &(fh));
            num = ptr->total - 1;
            step = 4;
            while (num > 0) {
                lseek(fd, offset + num * sizeof(fh), SEEK_SET);
                if (read(fd, filename, STRLEN) <= 0 || !brc_unread(FILENAME2POSTTIME(filename)))
                    break;
                num -= step;
                if (step < 32)
                    step += step / 2;
            }
            if (num < 0)
                num = 0;
            while (num < ptr->total) {
                lseek(fd, offset + num * sizeof(fh), SEEK_SET);
                if (read(fd, filename, STRLEN) <= 0 || brc_unread(FILENAME2POSTTIME(filename)))
                    break;
                num++;
            }
        }
        close(fd);
    }
    if (num < 0)
        num = 0;
    return num;
}


int search_board(int *num, int *i, int *find, char *bname)
{
    int n, ch, tmpn = false;

    if (*find == true) {
        bzero(bname, STRLEN);
        *find = false;
        *i = 0;
    }
    while (1) {
        move(t_lines - 1, 0);
        clrtoeol();
        prints("请输入要找寻的 board 名称：%s", bname);
        ch = igetkey();

    	if (ch==KEY_REFRESH)
    		break;
        if (isprint2(ch)) {
            bname[(*i)++] = ch;
            for (n = 0; n < brdnum; n++) {
                if (!strncasecmp(nbrd[n].name, bname, *i)) {
                    tmpn = true;
                    *num = n;
                    if (!strcmp(nbrd[n].name, bname))
                        return 1 /*找到类似的版，画面重画 */ ;
                }
            }
            if (tmpn)
                return 1;
            if (*find == false) {
                bname[--(*i)] = '\0';
            }
            continue;
        } else if (ch == Ctrl('H') || ch == KEY_LEFT || ch == KEY_DEL || ch == '\177') {
            (*i)--;
            if (*i < 0) {
                *find = true;
                break;
            } else {
                bname[*i] = '\0';
                continue;
            }
        } else if (ch == '\t') {
            *find = true;
            break;
        } else if (Ctrl('Z') == ch) {
            r_lastmsg();        /* Leeward 98.07.30 support msgX */
            break;
        } else if (ch == '\n' || ch == '\r' || ch == KEY_RIGHT) {
            *find = true;
            break;
        }
        bell(1);
    }
    if (*find) {
        move(t_lines - 1, 0);
        clrtoeol();
        return 2 /*结束了 */ ;
    }
    return 1;
}

int show_authorBM(ent, fileinfo, direct)
    int ent;
    struct fileheader *fileinfo;
    char *direct;
{
    struct boardheader *bptr;
    int tuid = 0;
    int n;

    if (!HAS_PERM(currentuser, PERM_ACCOUNTS) || !strcmp(fileinfo->owner, "Anonymous") || !strcmp(fileinfo->owner, "deliver"))
        return DONOTHING;
    else {
        struct userec *lookupuser;

        if (!(tuid = getuser(fileinfo->owner, &lookupuser))) {
            clrtobot();
            prints("不正确的使用者代号\n");
            pressanykey();
            move(2, 0);
            clrtobot();
            return FULLUPDATE;
        }

        move(3, 0);
        if (!(lookupuser->userlevel & PERM_BOARDS)) {
            clrtobot();
            prints("用户%s不是版主!\n", lookupuser->userid);
            pressanykey();
            move(2, 0);
            clrtobot();
            return FULLUPDATE;
        }
        clrtobot();
        prints("用户%s为以下版的版主\n\n", lookupuser->userid);

        prints("┏━━━━━━━━━━━━━━━━┳━━━━━━━━━━━━━━━━┓\n");
        prints("┃            版英文名            ┃            版中文名            ┃\n");

        for (n = 0; n < get_boardcount(); n++) {
            bptr = (struct boardheader *) getboard(n + 1);
            if (chk_BM_instr(bptr->BM, lookupuser->userid) == true) {
                prints("┣━━━━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━┫\n");
                prints("┃%-32s┃%-32s┃\n", bptr->filename, bptr->title + 12);
            }
        }
        prints("┗━━━━━━━━━━━━━━━━┻━━━━━━━━━━━━━━━━┛\n");
        pressanykey();
        move(2, 0);
        clrtobot();
        return FULLUPDATE;
    }
}

/* inserted by cityhunter to query BM */
int query_bm()
{
    const struct boardheader *bptr;
    int n;
    char uident[STRLEN];
    int tuid = 0;
    struct userec *lookupuser;

    modify_user_mode(QUERY);
    move(2, 0);
    clrtobot();
    prints("<输入使用者代号, 按空白键可列出符合字串>\n");
    move(1, 0);
    clrtoeol();
    prints("查询谁: ");
    usercomplete(NULL, uident);
    if (uident[0] == '\0') {
        clear();
        return FULLUPDATE;
    }
    if (!(tuid = getuser(uident, &lookupuser))) {
        move(2, 0);
        clrtoeol();
        prints("[1m不正确的使用者代号[m\n");
        pressanykey();
        move(2, 0);
        clrtoeol();
        return FULLUPDATE;
    }

    move(3, 0);
    if (!(lookupuser->userlevel & PERM_BOARDS)) {
        prints("用户%s不是版主!\n", lookupuser->userid);
        pressanykey();
        move(2, 0);
        clrtoeol();
        return FULLUPDATE;
    }
    prints("用户%s为以下版的版主\n\n", lookupuser->userid);

    prints("┏━━━━━━━━━━━━━━━━┳━━━━━━━━━━━━━━━━┓\n");
    prints("┃            版英文名            ┃            版中文名            ┃\n");

    for (n = 0; n < get_boardcount(); n++) {
        bptr = getboard(n + 1);
        if (chk_BM_instr(bptr->BM, lookupuser->userid) == true) {
            prints("┣━━━━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━━┫\n");
            prints("┃%-32s┃%-32s┃\n", bptr->filename, bptr->title + 12);
        }
    }
    prints("┗━━━━━━━━━━━━━━━━┻━━━━━━━━━━━━━━━━┛\n");
    pressanykey();
    move(2, 0);
    clrtoeol();
    return FULLUPDATE;
}

/* end of insertion */

void show_brdlist(page, clsflag, newflag)       /* show board list */
    int page, clsflag, newflag;
{
    struct newpostdata *ptr;
    int n;
    char tmpBM[BM_LEN - 1];
    char buf[STRLEN];           /* Leeward 98.03.28 */

    if (clsflag) {
        clear();
        if(yank_flag==2)
	        docmdtitle("[个人定制区]", "  [m主选单[←,e] 阅读[→,r] 选择[↑,↓] 添加[a,A] 移动[m] 删除[d] 排序[S] 求助[h]\n");
        else
       	 docmdtitle("[讨论区列表]", "  [m主选单[←,e] 阅读[→,r] 选择[↑,↓] 列出[y] 排序[S] 搜寻[/] 切换[c] 求助[h]\n");
        prints("[44m[37m %s 讨论区名称       V  类别 转信  %-24s 版  主   %s   [m\n", newflag ? "全部 未读" : "编号  ", "中  文  叙  述", newflag ? "" : "   ");
    }

    move(3, 0);
    for (n = page; n < page + BBS_PAGESIZE; n++) {
        if (n >= brdnum) {
            prints("\n");
            continue;
        }
        ptr = &nbrd[n];
        if (ptr->flag == -1) {  // added by bad 2002.8.3
            if (ptr->pos == -1)
                prints("       ");
            else if (!newflag)
                prints(" %4d  ＋  <目录>  ", n + 1);
            else
                prints(" %4d  ＋  <目录>  ", ptr->total);
        } else if (!newflag)
            prints(" %4d %c", n + 1, ptr->zap && !(ptr->flag & BOARD_NOZAPFLAG) ? '-' : ' ');   /*zap标志 */
        else if (ptr->zap && !(ptr->flag & BOARD_NOZAPFLAG)) {
            /*ptr->total = ptr->unread = 0;
               prints( "    -    -" ); */
            /* Leeward: 97.12.15: extended display */
            check_newpost(ptr);
            prints(" %4d%s%s ", ptr->total, ptr->total > 9999 ? " " : "  ", ptr->unread ? "◆" : "◇"); /*是否未读 */
        } else {
            if (ptr->total == -1) {
                refresh();
                check_newpost(ptr);
            }
            prints(" %4d%s%s ", ptr->total, ptr->total > 9999 ? " " : "  ", ptr->unread ? "◆" : "◇"); /*是否未读 */
        }
        /* Leeward 98.03.28 Displaying whether a board is READONLY or not */
        if (ptr->flag == -1)
            sprintf(buf, "%s", ptr->title);     // added by bad 2002.8.3
        else if (true == checkreadonly(ptr->name))
            sprintf(buf, "◆只读◆%s", ptr->title + 8);
        else
            sprintf(buf, " %s", ptr->title + 1);

        if (ptr->flag == -1)    // added by bad 2002.8.3
            prints("%-20s\n", buf);
        else {
        	strncpy(tmpBM, ptr->BM,BM_LEN);
            prints("%c%-16s %s%-36s %-12s\n", ((newflag && ptr->zap && !(ptr->flag & BOARD_NOZAPFLAG)) ? '*' : ' '), ptr->name, (ptr->flag & BOARD_VOTEFLAG) ? "[31mV[m" : " ", buf, ptr->BM[0] <= ' ' ? "诚征版主中" : strtok(tmpBM, " "));  /*第一个版主 */
        }
    }
    refresh();
}


static int choose_board(int newflag, char *boardprefix)
{                               /* 选择 版， readnew或readboard */
    static int num;
    struct newpostdata newpost_buffer[MAXBOARD];
    struct newpostdata *ptr;
    int page, ch, tmp, number, tmpnum;
    int loop_mode = 0;
    int i = 0, find = true;
    char bname[STRLEN];

    if (!strcmp(currentuser->userid, "guest"))
        yank_flag = 1;
    nbrd = newpost_buffer;
    modify_user_mode(newflag ? READNEW : READBRD);
    brdnum = number = 0;
/* show_brdlist( 0, 1, newflag ); *//*board list显示 的 2次显示问题解决! 96.9.5 alex */
    while (1) {
        if (brdnum <= 0) {      /*初始化 */
            if (load_boards(boardprefix) == -1)
                continue;
            if((yank_flag!=2)||(currentuser->flags[0]& BRDSORT_FLAG))
                qsort(nbrd, brdnum, sizeof(nbrd[0]), (int (*)(const void *, const void *)) cmpboard);
            page = -1;
            if (brdnum <= 0)
                break;
        }
        if (num < 0)
            num = 0;
        if (num >= brdnum)
            num = brdnum - 1;
        if (page < 0) {
            if (newflag) {      /* 如果是readnew的话，则跳到下一个未读版 */
                tmp = num;
                while (num < brdnum) {
                    ptr = &nbrd[num];
                    if ((ptr->total == -1)&&(ptr->flag != -1))
                        check_newpost(ptr);
                    if (ptr->unread)
                        break;
                    num++;
                }
                if (num >= brdnum)
                    num = tmp;
            }
            page = (num / BBS_PAGESIZE) * BBS_PAGESIZE; /*page计算 */
            show_brdlist(page, 1, newflag);
            update_endline();
        }
        if (num < page || num >= page + BBS_PAGESIZE) {
            page = (num / BBS_PAGESIZE) * BBS_PAGESIZE;
            show_brdlist(page, 0, newflag);
            update_endline();
        }
        move(3 + num - page, 0);
        prints(">", number);    /*显示当前board标志 */
        if (loop_mode == 0) {
            ch = igetkey();
            if (ch==KEY_REFRESH) {
            	show_brdlist(page, 1, newflag);
            	update_endline();
            }
        }
        move(3 + num - page, 0);
        prints(" ");
        if (ch == 'q' || ch == 'e' || ch == KEY_LEFT || ch == EOF)
            break;
        switch (ch) {
        case Ctrl('Z'):
            r_lastmsg();        /* Leeward 98.07.30 support msgX */
            break;
            /*
               case 'R':   Leeward 98.04.24 
               {
               char fname[STRLEN], restore[256];

               if(!strcmp(currentuser->userid,"guest")) 
               break;

               saveline(t_lines-2, 0, NULL);
               move(t_lines-2, 0);
               clrtoeol();
               getdata(t_lines-2, 0,"[1m[5m[31m立即断线[m∶[1m[33m以便恢复上次正常离开本站时的未读标记 (Y/N)？ [N][m: ", restore,4,DOECHO,NULL,true);
               if ('y' == restore[0] || 'Y' == restore[0])
               {
               sethomefile(fname, currentuser->userid,".boardrc" );
               sprintf(restore, "%s.bak", fname);
               f_cp(restore,fname,0);

               move(t_lines-2, 0);
               clrtoeol();
               prints("[1m[33m已恢复上次正常离开本站时的未读标记[m");
               move(t_lines-1, 0);
               clrtoeol();
               getdata(t_lines-1, 0,"[1m[32m请按 Enter 断线，然后重新登录 8-) [m", restore,1,DOECHO,NULL,true);
               abort_bbs(0);
               }
               saveline(t_lines-2, 1, NULL);
               break;
               }
             */
        case 'X':              /* Leeward 98.03.28 Set a board READONLY */
            {
                char buf[STRLEN];

                if (!HAS_PERM(currentuser, PERM_SYSOP) && !HAS_PERM(currentuser, PERM_OBOARDS))
                    break;
                if (!strcmp(nbrd[num].name, "syssecurity")
                    || !strcmp(nbrd[num].name, "Filter")
                    || !strcmp(nbrd[num].name, "junk")
                    || !strcmp(nbrd[num].name, "deleted"))
                    break;      /* Leeward 98.04.01 */

                if (strlen(nbrd[num].name)) {
                    board_setreadonly(nbrd[num].name, 1);

                    /* Bigman 2000.12.11:系统记录 */
                    sprintf(genbuf, "只读讨论区 %s ", nbrd[num].name);
                    securityreport(genbuf, NULL, NULL);
                    sprintf(genbuf, " readonly board %s", nbrd[num].name);
                    report(genbuf);

                    show_brdlist(page, 0, newflag);
                }
                break;
            }
        case 'Y':              /* Leeward 98.03.28 Set a board READABLE */
            {
                char buf[STRLEN];

                if (!HAS_PERM(currentuser, PERM_SYSOP) && !HAS_PERM(currentuser, PERM_OBOARDS))
                    break;

                board_setreadonly(nbrd[num].name, 0);

                /* Bigman 2000.12.11:系统记录 */
                sprintf(genbuf, "解开只读讨论区 %s ", nbrd[num].name);
                securityreport(genbuf, NULL, NULL);
                sprintf(genbuf, " readable board %s", nbrd[num].name);
                report(genbuf);

                show_brdlist(page, 0, newflag);
                break;
            }
        case 'L':
        case 'l':              /* Luzi 1997.10.31 */
            if (uinfo.mode != LOOKMSGS) {
                show_allmsgs();
                page = -1;
                break;
            } else
                return DONOTHING;
        case 'H':              /* Luzi 1997.10.31 */
            r_lastmsg();
            break;
        case 'W':
        case 'w':              /* Luzi 1997.10.31 */
            if (!HAS_PERM(currentuser, PERM_PAGE))
                break;
            s_msg();
            page = -1;
            break;
        case 'u':              /*Haohmaru.99.11.29 */
            {
                int oldmode = uinfo.mode;

                clear();
                modify_user_mode(QUERY);
                t_query(NULL);
                page = -1;
                modify_user_mode(oldmode);
                break;
            }
        case '!':
            Goodbye();
            page = -1;
            break;
        case 'O':
        case 'o':              /* Luzi 1997.10.31 */
            {                   /* Leeward 98.10.26 fix a bug by saving old mode */
                int savemode = uinfo.mode;

                if (!HAS_PERM(currentuser, PERM_BASIC))
                    break;
                t_friends();
                page = -1;
                modify_user_mode(savemode);
                /* return FULLUPDATE; */
                break;
            }
        case 'P':
        case 'b':
        case Ctrl('B'):
        case KEY_PGUP:
            if (num == 0)
                num = brdnum - 1;
            else
                num -= BBS_PAGESIZE;
            break;
        case 'C':
        case 'c':              /*阅读模式 */
            if (newflag == 1)
                newflag = 0;
            else
                newflag = 1;
            show_brdlist(page, 1, newflag);
            break;
        case 'N':
        case ' ':
        case Ctrl('F'):
        case KEY_PGDN:
            if (num == brdnum - 1)
                num = 0;
            else
                num += BBS_PAGESIZE;
            break;
        case 'p':
        case 'k':
        case KEY_UP:
            if (num-- <= 0)
                num = brdnum - 1;
            break;
        case 'n':
        case 'j':
        case KEY_DOWN:
            if (++num >= brdnum)
                num = 0;
            break;
        case '$':
            num = brdnum - 1;
            break;
        case 'h':
            show_help("help/boardreadhelp");
            page = -1;
            break;
        case '/':              /*搜索board */
            move(3 + num - page, 0);
            prints(">", number);
            tmpnum = num;
            tmp = search_board(&num, &i, &find, bname);
            move(3 + tmpnum - page, 0);
            prints(" ", number);
            if (tmp == 1)
                loop_mode = 1;
            else {
                find = true;
                i = 0;
                loop_mode = 0;
                update_endline();
            }
            break;
	case 'S':
            currentuser->flags[0] ^= BRDSORT_FLAG;      /*排序方式 */
            if(yank_flag!=2){
                    qsort(nbrd, brdnum, sizeof(nbrd[0]), (int (*)(const void *, const void *)) cmpboard);       /*排序 */
                    page = 999;
            } else {
                    if (currentuser->flags[0]& BRDSORT_FLAG) {      /*排序方式 */
                    	qsort(nbrd, brdnum, sizeof(nbrd[0]), (int (*)(const void *, const void *)) cmpboard);       /*排序 */
		    }
            	    else if (load_boards(boardprefix) == -1)
			continue;
                    page = 999;
	    }
	    break;
        case 's':              /* sort/unsort -mfchen */
	    /*
            if(yank_flag!=2){
	            currentuser->flags[0] ^= BRDSORT_FLAG;   
	            qsort(nbrd, brdnum, sizeof(nbrd[0]), (int (*)(const void *, const void *)) cmpboard);     
	            page = 999;
            }
            else {
	    */
            	modify_user_mode(SELECT);
		if(do_select(0, NULL, genbuf)==NEWDIRECT)
            	Read();
		if (nbrd!=newpost_buffer)
			nbrd=newpost_buffer;
              show_brdlist(page, 1, newflag);     /*  refresh screen */
	      brdnum = -1;
              modify_user_mode(newflag ? READNEW : READBRD);
	      /*
            }
	    */
            break;
            /*---	added period 2000-09-11	4 FavBoard	---*/
        case 'a':
            if (2 == yank_flag) {
                char bname[STRLEN];
                int i = 0;

                if (getfavnum() >= FAVBOARDNUM) {
                    move(2, 0);
                    clrtoeol();
                    prints("个人热门版数已经达上限(%d)！", FAVBOARDNUM);
                    pressreturn();
                    show_brdlist(page, 1, newflag);     /*  refresh screen */
                    break;
                }
                move(0, 0);
                clrtoeol();
                prints("输入讨论区英文名 (大小写皆可，按空白键自动搜寻): ");
                clrtoeol();

                make_blist();
                namecomplete((char *) NULL, bname);
                CreateNameList();       /*  free list memory. */
                if (*bname)
                    i = getbnum(bname);
                if (i > 0 && !IsFavBoard(i - 1)) {
                    addFavBoard(i - 1);
                    save_favboard();
                    brdnum = -1;        /*  force refresh board list */
                } else if (IsFavBoard(i - 1)) {
                    move(2, 0);
                    prints("已存在该讨论区.\n");
                    pressreturn();
                    show_brdlist(page, 1, newflag);     /*  refresh screen */
                } else {
                    move(2, 0);
                    prints("不正确的讨论区.\n");
                    pressreturn();
                    show_brdlist(page, 1, newflag);     /*  refresh screen */
                }
            }
            break;
        case 'A':              // added by bad 2002.8.3
            if (2 == yank_flag) {
                char bname[STRLEN];
                int i < 0;

                if (getfavnum() >= FAVBOARDNUM) {
                    move(2, 0);
                    clrtoeol();
                    prints("个人热门版数已经达上限(%d)！", FAVBOARDNUM);
                    pressreturn();
                    show_brdlist(page, 1, newflag);     /*  refresh screen */
                    break;
                }
                move(0, 0);
                clrtoeol();
                getdata(0, 0, "输入讨论区目录名: ", bname, 22, DOECHO, NULL, true);
                if (bname[0]) {
                    addFavBoardDir(i, bname);
                    save_favboard();
                    brdnum = -1;        /*  force refresh board list */
                }
            }
            break;
        case 'T':              // added by bad 2002.8.3
            if (2 == yank_flag) {
                char bname[STRLEN];
                int i = 0;

                if (nbrd[num].flag == -1 && nbrd[num].pos >= 0) {
                    move(0, 0);
                    clrtoeol();
                    getdata(0, 0, "输入讨论区目录名: ", bname, 22, DOECHO, NULL, true);
                    if (bname[0]) {
                        changeFavBoardDir(nbrd[num].pos, bname);
                        save_favboard();
                        brdnum = -1;    /*  force refresh board list */
                    }
                }
            }
            break;
        case 'm':
                if(yank_flag==2) {
		  if (currentuser->flags[0]& BRDSORT_FLAG) {
			  move(0,0);
			  prints("排序模式下不能移动，请用'S'键切换!");
			  pressreturn();
		  } else {
        	if (nbrd[num].flag != -1 || nbrd[num].pos != -1){
        		int p,q;
        		char ans[5];
                     if (nbrd[num].flag == -1)
                         p = nbrd[num].pos;
                     else
                         p = IsFavBoard(nbrd[num].pos) - 1;
                    move(0, 0);
                    clrtoeol();
                    getdata(0, 0, "请输入移动到的位置:", ans, 4, DOECHO, NULL, true);
                    q=atoi(ans)-1;
                    if (q<0||q>=brdnum) {
	                    move(2, 0);
	                    clrtoeol();
	                    prints("非法的移动位置！");
	                    pressreturn();
	                    show_brdlist(page, 1, newflag);     /*  refresh screen */
                    }
                    else {
                          if (q==0) q=0;
                          else 
                          	if (nbrd[q].flag == -1) q=nbrd[q].pos;
                          	else q=IsFavBoard(nbrd[q].pos)-1;
                          MoveFavBoard(p,q);
                          save_favboard();
                          brdnum = -1;
                    }
		}
		}
                    show_brdlist(page, 1, newflag);     /*  refresh screen */
        	}
        	break;
        case 'd':
            if (2 == yank_flag) {
                int p = 1;

                if (nbrd[num].flag == -1 && nbrd[num].pos == -1)
                    p = 0;
                if (nbrd[num].flag == -1 && nbrd[num].total > 0) {
                    char ans[2];

                    move(0, 0);
                    clrtoeol();
                    getdata(0, 0, "确认删除整个目录？(y/N)", ans, 2, DOECHO, NULL, true);
                    p = ans[0] == 'Y' || ans[0] == 'y';
                }
                if (p) {
                    if (nbrd[num].flag == -1)
                        DelFavBoard(nbrd[num].pos);
                    else
                        DelFavBoard(IsFavBoard(nbrd[num].pos) - 1);
                    save_favboard();
                    brdnum = -1;        /*  force refresh board list. */
                } else
                    show_brdlist(page, 1, newflag);     /*  refresh screen */
            }
            break;
            /*---	End of Addition	---*/
        case 'y':
            if (yank_flag < 2) {
                                /*--- Modified 4 FavBoard 2000-09-11	---*/
                yank_flag = !yank_flag;
                brdnum = -1;
            }
            break;
        case 'z':              /* Zap */
            if (yank_flag < 2) {
                                /*--- Modified 4 FavBoard 2000-09-11	---*/
                if (HAS_PERM(currentuser, PERM_BASIC) && !(nbrd[num].flag & BOARD_NOZAPFLAG)) {
                    ptr = &nbrd[num];
                    ptr->zap = !ptr->zap;
                    ptr->total = -1;
                    zapbuf[ptr->pos] = (ptr->zap ? 0 : login_start_time);
                    zapbuf_changed = 1;
                    page = 999;
                }
            }
            break;
        case KEY_HOME:
            num = 0;
            break;
        case KEY_END:
            num = brdnum - 1;
            break;
        case '\n':
        case '\r':             /*直接输入数字 跳转 */
            if (number > 0) {
                num = number - 1;
                break;
            }
            /* fall through */
        case 'r':
        case KEY_RIGHT:        /*进入 board */
            {
                char buf[STRLEN];

                ptr = &nbrd[num];

                if (ptr->flag == -1) {  // added by bad 2002.8.3
                    int oldnum, oldfavnow;

                    oldnum = num;
                    num = 0;
                    if (ptr->pos != -1) {
                        oldfavnow = SetFav(ptr->pos);
                        choose_board(newflag, boardprefix);
                        SetFav(oldfavnow);
                        num = oldnum;
                        page = -1;
                        brdnum = -1;
                        modify_user_mode(newflag ? READNEW : READBRD);
                    }
                } else {
                    brc_initial(currentuser->userid, ptr->name);
                    memcpy(currBM, ptr->BM, BM_LEN - 1);
                    if (DEFINE(currentuser, DEF_FIRSTNEW)) {
                        setbdir(digestmode, buf, currboard);
                        tmp = unread_position(buf, ptr);
                        page = tmp - t_lines / 2;
                        getkeep(buf, page > 1 ? page : 1, tmp + 1);
                    }
                    Read();

                    /* Leeward 98.05.17: updating unread flag on exiting Read() */
                    /* if (-1 != load_boards())
                       qsort( nbrd, brdnum, sizeof( nbrd[0] ), cmpboard ); */

                    /* 他想把zap版面的时间定义为上次阅读的时间，但是没有使用
                       if( zapbuf[ ptr->pos ] > 0 ) 
                       zapbuf[ ptr->pos ] = brc_list[0];
                     */
		    if(nbrd != newpost_buffer)
			nbrd = newpost_buffer;
                    brdnum  = -1;
                    modify_user_mode(newflag ? READNEW : READBRD);
                }
                break;
            }
        case 'v':              /*Haohmaru.2000.4.26 */
            clear();
            m_read();
            show_brdlist(page, 1, newflag);
            break;
        default:
            ;
        }
        if (ch >= '0' && ch <= '9') {
            number = number * 10 + (ch - '0');
            ch = '\0';
        } else {
            number = 0;
        }
    }
    clear();
    save_zapbuf();
    return -1;
}

static int check_newpost(struct newpostdata *ptr)
{
    struct BoardStatus *bptr;

    ptr->total = ptr->unread = 0;

    bptr = getbstatus(ptr->pos);
    if (bptr == NULL)
        return 0;
    ptr->total = bptr->total;

    if (!brc_initial(currentuser->userid, ptr->name)) {
        ptr->unread = 1;
    } else {
        if (brc_unread(bptr->lastpost)) {
            ptr->unread = 1;
        }
    }
    return 1;
}

/*---   Added by period 2000-09-11      Favorate Board List     ---*
 *---   use yank_flag=2 to reflect status                       ---*
 *---   corresponding code added: comm_lists.c                  ---*
 *---           add entry in array sysconf_cmdlist[]            ---*/
void FavBoard()
{
    int ifnew = 1, yanksav;

/*    if(heavyload()) ifnew = 0; *//* no heavyload() in FB2.6x */
    yanksav = yank_flag;
    yank_flag = 2;
    if (!getfavnum())
        load_favboard(1);
    choose_board(ifnew, NULL);
    yank_flag = yanksav;
}
