/*
		scan complete for global variable
*/

#include "bbs.h"

extern time_t login_start_time;
extern int *zapbuf;
extern int zapbuf_changed;
static int yank_flag=0;
extern int favbrd_list_t, favnow;
int choose_board(int newflag, char *boardprefix,int yank_flag);       /* 选择 版， readnew或readboard */
static int check_newpost(struct newpostdata *ptr);

void EGroup(cmd)
char *cmd;
{
    char buf[STRLEN];
    char *boardprefix;

    sprintf(buf, "EGROUP%c", *cmd);
    boardprefix = sysconf_str(buf);
    choose_board(DEFINE(currentuser, DEF_NEWPOST) ? 1 : 0, boardprefix,0);
}

static int clear_all_board_read_flag_func(struct boardheader *bh,void* arg)
{
    if (brc_initial(currentuser->userid, bh->filename) != 0)
        brc_clear();
}

int clear_all_board_read_flag()
{
    char ans[4];
    struct boardheader* save_board;
    int bid;

    getdata(t_lines - 1, 0, "清除所有的未读标记么(Y/N)? [N]: ", ans, 2, DOECHO, NULL, true);
    if (ans[0] == 'Y' || ans[0] == 'y') {

        bid=currboardent;
        save_board=currboard;
        apply_boards(clear_all_board_read_flag_func,NULL);
        currboard=save_board;
        currboardent=bid;
    }
    return 0;
}

void Boards()
{
    choose_board(0, NULL,0);
}

void New()
{
    choose_board(1, NULL,0);
}

int unread_position(dirfile, ptr)
char *dirfile;
struct newpostdata *ptr;
{
    struct fileheader fh;
    int id;
    int fd, offset, step, num;

