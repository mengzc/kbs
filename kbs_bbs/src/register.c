/*
    Pirate Bulletin Board System
    Copyright (C) 1990, Edward Luke, lush@Athena.EE.MsState.EDU
    Eagles Bulletin Board System
    Copyright (C) 1992, Raymond Rocker, rocker@rock.b11.ingr.com
                        Guy Vega, gtvega@seabass.st.usm.edu
                        Dominic Tynes, dbtynes@seabass.st.usm.edu

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

/*
    checked Global variable
*/
#include "bbs.h"

#define  EMAIL          0x0001
#define  NICK           0x0002
#define  REALNAME       0x0004
#define  ADDR           0x0008
#define  REALEMAIL      0x0010
#define  BADEMAIL       0x0020
#define  NEWREG         0x0040

char *sysconf_str();
char *Ctime();

extern struct user_info uinfo;
extern time_t login_start_time;
extern int convcode;            /* KCN,99.09.05 */
extern int switch_code();       /* KCN,99.09.05 */

void new_register()
{
    struct userec newuser;
    int allocid, do_try, flag;
    FILE* fn;
    char buf[STRLEN], fname[PATHLEN], title[STRLEN];

/* temp !!!!!*/
/*    prints("Sorry, we don't accept newusers due to system problem, we'll fixit ASAP\n");
    oflush();
    sleep(2);
    exit(-1);
*/
    memset(&newuser, 0, sizeof(newuser));
    getdata(0, 0, "使用GB编码阅读?(\xa8\xcf\xa5\xce BIG5\xbd\x58\xbe\x5c\xc5\xaa\xbd\xd0\xbf\xefN)(Y/N)? [Y]: ", buf, 4, DOECHO, NULL, true);
    if (*buf == 'n' || *buf == 'N')
        if (!convcode)
            switch_code();

    ansimore("etc/register", false);
    do_try = 0;
    while (1) {
        if (++do_try >= 10) {
            prints("\n掰掰，按太多下  <Enter> 了...\n");
            refresh();
            longjmp(byebye, -1);
        }
        getdata(0, 0, "请输入代号: ", newuser.userid, IDLEN + 1, DOECHO, NULL, true);
        flag = 1;
        if (id_invalid(newuser.userid) == 1) {
            prints("帐号必须由英文字母或数字组成，并且第一个字符必须是英文字母!\n");
            /*                prints("帐号必须由英文字母或数字，而且帐号第一个字是英文字母!\n"); */
            flag = 0;
        }
        if (flag) {
            if (strlen(newuser.userid) < 2) {
                prints("代号至少需有两个英文字母!\n");
            } else if ((*newuser.userid == '\0') || bad_user_id(newuser.userid)) {
                prints("系统用字或是不雅的代号。\n");
            } else if ((usernum = searchuser(newuser.userid)) != 0) {   /*( dosearchuser( newuser.userid ) ) midified by dong , 1998.12.2, change getuser -> searchuser , 1999.10.26 */
                prints("此帐号已经有人使用\n");
            } else {
                /*---	---*/
                struct stat lst;
                time_t lnow;

                lnow = time(NULL);
                sethomepath(genbuf, newuser.userid);
                if (!stat(genbuf, &lst) && S_ISDIR(lst.st_mode)
                    && (lnow - lst.st_ctime < SEC_DELETED_OLDHOME /* 3600*24*30 */ )) {
                    prints("目前无法注册帐号%s，请与系统管理人员联系。\n", newuser.userid);
                    sprintf(genbuf, "IP %s new id %s failed[home changed in past 30 days]", fromhost, newuser.userid);
                    bbslog("user","%s",genbuf);
                } else
                /*---	---*/
                    break;
            }
        }
    }

    newuser.firstlogin = newuser.lastlogin = time(NULL) - 13 * 60 * 24;
    do_try = 0;
    while (1) {
        char passbuf[STRLEN], passbuf2[STRLEN];

        if (++do_try >= 10) {
            prints("\n掰掰，按太多下  <Enter> 了...\n");
            refresh();
            longjmp(byebye, -1);
        }
        getdata(0, 0, "请设定您的密码: ", passbuf, 39, NOECHO, NULL, true);
        if (strlen(passbuf) < 4 || !strcmp(passbuf, newuser.userid)) {
            prints("密码太短或与使用者代号相同, 请重新输入\n");
            continue;
        }
        getdata(0, 0, "请再输入一次你的密码: ", passbuf2, 39, NOECHO, NULL, true);
        if (strcmp(passbuf, passbuf2) != 0) {
            prints("密码输入错误, 请重新输入密码.\n");
            continue;
        }

        setpasswd(passbuf, &newuser);
        break;
    }
    newuser.userlevel = PERM_BASIC;
    newuser.userdefine = -1;
/*   newuser.userdefine&=~DEF_MAILMSG;
    newuser.userdefine&=~DEF_EDITMSG; */
    newuser.userdefine &= ~DEF_NOTMSGFRIEND;
    if (convcode)
        newuser.userdefine &= ~DEF_USEGB;

    newuser.notemode = -1;
    newuser.exittime = time(NULL) - 100;
    /*newuser.unuse2 = -1;*/
    newuser.flags[0] = CURSOR_FLAG;
    newuser.flags[0] |= PAGER_FLAG;
    newuser.flags[1] = 0;
    newuser.firstlogin = newuser.lastlogin = time(NULL);

    allocid = getnewuserid2(newuser.userid);
    if (allocid > MAXUSERS || allocid <= 0) {
        prints("抱歉, 由于某些系统原因, 无法注册新的帐号.\n\r");
        oflush();
        sleep(2);
        exit(1);
    }
    newbbslog(BBSLOG_USIES, "APPLY: uid %d from %s", allocid, fromhost);

    update_user(&newuser, allocid, 1);

    if (!dosearchuser(newuser.userid)) {
        /* change by KCN 1999.09.08
           fprintf(stderr,"User failed to create\n") ;
         */
        prints("User failed to create %d-%s\n", allocid, newuser.userid);
        oflush();
        exit(1);
    }
    bbslog("user","%s","new account");
}

