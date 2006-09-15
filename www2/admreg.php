<?php
    require("www2-admin.php");
   
    if(!($currentuser["userlevel"] & BBS_PERM_ACCOUNTS))
        admin_deny();
   
    admin_header("批注册单", "设定使用者注册资料");

    $reglist = array();
    $count = bbs_admin_getnewreg($reglist);

    if($count == 0) {
        print("目前没有需要审批的注册单。");
    }
    else {
        print("共有 {$count} 个用户等待审批。");
        print("<table align=\"center\" cellpadding=\"3\" border=\"1\"><tr><th>编号和时间</th><th>用户名</th><th>真实姓名</th><th>服务单位</th></tr>");
        for($i=0; $i<$count; $i++) {
            print("<tr><td>{$reglist[$i]["usernum"]}</td><td>{$reglist[$i]["userid"]}</td><td>{$reglist[$i]["realname"]}</td><td>{$reglist[$i]["career"]}</td></tr>");
        }
        print("</table><br>");
    }

    page_footer();
?>