    num = ptr->total + 1;
    if (ptr->unread && (fd = open(dirfile, O_RDWR)) > 0) {
        if (!brc_initial(currentuser->userid, ptr->name)) {
            num = 1;
        } else {
            offset = (int) ((char *) &(fh.id) - (char *) &(fh));
            num = ptr->total - 1;
            step = 4;
            while (num > 0) {
                lseek(fd, offset + num * sizeof(fh), SEEK_SET);
                if (read(fd, &id, sizeof(unsigned int)) <= 0 || !brc_unread(id))
                    break;
                num -= step;
                if (step < 32)
                    step += step / 2;
            }
            if (num < 0)
                num = 0;
            while (num < ptr->total) {
                lseek(fd, offset + num * sizeof(fh), SEEK_SET);
                if (read(fd, &id, sizeof(unsigned int)) <= 0 || brc_unread(id))
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

extern int load_mboards();
extern void mailtitle();
extern char *maildoent(char *buf, int num, struct fileheader *ent);

static int check_newpost(struct newpostdata *ptr)
{
    struct BoardStatus *bptr;

    if (ptr->dir)
        return 0;

    ptr->total = ptr->unread = 0;

    bptr = getbstatus(ptr->pos+1);
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


enum board_mode {
    BOARD_BOARD,
    BOARD_BOARDALL,
    BOARD_FAV
};

struct favboard_proc_arg {
    struct newpostdata *nbrd;
    int newflag;
    int tmpnum;
    enum board_mode yank_flag;
    int fav_father;
    bool reloaddata;

    char* boardprefix;
    /*用于search_board得时候缓存*/
    int loop_mode;
    int find;
    char bname[BOARDNAMELEN + 1];
    int bname_len;
    char** namelist;
};

static int search_board(int *num, struct _select_def *conf, int key)
{
    struct favboard_proc_arg *arg = (struct favboard_proc_arg *) conf->arg;
    int n, ch, tmpn = false;
    struct newpostdata* buffer;

    *num=0;
    if (arg->find == true) {
        bzero(arg->bname, BOARDNAMELEN);
        arg->find = false;
        arg->bname_len = 0;
    }
    if (arg->namelist==NULL) {
    	arg->namelist=(char**)malloc(MAXBOARD*sizeof(char*));
    	conf->get_data(conf,-1,-1);
    }
    while (1) {
        move(t_lines - 1, 0);
        clrtoeol();
        prints("请输入要找寻的 board 名称：%s", arg->bname);
        if (key == -1)
            ch = igetkey();
        else {
            ch = key;
            key = -1;
        }

        if (ch == KEY_REFRESH)
            break;
        if (isprint2(ch)) {
            arg->bname[arg->bname_len++] = ch;
            for (n = 0; n < conf->item_count; n++) {
                if ((!strncasecmp(arg->namelist[n], arg->bname, arg->bname_len))&&*num==0) {
                    tmpn = true;
                    *num = n;
                }
                if (!strcmp(arg->namelist[n], arg->bname))
                    return 1 /*找到类似的版，画面重画 */ ;
            }
            if (tmpn)
                return 1;
            if (arg->find == false) {
                arg->bname[--arg->bname_len] = '\0';
            }
            continue;
        } else if (ch == Ctrl('H') || ch == KEY_LEFT || ch == KEY_DEL || ch == '\177') {
            arg->bname_len--;
            if (arg->bname_len< 0) {
                arg->find = true;
                break;
            } else {
                arg->bname[arg->bname_len]=0;
                continue;
            }
        } else if (ch == '\t') {
            arg->find = true;
            break;
        } else if (Ctrl('Z') == ch) {
            r_lastmsg();        /* Leeward 98.07.30 support msgX */
            break;
        } else if (ch == '\n' || ch == '\r' || ch == KEY_RIGHT) {
            arg->find = true;
            break;
        }
        bell();
    }
    if (arg->find) {
        move(t_lines - 1, 0);
        clrtoeol();
        return 2 /*结束了 */ ;
    }
    return 1;
}

static int fav_show(struct _select_def *conf, int pos)
{
    struct favboard_proc_arg *arg = (struct favboard_proc_arg *) conf->arg;
    struct newpostdata *ptr;
    char buf[LENGTH_SCREEN_LINE];

    ptr = &arg->nbrd[pos-(conf->page_pos)];
    if (ptr->dir == 1) {        /* added by bad 2002.8.3*/
        if (ptr->tag < 0)
            prints("       ");
        else if (!arg->newflag)
            prints(" %4d  ＋  <目录>  ", pos);
        else
            prints(" %4d  ＋  <目录>  ", ptr->total);
    } else {
        if (!arg->newflag){
			check_newpost(ptr);
            prints(" %4d  %s ", pos, ptr->unread?"◆" : "◇");     /*zap标志 */
		}
        else if (ptr->zap && !(ptr->flag & BOARD_NOZAPFLAG)) {
            /*
             * ptr->total = ptr->unread = 0;
             * prints( "    -    -" ); 
             */
            /*
             * Leeward: 97.12.15: extended display 
             */
            check_newpost(ptr);
            prints(" %4d%s%s ", ptr->total, ptr->total > 9999 ? " " : "  ", ptr->unread ? "◆" : "◇"); /*是否未读 */
        } else {
            if (ptr->total == -1) {
                /*refresh();*/
                check_newpost(ptr);
            }
            prints(" %4d%s%s ", ptr->total, ptr->total > 9999 ? " " : "  ", ptr->unread ? "◆" : "◇"); /*是否未读 */
        }
    }
    /*
     * Leeward 98.03.28 Displaying whether a board is READONLY or not 
     */
    if (ptr->dir == 2)
        sprintf(buf, "%s(%d)", ptr->title, ptr->total);
    else if (ptr->dir >= 1)
        sprintf(buf, "%s", ptr->title); /* added by bad 2002.8.3*/
    else if (true == checkreadonly(ptr->name))
        sprintf(buf, "[只读] %s", ptr->title + 8);
    else
        sprintf(buf, "%s", ptr->title + 1);

    if (ptr->dir >= 1)          /* added by bad 2002.8.3*/
        prints("%-50s\n", buf);
    else {
          char flag[20];
          char f;
          char tmpBM[BM_LEN + 1];

          strncpy(tmpBM, ptr->BM, BM_LEN);
          tmpBM[BM_LEN] = 0;
  
          if ((ptr->flag & BOARD_CLUB_READ) && (ptr->flag & BOARD_CLUB_WRITE))
                 f='A';
          else if (ptr->flag & BOARD_CLUB_READ)
                f = 'c';
          else if (ptr->flag & BOARD_CLUB_WRITE)
                f = 'p';
          else
                f = ' ';
          if (ptr->flag & BOARD_CLUB_HIDE) {
	            sprintf(flag,"\x1b[1;31m%c\x1b[m",f);
	       } else if (f!=' ') {
	           sprintf(flag,"\x1b[1;33m%c\x1b[m",f);
          } else sprintf(flag,"%c",f);
          prints("%c%-16s %s%s%-36s %-12s\n", ((ptr->zap && !(ptr->flag & BOARD_NOZAPFLAG)) ? '*' : ' '), ptr->name, (ptr->flag & BOARD_VOTEFLAG) ? "[31mV[m" : " ", flag, buf, ptr->BM[0] <= ' ' ? "诚征版主中" : strtok(tmpBM, " ")); /*第一个版主 */
    }
    return SHOW_CONTINUE;
}

static int fav_prekey(struct _select_def *conf, int *command)
{
    struct favboard_proc_arg *arg = (struct favboard_proc_arg *) conf->arg;
    struct newpostdata *ptr;

    if (arg->loop_mode) {
        int tmp, num;
/* search_board有问题，目前我们只有一页的数据，只能search
一页*/
        tmp = search_board(&num, conf, *command);
        if (tmp == 1) {
            conf->new_pos = num + 1;
            arg->loop_mode = 1;
            move(t_lines - 1, 0);
            clrtoeol();
            prints("请输入要找寻的 board 名称：%s", arg->bname);
            return SHOW_SELCHANGE;
        } else {
            arg->find = true;
            arg->bname_len = 0;
            arg->loop_mode = 0;
            conf->new_pos = num + 1;
            return SHOW_REFRESH;
        }
        return SHOW_REFRESH;
    }
    if ((*command == '\r' || *command == '\n') && (arg->tmpnum != 0)) {
        /* 直接输入数字跳转*/
        conf->new_pos = arg->tmpnum;
        arg->tmpnum = 0;
        return SHOW_SELCHANGE;
    }

    if (!isdigit(*command))
        arg->tmpnum = 0;

    if (!arg->loop_mode) {
        int y,x;
        getyx(&y, &x);
        update_endline();
        move(y, x);
    }
    ptr = &arg->nbrd[conf->pos - conf->page_pos];
    switch (*command) {
    case 'e':
    case 'q':
        *command = KEY_LEFT;
        break;
    case 'p':
    case 'k':
        *command = KEY_UP;
        break;
    case ' ':
    case 'N':
        *command = KEY_PGDN;
        break;
    case 'n':
    case 'j':
        *command = KEY_DOWN;
        break;
    };
    return SHOW_CONTINUE;
}

static int fav_gotonextnew(struct _select_def *conf)
{
    struct favboard_proc_arg *arg = (struct favboard_proc_arg *) conf->arg;
    int tmp, num,page_pos=conf->page_pos;

        /*搜寻下一个未读的版面*/
    if (arg->newflag) {
      num = tmp = conf->pos;
      while(num<=conf->item_count) {
          while ((num <= page_pos+conf->item_per_page-1)&&num<=conf->item_count) {
              struct newpostdata *ptr;
  
              ptr = &arg->nbrd[num - page_pos];
              if ((ptr->total == -1) && (ptr->dir == 0))
                  check_newpost(ptr);
              if (ptr->unread)
                  break;
                  num++;
          }
          if ((num <= page_pos+conf->item_per_page-1)&&num<=conf->item_count) {
              conf->pos = num;
              conf->page_pos=page_pos;
  	     return SHOW_DIRCHANGE;
  	 }
          page_pos+=conf->item_per_page;
          num=page_pos;
          (*conf->get_data)(conf, page_pos, conf->item_per_page);
      }
      if (page_pos!=conf->page_pos)
        (*conf->get_data)(conf, conf->page_pos, conf->item_per_page);
    }
    return SHOW_REFRESH;
}

static int fav_onselect(struct _select_def *conf)
{
    struct favboard_proc_arg *arg = (struct favboard_proc_arg *) conf->arg;
    char buf[STRLEN];
    struct newpostdata *ptr;

    ptr = &arg->nbrd[conf->pos - conf->page_pos];

    if (ptr->dir == 1) {        /* added by bad 2002.8.3*/
        return SHOW_SELECT;
    } else {
        struct boardheader bh;
        int tmp, page;

        if (getboardnum(ptr->name, &bh) != 0 && check_read_perm(currentuser, &bh)) {
            brc_initial(currentuser->userid, ptr->name);
            memcpy(currBM, ptr->BM, BM_LEN - 1);
            if (DEFINE(currentuser, DEF_FIRSTNEW)) {
                setbdir(digestmode, buf, currboard->filename);
                tmp = unread_position(buf, ptr);
                page = tmp - t_lines / 2;
                getkeep(buf, page > 1 ? page : 1, tmp + 1);
            }
            Read();

            (*conf->get_data)(conf, conf->page_pos, conf->item_per_page);
            modify_user_mode(SELECT);
            if (arg->newflag) { /* 如果是readnew的话，则跳到下一个未读版 */
                return fav_gotonextnew(conf);
            }
        }
        return SHOW_REFRESH;
    }
    return SHOW_CONTINUE;
}

static int fav_key(struct _select_def *conf, int command)
{
    struct favboard_proc_arg *arg = (struct favboard_proc_arg *) conf->arg;
    struct newpostdata *ptr;

    ptr = &arg->nbrd[conf->pos - conf->page_pos];
    if (command >= '0' && command <= '9') {
        arg->tmpnum = arg->tmpnum * 10 + (command - '0');
        return SHOW_CONTINUE;
    }
    switch (command) {
    case Ctrl('Z'):
        r_lastmsg();            /* Leeward 98.07.30 support msgX */
        break;
    case 'X':                  /* Leeward 98.03.28 Set a board READONLY */
        {
            char buf[STRLEN];

            if (!HAS_PERM(currentuser, PERM_SYSOP) && !HAS_PERM(currentuser, PERM_OBOARDS))
                break;
            if (!strcmp(ptr->name, "syssecurity")
                || !strcmp(ptr->name, "Filter")
                || !strcmp(ptr->name, "junk")
                || !strcmp(ptr->name, "deleted"))
                break;          /* Leeward 98.04.01 */
            if (ptr->dir)
                break;

            if (strlen(ptr->name)) {
                board_setreadonly(ptr->name, 1);

                /*
                 * Bigman 2000.12.11:系统记录 
                 */
                sprintf(genbuf, "只读讨论区 %s ", ptr->name);
                securityreport(genbuf, NULL, NULL);
                sprintf(genbuf, " readonly board %s", ptr->name);
                bbslog("1user", "%s", genbuf);

                return SHOW_REFRESHSELECT;
            }
            break;
        }
    case 'Y':                  /* Leeward 98.03.28 Set a board READABLE */
        {
            char buf[STRLEN];

            if (!HAS_PERM(currentuser, PERM_SYSOP) && !HAS_PERM(currentuser, PERM_OBOARDS))
                break;
            if (ptr->dir)
                break;

            board_setreadonly(ptr->name, 0);

            /*
             * Bigman 2000.12.11:系统记录 
             */
            sprintf(genbuf, "解开只读讨论区 %s ", ptr->name);
            securityreport(genbuf, NULL, NULL);
            sprintf(genbuf, " readable board %s", ptr->name);
            bbslog("1user", "%s", genbuf);

            return SHOW_REFRESHSELECT;
        }
        break;
    case 'L':
    case 'l':                  /* Luzi 1997.10.31 */
        if (uinfo.mode != LOOKMSGS) {
            show_allmsgs();
            return SHOW_REFRESH;
        }
        break;
    case 'W':
    case 'w':                  /* Luzi 1997.10.31 */
        if (!HAS_PERM(currentuser, PERM_PAGE))
            break;
        s_msg();
        return SHOW_REFRESH;
    case 'u':                  /*Haohmaru.99.11.29 */
        {
            int oldmode = uinfo.mode;

            clear();
            modify_user_mode(QUERY);
            t_query(NULL);
            modify_user_mode(oldmode);
            return SHOW_REFRESH;
        }
	/*add by stiger */
    case 'H':
	{
		read_hot_info(0,NULL,NULL);
    	return SHOW_REFRESH;
	}
	/* add end */
    case '!':
        Goodbye();
        return SHOW_REFRESH;
    case 'O':
    case 'o':                  /* Luzi 1997.10.31 */
#ifdef NINE_BUILD
    case 'C':
    case 'c':
#endif
        {                       /* Leeward 98.10.26 fix a bug by saving old mode */
            int savemode;

            savemode = uinfo.mode;

            if (!HAS_PERM(currentuser, PERM_BASIC))
                break;
            t_friends();
            modify_user_mode(savemode);
            return SHOW_REFRESH;
        }
#ifdef NINE_BUILD
    case 'F':
    case 'f':
#else
    case 'C':
    case 'c':                  /*阅读模式 */
#endif
        if (arg->newflag == 1)
            arg->newflag = 0;
        else
            arg->newflag = 1;
        return SHOW_REFRESH;
    case 'h':
        show_help("help/boardreadhelp");
        return SHOW_REFRESH;
    case '/':                  /*搜索board */
        {
            int tmp, num;

            tmp = search_board(&num, conf , -1);
            if (tmp == 1) {
                conf->new_pos = num + 1;
                arg->loop_mode = 1;
                move(t_lines - 1, 0);
                clrtoeol();
                prints("请输入要找寻的 board 名称：%s", arg->bname);
                return SHOW_SELCHANGE;
            } else {
                arg->find = true;
                arg->bname_len = 0;
                arg->loop_mode = 0;
                conf->new_pos = num + 1;
                return SHOW_REFRESH;
            }
            return SHOW_REFRESH;
        }
    case 'S':
        currentuser->flags[0] ^= BRDSORT_FLAG;  /*排序方式 */
        return SHOW_DIRCHANGE;
    case 's':                  /* sort/unsort -mfchen */
        modify_user_mode(SELECT);
        if (do_select(0, NULL, genbuf) == NEWDIRECT) {
            Read();
        }
        modify_user_mode(arg->newflag ? READNEW : READBRD);
        return SHOW_REFRESH;
    case 'a':
        {
            char bname[STRLEN];
            int i = 0;

            if (favbrd_list_t >= FAVBOARDNUM) {
                move(2, 0);
                clrtoeol();
                prints("个人热门版数已经达上限(%d)！", FAVBOARDNUM);
                pressreturn();
                return SHOW_REFRESH;
            }
            if (BOARD_FAV == arg->yank_flag) {
	            move(0, 0);
	            clrtoeol();
	            prints("输入讨论区英文名 (大小写皆可，按空白键自动搜寻): ");
	            clrtoeol();

	            make_blist();
	            namecomplete((char *) NULL, bname);
	            CreateNameList();   /*  free list memory. */
	            if (*bname)
	                i = getbnum(bname);
	            if (i==0)
	            	return SHOW_REFRESH;
            } else {
        		struct boardheader bh;
                move(2, 0);
                clrtoeol();
        		i=getboardnum(ptr->name, &bh);
        		if (i<=0)
        			return SHOW_REFRESH;
            	if (IsFavBoard(i - 1)) {
                	move(2, 0);
                	prints("已存在该讨论区.");
                        clrtoeol();
                	pressreturn();
                	return SHOW_REFRESH;
            	}
                if (askyn("加入个人定制区？",0)!=1)
                	return SHOW_REFRESH;
            }
            if (i > 0 && !IsFavBoard(i - 1)) {
                addFavBoard(i - 1);
                save_favboard();
                arg->reloaddata=true;
            	if (BOARD_FAV == arg->yank_flag)
                	return SHOW_DIRCHANGE;
            	else
                	return SHOW_REFRESH;
            } else if (IsFavBoard(i - 1)) {
                move(2, 0);
                prints("已存在该讨论区.");
                clrtoeol();
                pressreturn();
                return SHOW_REFRESH;
            } else {
                move(2, 0);
                prints("不正确的讨论区.");
                clrtoeol();
                pressreturn();
                return SHOW_REFRESH;
            }
        }
        break;
    case 'A':                  /* added by bad 2002.8.3*/
        if (BOARD_FAV == arg->yank_flag) {
            char bname[STRLEN];
            int i = 0;

            if (favbrd_list_t >= FAVBOARDNUM) {
                move(2, 0);
                clrtoeol();
                prints("个人热门版数已经达上限(%d)！", FAVBOARDNUM);
                pressreturn();
                return SHOW_REFRESH;
            }
            move(0, 0);
            clrtoeol();
            getdata(0, 0, "输入讨论区目录名: ", bname, 22, DOECHO, NULL, true);
            if (bname[0]) {
                addFavBoardDir(i, bname);
                save_favboard();
                arg->reloaddata=true;
                return SHOW_DIRCHANGE;
            }
        }
        break;
    case 'T':                  /* added by bad 2002.8.3*/
        if (BOARD_FAV == arg->yank_flag) {
            char bname[STRLEN];
            int i = 0;

            if (ptr->dir == 1 && ptr->tag >= 0) {
                move(0, 0);
                clrtoeol();
                getdata(0, 0, "输入讨论区目录名: ", bname, 22, DOECHO, NULL, true);
                if (bname[0]) {
                    changeFavBoardDir(ptr->tag, bname);
                    save_favboard();
                    return SHOW_REFRESH;
                }
            }
        }
        break;
    case 'm':
        if (arg->yank_flag == BOARD_FAV) {
            if (currentuser->flags[0] & BRDSORT_FLAG) {
                move(0, 0);
                prints("排序模式下不能移动，请用'S'键切换!");
                pressreturn();
            } else {
                if (ptr->tag >= 0) {
                    int p, q;
                    char ans[5];

                    p = ptr->tag;
                    move(0, 0);
                    clrtoeol();
                    getdata(0, 0, "请输入移动到的位置:", ans, 4, DOECHO, NULL, true);
                    q = atoi(ans) - 1;
                    if (q < 0 || q >= conf->item_count) {
                        move(2, 0);
                        clrtoeol();
                        prints("非法的移动位置！");
                        pressreturn();
                    } else {
                        arg->fav_father=MoveFavBoard(p, q, arg->fav_father);
                        save_favboard();
                        arg->reloaddata=true;
                        return SHOW_DIRCHANGE;
                    }
                }
            }
            return SHOW_REFRESH;
        }
        break;
    case 'd':
        if (BOARD_FAV == arg->yank_flag) {
            int p = 1;

            if (ptr->tag < 0)
                p = 0;
            if (ptr->dir == 1 && p) {
                char ans[2];

                move(0, 0);
                clrtoeol();
                getdata(0, 0, "确认删除整个目录？(y/N)", ans, 2, DOECHO, NULL, true);
                p = ans[0] == 'Y' || ans[0] == 'y';
            }
            if (p) {
                DelFavBoard(ptr->tag);
                save_favboard();
                arg->fav_father=favnow;
                arg->reloaddata=true;
                return SHOW_DIRCHANGE;
            }
            return SHOW_REFRESH;
        }
    case 'y':
        if (arg->yank_flag < BOARD_FAV) {
                                /*--- Modified 4 FavBoard 2000-09-11	---*/
            arg->yank_flag = 1 - arg->yank_flag;
            arg->reloaddata=true;
            return SHOW_DIRCHANGE;
        }
        break;
    case 'z':                  /* Zap */
        if (arg->yank_flag < BOARD_FAV) {
                                /*--- Modified 4 FavBoard 2000-09-11	---*/
            if (HAS_PERM(currentuser, PERM_BASIC) && !(ptr->flag & BOARD_NOZAPFLAG)) {
                ptr->zap = !ptr->zap;
                ptr->total = -1;
                zapbuf[ptr->pos] = (ptr->zap ? 0 : login_start_time);
                zapbuf_changed = 1;
                return SHOW_REFRESHSELECT;
            }
        }
        break;
    case 'v':                  /*Haohmaru.2000.4.26 */
        if(!strcmp(currentuser->userid, "guest") || !HAS_PERM(currentuser, PERM_READMAIL)) return SHOW_CONTINUE;
        clear();
		if (HAS_MAILBOX_PROP(&uinfo, MBP_MAILBOXSHORTCUT))
			MailProc();
		else
        	m_read();
        return SHOW_REFRESH;
    }
    return SHOW_CONTINUE;
}

static void fav_refresh(struct _select_def *conf)
{
    struct favboard_proc_arg *arg = (struct favboard_proc_arg *) conf->arg;
    struct newpostdata *ptr;

    clear();
    ptr = &arg->nbrd[conf->pos - conf->page_pos];
    if (DEFINE(currentuser, DEF_HIGHCOLOR)) {
        if (arg->yank_flag == BOARD_FAV)
            docmdtitle("[个人定制区]",
                       "  [m主选单[\x1b[1;32m←\x1b[m,\x1b[1;32me\x1b[m] 阅读[\x1b[1;32m→\x1b[m,\x1b[1;32mr\x1b[m] 选择[\x1b[1;32m↑\x1b[m,\x1b[1;32m↓\x1b[m] 添加[\x1b[1;32ma\x1b[m,\x1b[1;32mA\x1b[m] 移动[\x1b[1;32mm\x1b[m] 删除[\x1b[1;32md\x1b[m] 排序[\x1b[1;32mS\x1b[m] 求助[\x1b[1;32mh\x1b[m]\n");
        else
            docmdtitle("[讨论区列表]",
                       "  [m主选单[\x1b[1;32m←\x1b[m,\x1b[1;32me\x1b[m] 阅读[\x1b[1;32m→\x1b[m,\x1b[1;32mr\x1b[m] 选择[\x1b[1;32m↑\x1b[m,\x1b[1;32m↓\x1b[m] 列出[\x1b[1;32my\x1b[m] 排序[\x1b[1;32mS\x1b[m] 搜寻[\x1b[1;32m/\x1b[m] 切换[\x1b[1;32mc\x1b[m] 求助[\x1b[1;32mh\x1b[m]\n");
        prints("[1;44m[37m  %s 讨论区名称       V  类别 转信  %-24s 版  主     [m\n", arg->newflag ? "全部 未读" : "编号 未读", "中  文  叙  述");
    } else {
        if (arg->yank_flag == BOARD_FAV)
            docmdtitle("[个人定制区]", "  [m主选单[←,e] 阅读[→,r] 选择[↑,↓] 添加[a,A] 移动[m] 删除[d] 排序[S] 求助[h]\n");
        else
            docmdtitle("[讨论区列表]", "  [m主选单[←,e] 阅读[→,r] 选择[↑,↓] 列出[y] 排序[S] 搜寻[/] 切换[c] 求助[h]\n");
        prints("[44m[37m  %s 讨论区名称        V 类别 转信  %-24s 版  主     [m\n", arg->newflag ? "全部 未读" : "编号 未读", "中  文  叙  述");
    }
    if (!arg->loop_mode)
        update_endline();
    else {
            move(t_lines - 1, 0);
            clrtoeol();
            prints("请输入要找寻的 board 名称：%s", arg->bname);
    }

}

static int fav_getdata(struct _select_def *conf, int pos, int len)
{
    struct favboard_proc_arg *arg = (struct favboard_proc_arg *) conf->arg;

    if (arg->reloaddata) {
    	arg->reloaddata=false;
    	if (arg->namelist) 	{
    		free(arg->namelist);
    		arg->namelist=NULL;
    	}
    }
    if (pos==-1) 
        fav_loaddata(NULL, arg->fav_father,1, conf->item_count,currentuser->flags[0] & BRDSORT_FLAG,arg->namelist);
    else
        conf->item_count = fav_loaddata(arg->nbrd, arg->fav_father,pos, len,currentuser->flags[0] & BRDSORT_FLAG,NULL);
    return SHOW_CONTINUE;
}

static int boards_getdata(struct _select_def *conf, int pos, int len)
{
    struct favboard_proc_arg *arg = (struct favboard_proc_arg *) conf->arg;

    if (arg->reloaddata) {
    	arg->reloaddata=false;
    	if (arg->namelist) 	{
    		free(arg->namelist);
    		arg->namelist=NULL;
    	}
    }
    if (pos==-1) 
         load_boards(NULL, arg->boardprefix,1, conf->item_count,currentuser->flags[0] & BRDSORT_FLAG,arg->yank_flag,arg->namelist);
    else
         conf->item_count = load_boards(arg->nbrd, arg->boardprefix,pos, len,currentuser->flags[0] & BRDSORT_FLAG,arg->yank_flag,NULL);
    return SHOW_CONTINUE;
}
int choose_board(int newflag, char *boardprefix,int favmode)
{
/* 选择 版， readnew或readboard */
    struct _select_def favboard_conf;
    struct favboard_proc_arg arg;
    POINT *pts;
    int i;
    int y;
    struct newpostdata *nbrd;
    int favlevel = 0;           /*当前层数 */
    int favlist[FAVBOARDNUM];
    int sellist[FAVBOARDNUM];   /*保存目录递归信息 */

    clear();
    pts = (POINT *) malloc(sizeof(POINT) * BBS_PAGESIZE);

    for (i = 0; i < BBS_PAGESIZE; i++) {
        pts[i].x = 1;
        pts[i].y = i + 3;
    };

    nbrd = (struct newpostdata *) malloc(sizeof(*nbrd) * BBS_PAGESIZE);
    arg.nbrd = nbrd;
    if (favmode) {
        arg.newflag = 1;
        arg.yank_flag = BOARD_FAV;
    } else {
        arg.newflag = newflag;
        arg.yank_flag = yank_flag;
    }
    sellist[0] = 1;
    favlist[0] = -1;
    arg.namelist=NULL;
    while (1) {
        bzero((char *) &favboard_conf, sizeof(struct _select_def));
        arg.tmpnum = 0;
        if (favmode) {
            arg.fav_father = favlist[favlevel];
            favnow = favlist[favlevel];
        } else {
            arg.boardprefix=boardprefix;
	 }
        arg.find = true;
        arg.loop_mode = 0;

        favboard_conf.item_per_page = BBS_PAGESIZE;
        favboard_conf.flag = LF_VSCROLL | LF_BELL | LF_LOOP | LF_MULTIPAGE;     /*|LF_HILIGHTSEL;*/
        favboard_conf.prompt = ">";
        favboard_conf.item_pos = pts;
        favboard_conf.arg = &arg;
        favboard_conf.title_pos.x = 0;
        favboard_conf.title_pos.y = 0;
        favboard_conf.pos = sellist[favlevel];
        favboard_conf.page_pos = ((sellist[favlevel]-1)/BBS_PAGESIZE)*BBS_PAGESIZE+1;
        if (arg.namelist) {
        	free(arg.namelist);
        	arg.namelist=NULL;
        }
        arg.reloaddata=false;

        if (favmode)        
            favboard_conf.get_data = fav_getdata;
        else
            favboard_conf.get_data = boards_getdata;
        (*favboard_conf.get_data)(&favboard_conf, favboard_conf.page_pos, BBS_PAGESIZE);
        if (favboard_conf.item_count==0)
            if (arg.yank_flag == BOARD_FAV || arg.yank_flag == BOARD_BOARDALL)
                break;
	    else {
                char ans[3];
                getdata(t_lines - 1, 0, "该讨论区组的版面已经被你全部取消了，是否查看所有讨论区？(Y/N)[N]", ans, 2, DOECHO, NULL, true);
                if (toupper(ans[0]) != 'Y')
                    break;
		arg.yank_flag=BOARD_BOARDALL;
                (*favboard_conf.get_data)(&favboard_conf, favboard_conf.page_pos, BBS_PAGESIZE);
                if (favboard_conf.item_count==0)
		    break;
	    }
        fav_gotonextnew(&favboard_conf);
        favboard_conf.on_select = fav_onselect;
        favboard_conf.show_data = fav_show;
        favboard_conf.pre_key_command = fav_prekey;
        favboard_conf.key_command = fav_key;
        favboard_conf.show_title = fav_refresh;

        update_endline();
        if (list_select_loop(&favboard_conf) == SHOW_QUIT) {
            /*退出一层目录*/
            favlevel--;
            if (favlevel == -1)
                break;
        } else {
            /*选择了一个目录,SHOW_SELECT*/
            sellist[favlevel] = favboard_conf.pos;
            favlevel++;
            if (favmode)
                favlist[favlevel] = nbrd[favboard_conf.pos - favboard_conf.page_pos].tag;
            else
                favlist[favlevel] = nbrd[favboard_conf.pos - favboard_conf.page_pos].pos;
            sellist[favlevel] = 1;
        };
        clear();
    }
    free(nbrd);
    free(pts);
    if (arg.namelist) {
    	free(arg.namelist);
    	arg.namelist=NULL;
    }
    save_zapbuf();
}

void FavBoard()
{
    if (favbrd_list_t == -1)
        load_favboard(1);
    choose_board(1, NULL,1);
}