/*加入对 #TH 结尾的realemail的帐号自动通过注册的功能  by binxun
*/
int invalid_realmail(userid, email, msize)
    char *userid, *email;
    int msize;
{
    FILE *fn;
    char *emailfile, ans[4], fname[STRLEN];
    char genbuf[STRLEN];
    struct userec* uc;
    time_t now;
    int len = strlen(email);

    if ((emailfile = sysconf_str("EMAILFILE")) == NULL)
        return 0;

    if (strchr(email, '@') && valid_ident(email))
        return 0;
    /*
       ansimore( emailfile, false );
       getdata(t_lines-1,0,"您要现在 email-post 吗? (Y/N) [Y]: ",
       ans,2,DOECHO,NULL,true);
       while( *ans != 'n' && *ans != 'N' ) {
     */
    sprintf(fname, "tmp/email/%s", userid);
    if ((fn = fopen(fname, "r")) != NULL) {
        fgets(genbuf, STRLEN, fn);
        fclose(fn);
        strtok(genbuf, "\n");
        if (!valid_ident(genbuf)) {
        } else if (strchr(genbuf, '@') != NULL) {
            unlink(fname);
            strncpy(email, genbuf, msize);
            move(10, 0);
            prints("恭贺您!! 您已通过身份验证, 成为本站公民. \n");
            prints("         本站为您所提供的额外服务, \n");
            prints("         包括Mail,Post,Message,Talk 等. \n");
            prints("  \n");
            prints("建议您,  先四处浏览一下, \n");
            prints("         不懂的地方, 请在 sysop 版留言, \n");
            prints("         本站会派专人为您解答. \n");
            getdata(18, 0, "请按 <Enter>  <<  ", ans, 2, DOECHO, NULL, true);
            return 0;
        }
    }

#ifdef SMTH

    if(len >= 3)
    {
    	strncpy(genbuf,email+strlen(email)-3,3);
	if(!strncasecmp(genbuf,"#TH",3))
	{
		getuser(userid,&uc);
		// > 3 days
		now = time(NULL);
		if(now - uc->firstlogin >= REGISTER_WAIT_TIME)
		{
	 		if(auto_register(userid,email,msize) < 0) // 完成自动注册
				return 1;
			else
				return 0;     //success
		}
	}
    }
#endif
    return 1;
}

int auto_register(char* userid,char* email,int msize)
{
	struct userdata ud;
	struct userec* uc;
	char* item,*temp;
	char fdata[7][STRLEN];
	char genbuf[STRLEN];
	char buf[STRLEN];
	char fname[STRLEN];
	int unum;
	FILE* fout;
	int n;
	struct userec deliveruser;

	bzero(&deliveruser,sizeof(struct userec));
	strcpy(deliveruser.userid,"deliver");
	deliveruser.userlevel = -1;
	strcpy(deliveruser.username,"自动发信系统");


	static const char *finfo[] = { "帐号位置", "申请代号", "真实姓名", "服务单位",
        "目前住址", "连络电话", "生    日", NULL
    };
  	static const char *field[] = { "usernum", "userid", "realname", "career",
    	"addr", "phone", "birth", NULL
	};

	bzero(fdata,7*STRLEN);

	if((unum = getuser(userid,&uc)) == 0)return -1;//faild
	if(read_userdata(userid,&ud) < 0)return -1;

	strncpy(genbuf,email,STRLEN - 16);
	item =strtok(genbuf,"#");
	if(item)strncpy(ud.realname,item,NAMELEN);
	item = strtok(NULL,"#");  //学号
	item = strtok(NULL,"#");
	if(item)strncpy(ud.address,item,STRLEN);

	email[strlen(email) - 3] = '@';
	strncpy(ud.realemail,email,STRLEN-16); //email length must be less STRLEN-16


	sprintf(fdata[0],"%d",unum);
	strncpy(fdata[2],ud.realname,NAMELEN);
	strncpy(fdata[4],ud.address,STRLEN);
	strncpy(fdata[5],ud.email,STRLEN);
	strncpy(fdata[1],userid,IDLEN);

	sprintf(buf,"tmp/email/%s",userid);
	if ((fout = fopen(buf,"w")) != NULL)
	{
		fprintf(fout,"%s\n",email);
		fclose(fout);
	}

	if(write_userdata(userid,&ud) < 0)return -1;
	mail_file("deliver","etc/s_fill",userid,"恭喜你,你已经完成注册.",0,0);
	//sprintf(genbuf,"deliver 让 %s 自动通过身份确认.",uinfo.userid);

	sprintf(fname, "tmp/security.%d", getpid());
	if ((fout = fopen(fname, "w")) != NULL)
	{
		fprintf(fout, "系统安全记录系统\n[32m原因：%s自动通过注册[m\n", userid);
                fprintf(fout, "以下是通过者个人资料");
                fprintf(fout, "\n\n您的代号     : %s\n", ud.userid);
                fprintf(fout, "您的昵称     : %s\n", uc->username);
                fprintf(fout, "真实姓名     : %s\n", ud.realname);
                fprintf(fout, "电子邮件信箱 : %s\n", ud.email);
                fprintf(fout, "真实 E-mail  : %s\n", ud.realemail);
                fprintf(fout, "服务单位     : %s\n", "");
                fprintf(fout, "目前住址     : %s\n", ud.address);
                fprintf(fout, "连络电话     : %s\n", "");
                fprintf(fout, "注册日期     : %s", ctime(&uc->firstlogin));
                fprintf(fout, "最近光临日期 : %s", ctime(&uc->lastlogin));
                fprintf(fout, "最近光临机器 : %s\n", uc->lasthost);
                fprintf(fout, "上站次数     : %d 次\n", uc->numlogins);
                fprintf(fout, "文章数目     : %d(Board)\n", uc->numposts);
                fprintf(fout, "生    日     : %s\n", "");

                fclose(fout);
                //post_file(currentuser, "", fname, "Registry", str, 0, 2);

		sprintf(genbuf,"%s 自动通过注册",ud.userid);
		post_file(&deliveruser,"",fname,"Registry",genbuf,0,1);
	/*if (( fout = fopen(logfile,"a")) != NULL)
	{
		fclose(fout);
	}*/
	}

	sethomefile(buf, userid, "/register");
	if ((fout = fopen(buf, "w")) != NULL) {
		for (n = 0; field[n] != NULL; n++)
			fprintf(fout, "%s     : %s\n", finfo[n], fdata[n]);
		fprintf(fout, "您的昵称     : %s\n", uc->username);
		fprintf(fout, "电子邮件信箱 : %s\n", ud.email);
		fprintf(fout, "真实 E-mail  : %s\n", ud.realemail);
		fprintf(fout, "注册日期     : %s\n", ctime(&uc->firstlogin));
		fprintf(fout, "注册时的机器 : %s\n", uc->lasthost);
		fprintf(fout, "Approved: %s\n", userid);
		fclose(fout);
	}

	return 0;
}

void check_register_info()
{
    char *newregfile;
    int perm;
    char buf[STRLEN];
	struct userdata ud;

    clear();
    sprintf(buf, "%s", email_domain());
    if (!(currentuser->userlevel & PERM_BASIC)) {
        currentuser->userlevel = PERM_DENYMAIL|PERM_DENYRELAX;
        return;
    }
    /*urec->userlevel |= PERM_DEFAULT; */
    perm = PERM_DEFAULT & sysconf_eval("AUTOSET_PERM",PERM_DEFAULT);
    read_userdata(currentuser->userid, &ud);

    invalid_realmail(currentuser->userid,ud.realemail,STRLEN - 16);

    read_userdata(currentuser->userid,&ud);

    /*    if( sysconf_str( "IDENTFILE" ) != NULL ) {  commented out by netty to save time */
    while (strlen(currentuser->username) < 2) {
        getdata(2, 0, "请输入您的昵称:(例如," DEFAULT_NICK ") << ", buf, NAMELEN, DOECHO, NULL, true);
        strcpy(currentuser->username, buf);
        strcpy(uinfo.username, buf);
        UPDATE_UTMP_STR(username, uinfo);
    }
    if (strlen(ud.realname) < 2) {
        move(3, 0);
        prints("请输入您的真实姓名: (站长会帮您保密的 !)\n");
        getdata(4, 0, "> ", buf, NAMELEN, DOECHO, NULL, true);
        strcpy(ud.realname, buf);
    }
    if (strlen(ud.address) < 6) {
        move(5, 0);
        prints("您目前填写的地址是‘%s’，长度小于 [1m[37m6[m，系统认为其过于简短。\n", ud.address[0] ? ud.address : "空地址");  /* Leeward 98.04.26 */
        getdata(6, 0, "请详细填写您的住址：", buf, NAMELEN, DOECHO, NULL, true);
        strcpy(ud.address, buf);
    }
    if (strchr(ud.email, '@') == NULL) {
        move(3, 0);
        prints("只有本站的合法公民才能够完全享有各种功能， \n");
        /* alex           prints( "成为本站合法公民有两种办法：\n\n" );
           prints( "1. 如果你有合法的email信箱(非BBS), \n");
           prints( "       你可以用回认证信的方式来通过认证。 \n\n" );
           prints( "2. 如果你没有email信箱(非BBS)，你可以在进入本站以后，\n" );
           prints( "       在'个人工具箱'内 详细注册真实身份，( 主菜单  -->  I) 个人工具箱  -->  F) 填写注册单 )\n" );
           prints( "       SYSOPs 会尽快 检查并确认你的注册单。\n" );
           move( 17, 0 );
           prints( "电子信箱格式为: xxx@xxx.xxx.edu.cn \n" );
           getdata( 18, 0, "请输入电子信箱: (不能提供者按 <Enter>) << "
           , urec->email, STRLEN,DOECHO,NULL,true);
           if ((strchr( urec->email, '@' ) == NULL )) {
           sprintf( genbuf, "%s.bbs@%s", urec->userid,buf );
           strncpy( urec->email, genbuf, STRLEN);
           }
           alex, 因为取消了email功能 , 97.7 */
        prints("成为" NAME_BBS_NICK "合法" NAME_USER_SHORT "的方法如下：\n\n");
        prints("你的帐号的第一次登录后的 " REGISTER_WAIT_TIME_NAME "内（[1m[33m不是指上 BBS " REGISTER_WAIT_TIME_NAME "[m），\n");
        prints("    你处于新手上路期间, 请四处参观学习网络使用方法和各种礼仪。\n");
        prints("    在此期间，不能注册成为合法" NAME_USER_SHORT "。\n\n");
        prints("过了这开始的 " REGISTER_WAIT_TIME_NAME ", 你就可以在进入" NAME_BBS_NICK "以后，\n");
        prints("    在‘个人工具箱’内详细注册真实身份，\n");
        prints("    " NAME_SYSOP_GROUP "会尽快检查并确认你的注册单。\n\n");
        /* Leeward adds below 98.04.26 */
        prints("[1m[33m如果你已经通过了注册，成为了合法" NAME_USER_SHORT
               "，却依然还是看到了本信息，\n那是因为你没有在‘个人工具箱’内设定‘电子邮件信箱’。[m\n请从‘主选单’进入‘个人工具箱’内，再进入‘设定个人资料’一项进行设定。\n如果你实在没有任何可用的‘电子邮件信箱’可以设定，又不愿意看到本信息，\n可以使用 [1m[33m%s.bbs@smth.org[m 进行设定。\n注意∶上面给出的电子邮件信箱不能接收电子邮件，仅仅是用来使系统不再显示本信息。",
               currentuser->userid);
        pressreturn();
    }
#ifdef HAVE_BIRTHDAY
	if (currentuser->lastlogin - currentuser->firstlogin < 3 * 86400)
	{
		if (currentuser->numlogins == 1)
		{
			time_t now;
			struct tm *tmnow;
			FILE *fout;
			char buf2[STRLEN];

			now = time(0);
			tmnow = localtime(&now);
			clear();
			buf[0] = '\0';
			while (buf[0] < '1' || buf[0] > '2')
			{
				getdata(2, 0, "请输入你的性别: [1]男的 [2]女的 (1 or 2): ",
						buf, 2, DOECHO, NULL, true);
			}
			switch (buf[0])
			{
			case '1':
				currentuser->gender = 'M';
				break;
			case '2':
				currentuser->gender = 'F';
				break;
			}
			move(4, 0);
			prints("请输入您的出生日期");
			while (currentuser->birthyear < tmnow->tm_year - 98
				   || currentuser->birthyear > tmnow->tm_year - 3)
			{
				buf[0] = '\0';
				getdata(5, 0, "四位数公元年: ", buf, 5, DOECHO, NULL, true);
				if (atoi(buf) < 1900)
					continue;
				currentuser->birthyear = atoi(buf) - 1900;
			}
			while (currentuser->birthmonth < 1 || currentuser->birthmonth > 12)
			{
				buf[0] = '\0';
				getdata(6, 0, "出生月: (1-12) ", buf, 3, DOECHO, NULL, true);
				currentuser->birthmonth = atoi(buf);
			}
			do
			{
				buf[0] = '\0';
				getdata(7, 0, "出生日: (1-31) ", buf, 3, DOECHO, NULL, true);
				currentuser->birthday = atoi(buf);
			} while (!is_valid_date(currentuser->birthyear + 1900,
						currentuser->birthmonth,
						currentuser->birthday));
			sprintf(buf, "tmp/newcomer.%s", currentuser->userid);
			if ((fout = fopen(buf, "w")) != NULL)
			{
				fprintf(fout, "大家好,\n\n");
				fprintf(fout, "我是 %s (%s), 来自 %s\n", currentuser->userid,
						currentuser->username, fromhost);
				fprintf(fout, "今天%s初来此站报到, 请大家多多指教。\n",
						(currentuser->gender == 'M') ? "小弟" : "小女子");
				move(9, 0);
				prints("请作个简短的个人简介, 向本站其他使用者打个招呼\n");
				prints("(最多三行, 写完可直接按 <Enter> 跳离)....");
				getdata(11, 0, ":", buf2, 75, DOECHO, NULL, true);
				if (buf2[0] != '\0')
				{
					fprintf(fout, "\n\n自我介绍:\n\n");
					fprintf(fout, "%s\n", buf2);
					getdata(12, 0, ":", buf2, 75, DOECHO, NULL, true);
					if (buf2[0] != '\0')
					{
						fprintf(fout, "%s\n", buf2);
						getdata(13, 0, ":", buf2, 75, DOECHO, NULL, true);
						if (buf2[0] != '\0')
						{
							fprintf(fout, "%s\n", buf2);
						}
					}
				}
				fclose(fout);
				sprintf(buf2, "新手上路: %s", currentuser->username);
				post_file(currentuser, "", buf, "newcomers", buf2, 0, 2);
				unlink(buf);
			}
			pressanykey();
		}
	}
#endif
#ifdef NINE_BUILD
    if (currentuser->numlogins == 1)
    {
        time_t now;
        struct tm *tmnow;
        FILE *fout;
        char buf2[STRLEN];
	clear();
	sprintf(buf, "tmp/newcomer.%s", currentuser->userid);
	if ((fout = fopen(buf, "w")) != NULL)
	{
		fprintf(fout, "大家好,\n\n");
		fprintf(fout, "我是 %s (%s), 来自 %s\n", currentuser->userid,
				currentuser->username, fromhost);
		fprintf(fout, "今天%s初来此站报到, 请大家多多指教。\n",
				"小弟");
		move(9, 0);
		prints("请作个简短的个人简介, 向本站其他使用者打个招呼\n");
		prints("(最多三行, 写完可直接按 <Enter> 跳离)....");
		getdata(11, 0, ":", buf2, 75, DOECHO, NULL, true);
		if (buf2[0] != '\0')
		{
			fprintf(fout, "\n\n自我介绍:\n\n");
			fprintf(fout, "%s\n", buf2);
			getdata(12, 0, ":", buf2, 75, DOECHO, NULL, true);
			if (buf2[0] != '\0')
			{
				fprintf(fout, "%s\n", buf2);
				getdata(13, 0, ":", buf2, 75, DOECHO, NULL, true);
				if (buf2[0] != '\0')
				{
					fprintf(fout, "%s\n", buf2);
				}
			}
		}
		fclose(fout);
		sprintf(buf2, "新手上路: %s", currentuser->username);
		post_file(currentuser, "", buf, "newcomers", buf2, 0, 2);
		unlink(buf);
    }
    	}
#endif
    if (!strcmp(currentuser->userid, "SYSOP")) {
        currentuser->userlevel = ~0;
        currentuser->userlevel &= ~PERM_SUICIDE;        /* Leeward 98.10.13 */
        currentuser->userlevel &= ~(PERM_DENYMAIL|PERM_DENYRELAX);       /* Bigman 2000.9.22 */
        currentuser->userlevel &= ~PERM_JURY;       /* 不能是仲裁 */
    }
    if (!(currentuser->userlevel & PERM_LOGINOK)) {
        if (HAS_PERM(currentuser, PERM_SYSOP))
            return;
        if (!invalid_realmail(currentuser->userid, ud.realemail, STRLEN - 16)) {
            currentuser->userlevel |= PERM_DEFAULT;
            /*
            if (HAS_PERM(currentuser, PERM_DENYPOST) && !HAS_PERM(currentuser, PERM_SYSOP))
                currentuser->userlevel &= ~PERM_POST;
            */
        } else {
            /* added by netty to automatically send a mail to new user. */
            /* begin of check if local email-addr  */
            /*       if (
               (!strstr( urec->email, "@bbs.") ) &&
               (!strstr( urec->email, ".bbs@") )&&
               (!invalidaddr(urec->email))&&
               sysconf_str( "EMAILFILE" )!=NULL) 
               {
               move( 15, 0 );
               prints( "您的电子信箱  尚须通过回信验证...  \n" );
               prints( "      SYSOP 将寄一封验证信给您,\n" );
               prints( "      您只要回信, 就可以成为本站合格公民.\n" );
               getdata( 19 ,0, "您要 SYSOP 寄这一封信吗?(Y/N) [Y] << ", ans,2,DOECHO,NULL,true);
               if ( *ans != 'n' && *ans != 'N' ) {
               code=(time(0)/2)+(rand()/10);
               sethomefile(genbuf,urec->userid,"mailcheck");
               if((dp=fopen(genbuf,"w"))==NULL)
               {
               fclose(dp);
               return;
               }
               fprintf(dp,"%9.9d\n",code);
               fclose(dp);
               sprintf( genbuf, "/usr/lib/sendmail -f SYSOP.bbs@%s %s ", 
               email_domain(), urec->email );
               fout = popen( genbuf, "w" );
               fin  = fopen( sysconf_str( "EMAILFILE" ), "r" );
               if ((fin != NULL) && (fout != NULL)) {
               fprintf( fout, "Reply-To: SYSOP.bbs@%s\n", email_domain());
               fprintf( fout, "From: SYSOP.bbs@%s\n",  email_domain() ); 
               fprintf( fout, "To: %s\n", urec->email);
               fprintf( fout, "Subject: @%s@[-%9.9d-]firebird mail check.\n", urec->userid ,code);
               fprintf( fout, "X-Forwarded-By: SYSOP \n" );
               fprintf( fout, "X-Disclaimer: None\n");
               fprintf( fout, "\n");
               fprintf(fout,"您的基本资料如下：\n",urec->userid);
               fprintf(fout,"使用者代号：%s (%s)\n",urec->userid,urec->username);
               fprintf(fout,"姓      名：%s\n",urec->realname);
               fprintf(fout,"上站位置  ：%s\n",urec->lasthost);
               fprintf(fout,"电子邮件  ：%s\n\n",urec->email);
               fprintf(fout,"亲爱的 %s(%s):\n",urec->userid,urec->username);
               while (fgets( genbuf, 255, fin ) != NULL ) {
               if (genbuf[0] == '.' && genbuf[ 1 ] == '\n')
               fputs( ". \n", fout );
               else fputs( genbuf, fout );
               }
               fprintf(fout, ".\n");                                    
               fclose( fin );
               fclose( fout );                                     
               }
               getdata( 20 ,0, "信已寄出, SYSOP 将等您回信哦!! 请按 <Enter> << ", ans,2,DOECHO,NULL ,true);
               }
               }else
               {
               showansi=1;
               if(sysconf_str( "EMAILFILE" )!=NULL)
               {
               prints("\n你的电子邮件地址 【[33m%s[m】\n",urec->email);
               prints("并非 Unix 帐号，系统不会投递身份确认信，请到[32m工具箱[m中修改..\n");
               pressanykey();
               }
               }
               deleted by alex, remove email certify */

            clear();            /* Leeward 98.05.14 */
            move(12, 0);
            prints("你还没有通过身份认证，将会没有talk,mail,message,post权...  \n");
            prints("如果你要成为" NAME_BBS_NICK "的注册" NAME_USER_SHORT "，\n\n");
            prints("请在[31m个人工具箱[m内[31m详细注册身份[m\n");
            prints("您只要确实详细填写了你的真实身份资料, \n");
            prints("在" NAME_SYSOP_GROUP "替你手工认证了以后，就可以成为本站合格" NAME_USER_SHORT ".\n");
            prints("\n主菜单  -->  I) 个人工具箱  -->  F) 填写注册单\n");
            pressreturn();
        }
        /* end of check if local email-addr */
        /*  above lines added by netty...  */
    }
	write_userdata(currentuser->userid, &ud);
    newregfile = sysconf_str("NEWREGFILE");
    if (currentuser->lastlogin - currentuser->firstlogin < REGISTER_WAIT_TIME && !HAS_PERM(currentuser, PERM_SYSOP) && newregfile != NULL) {
        currentuser->userlevel &= ~(perm);
        ansimore(newregfile, true);
    }
}
