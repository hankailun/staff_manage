#include "client.h"
int net_init(client *info,char *ip,int port)//网络初始化
{
///{{{
	//创建流式套接字
    info->sfd =  socket(AF_INET, SOCK_STREAM, 0);
    if(info->sfd < 0)
    {
        ERR_MSG("socket");
        return -1;
    }

 
    //填充服务器的信息
    info->sin.sin_family      = AF_INET;
    info->sin.sin_port        = htons(port);
    info->sin.sin_addr.s_addr = inet_addr(ip);
    if(connect(info->sfd, (struct sockaddr*)&(info->sin), sizeof(info->sin)) <0)
    {
        ERR_MSG("connect");
        return -1;
    }
    
	return 0;
///}}}
}

sqlite3 *sql_init()//数据库初始化
{
///{{{
	sqlite3 *db = (sqlite3*)malloc(sizeof(db));
	if(NULL==db)
	{
		ERR_MSG("malloc");
		return NULL;
	}
	if(sqlite3_open("../server/staff.db",&db)!=0)
	{
		ERR_SQLMSG("sqlite3_open",db);
		return NULL;
	}
	return db;
///}}}
}

int mune1_select(pack *data)//一级菜单
{
	char mode[N]="";
    int ret_l;	
	while(1)
	{
		printf("---------------------\n");
		printf("------员工管理系统-----\n");
		printf("-------1.登录---------\n");
		printf("-------2.修改密码------\n");
        printf("-------3.退出---------\n");
		printf("---------------------\n");

		bzero(mode,N);
		printf("请选择模式>>");
		scanf("%s",mode);
		while(getchar()!=10);
		if(strlen(mode)>1)
		{
			printf("无效指令\n");
			continue;
		}

		switch(mode[0])
		{
		case '1'://登录
			ret_l = user_login(data);   //返回用户id
			if(ret_l > 0 )
				return ret_l;
			break;
        case '2'://修改密码
			passward_modify(data);
			break;
		case '3'://退出
			return 0;
		default:
			printf("无效的命令\n");
			break;
		}
		printf("按任意键清屏\n");
		getchar();
		system("clear");
	}

}

int user_login(pack *data)//用户登录 ok；>0 nok;<0
{
	pack buf;
	char passward[N]="";//密码
    int staffid;
	static int errnum=3;
	
	printf("请输入员工id>>");
	INT_INPUTCHECK(staffid);

	printf("请输入密码>>");
	scanf("%s",passward);
	while(getchar()!=10);

	//给服务器发送登录请求
	bzero(&buf, sizeof(buf));
	buf.mod = 2;
	buf.code.id = staffid;
	strcpy(buf.code.code,passward);
	ssize_t res_s = send(data->cliinfo.sfd,&buf,sizeof(buf),0);
	if(res_s<0)
	{
		ERR_MSG("send");
		return -1;
	}

	//接收登录请求响应
	bzero(&buf, sizeof(buf));
	ssize_t res = recv(data->cliinfo.sfd, &buf, sizeof(buf), 0);
	if(res < 0)
	{
		ERR_MSG("recv");
		return -1;
	}
	else if(0 == res)
	{
		printf("服务器关闭\n");
		return -1;
	}
	
	if(buf.mod!=20)
	{
		if(buf.mod == 21)
			printf("用户id不存在，请检查账号\n");
		else if(buf.mod == 22)
			printf("该账号在线，请检查账号或联系客服进行申诉\n");
		else if(buf.mod == 23)
		{
			errnum--;
			printf("密码错误，请重新输入,%d次失败后将锁定账号60s\n",errnum);
			if(errnum==0)
			{
				printf("客户端已锁定,请60s后重试\n");
				sleep(60);
				errnum =3;
			}
				
		}
		return -1;
	}
	errnum = 3;
	data->code.power = buf.code.power;

	return staffid;
///}}}
}

int passward_modify(pack *data)//用户密码修改
{
	pack buf;
	int staffid;//员工id
	char passward[16]="";//老密码
	char code[16]="";//新密码
	
	printf("请输入员工id>>");
	INT_INPUTCHECK(staffid);

	printf("请输入旧密码>>");
	scanf("%s",passward);
	while(getchar()!=10);

	printf("请输入新密码>>");
	scanf("%s",code);
	while(getchar()!=10);

	//给服务器发送注册请求
	bzero(&buf,sizeof(buf));
	buf.mod=13;
	buf.code.id=staffid;
	strcpy(buf.code.code,passward);
	strcpy(buf.code.newcode,code);
	ssize_t res_s = send(data->cliinfo.sfd,&buf,sizeof(buf),0);
	if(res_s<0)
	{
		ERR_MSG("send");
		return -1;
	}

	//接收注册请求响应
	bzero(&buf, sizeof(buf));
	ssize_t res = recv(data->cliinfo.sfd, &buf, sizeof(buf), 0);
	if(res < 0)
	{
		ERR_MSG("recv");
		return -1;
	}
	else if(0 == res)
	{
		printf("对方关闭\n");
		return -1;
	}

	if(buf.mod==130)//修改成功
	{
		printf("修改密码成功\n");
		return 0;
	}
	printf("修改密码失败\n");
  
	return -1;
}

int mune2_manage(pack *data,int id)//二级管理员菜单
{
	char mode[N]="";
	
    while(1)
    {
		printf("---------管理员-------\n");
		printf("-------1.员工信息查询---\n");
		printf("-------2.打卡----------\n");
		printf("-------3.考勤记录-------\n");
		printf("-------4.信息修改-------\n");
		printf("-------5.返回----------\n");
		printf("-------6.退出----------\n");
		printf("-----------------------\n");

		bzero(mode,N);
		printf("请选择功能>>");
		scanf("%s",mode);
		while(getchar()!=10);
		if(strlen(mode)>1)
		{
			printf("无效指令\n");
			continue;
		}

		switch(mode[0])
		{
		case '1'://员工信息查询
			message_seek(data,id);
			break;
		case '2'://打卡
			staff_check_on(data,id);
			break;
		case '3'://考勤记录
			manage_check_print(data,id);
			break;
		case '4'://信息修改
			manage_modify_message(data,id);
			break;
		case '5'://返回上一级菜单
			user_quit(data,id);
			//返回上一级菜单
			return 1;
		case '6'://退出
			return 0;
		default:
			printf("无效指令\n");
			break;
		}				
		printf("按任意键清屏\n");
		getchar();
		system("clear");
    }
	return 0;
}

int mune2_user(pack *data,int id)//二级普通用户菜单
{
	char mode[N]="";
	
    while(1)
    {
		printf("----------------------\n");
		printf("--------普通用户-------\n");
		printf("-------1.员工信息查询---\n");
		printf("-------2.打卡----------\n");
		printf("-------3.考勤记录-------\n");
		printf("-------4.信息修改-------\n");
		printf("-------5.返回----------\n");
		printf("-------6.退出----------\n");
		printf("-----------------------\n");

		bzero(mode,N);
		printf("请选择功能>>");
		scanf("%s",mode);
		while(getchar()!=10);
		if(strlen(mode)>1)
		{
			printf("无效指令\n");
			continue;
		}

		switch(mode[0])
		{
		case '1'://员工信息查询
			user_seek(data,id);
			break;
		case '2'://打卡
			staff_check_on(data,id);
			break;
		case '3'://考勤记录
			user_check_print(data,id);
			break;
		case '4'://信息修改
			user_modify_message(data,id);
			break;
		case '5'://返回上一级菜单
			user_quit(data,id);
			//返回上一级菜单
			return 1;
		case '6'://退出
			return 0;
		default:
			printf("无效指令\n");
			break;
		}				
		printf("按任意键清屏\n");
		getchar();
		system("clear");
    }
	return 0;
}

int staff_message_insert(pack *packed)//员工信息插入
{
	pack data = *packed;
	pack buf = *packed;

	printf("请输入职工姓名>>");
	scanf("%s",buf.staff.name);
	while(getchar()!=10);

	printf("请输入职工性别>>");
	scanf("%s",buf.staff.sex);
	while(getchar()!=10);

	printf("请输入职工年龄>>");
	INT_INPUTCHECK(buf.staff.age);

	printf("请输入职工薪资>>");
	INT_INPUTCHECK(buf.staff.salary);

	printf("请输入职工电话>>");
	scanf("%s",buf.staff.phone);
	while(getchar()!=10);

	printf("请输入职工家庭地址>>");
	scanf("%s",buf.staff.addr);
	while(getchar()!=10);

	//给服务器发送员工信息填充请求
	buf.mod=3;
	buf.staff.id = data.code.id;
	ssize_t res_s = send(data.cliinfo.sfd,&buf,sizeof(buf),0);
	if(res_s<0)
	{
		ERR_MSG("send");
		return -1;
	}

	//接收员工信息填充请求响应
	bzero(&buf, sizeof(buf));
	ssize_t res = recv(data.cliinfo.sfd, &buf, sizeof(buf),0);
	if(res < 0)
	{
		ERR_MSG("recv");
		return -1;
	}
	else if(0 == res)
	{
		printf("对方关闭\n");
		return -1;
	}

	if(buf.mod!=31)//出错
		return -1;

    printf("员工信息填充完毕\n");
	return 0;
}

int message_seek(pack *packed,int curid)//用户信息查询
{
	int seekid;

	printf("请输入员工ID>>");
	INT_INPUTCHECK(seekid);

	if(user_seek(packed,seekid))
		return -1;
	
	return 0;
}

int user_seek(pack *packed,int id)//用户信息查询
{
	pack data = *packed;
	pack buf;

	//给服务器发送查询请求
	bzero(&buf,sizeof(buf));
	buf.mod=4;
	buf.code.id=id;
	buf.cliinfo = data.cliinfo;
	ssize_t res_s = send(data.cliinfo.sfd,&buf,sizeof(buf),0);
	if(res_s<0)
	{
		ERR_MSG("send");
		return -1;
	}
	
	//接收服务器回馈信号
	bzero(&buf, sizeof(buf));
	ssize_t res = recv(data.cliinfo.sfd, &buf, sizeof(buf), 0);
	if(res < 0)
	{
		ERR_MSG("recv");
		return -1;
	}
	else if(0 == res)
	{
		printf("对方关闭\n");
		return -1;
	}

	if(buf.mod==41)//查询出错
	{
		printf(">>员工id错误<<\n");
		return -1;
	}

	//打印收信息
	printf("员工ID：%d\n",buf.staff.id);
	printf("员工姓名：%s\n",buf.staff.name);
	printf("员工性别：%s\n",buf.staff.sex);
	printf("年龄：%d\n",buf.staff.age);
	printf("薪资：%d\n",buf.staff.salary);
	printf("联系电话：%s\n",buf.staff.phone);
	printf("家庭住址：%s\n",buf.staff.sex);

	return 0;
}

int staff_check_on(pack *packed,int curid)//员工打卡
{
	time_t cur_t;
	pack data = *packed;
	pack buf;

	time(&cur_t);
	//给服务器发送打卡请求
	bzero(&buf,sizeof(buf));
	buf.mod=5;
	buf.code.id=curid;
	strcpy(buf.time,ctime(&cur_t));
	buf.cliinfo = data.cliinfo;
	ssize_t res_s = send(data.cliinfo.sfd,&buf,sizeof(buf),0);
	if(res_s<0)
	{
		ERR_MSG("send");
		return -1;
	}

	//接收服务器回馈信号
	bzero(&buf, sizeof(buf));
	ssize_t res = recv(data.cliinfo.sfd, &buf, sizeof(buf), 0);
	if(res < 0)
	{
		ERR_MSG("recv");
		return -1;
	}
	else if(0 == res)
	{
		printf("对方关闭\n");
		return -1;
	}

	if(buf.mod==50)//打卡出错
	{
		printf(">>服务器异常，打卡失败<<\n");
		return -1;
	}

	printf(">>打卡成功<<\n");
	return 0;

}

int manage_check_print(pack *packed,int curid)//@@管理员打印考勤记录
{
	pack data = *packed;
	pack buf;
	int seekid;
	int flag=1;

	printf("输入需要查询的员工id>>");
	INT_INPUTCHECK(seekid);
	
	//给服务器发送查询请求
	bzero(&buf,sizeof(buf));
	buf.mod=6;
	buf.code.id=seekid;
	buf.cliinfo = data.cliinfo;
	ssize_t res_s = send(data.cliinfo.sfd,&buf,sizeof(buf),0);
	if(res_s<0)
	{
		ERR_MSG("send");
		return -1;
	}

	while(1)
	{
		//接收服务器回馈信号
		bzero(&buf, sizeof(buf));
		ssize_t res = recv(data.cliinfo.sfd, &buf, sizeof(buf), 0);
		if(res < 0)
		{
			ERR_MSG("recv");
			return -1;
		}
		else if(0 == res)
		{
			printf("对方关闭\n");
			return -1;
		}

		if(buf.mod==61)//查询出错
		{
			printf(">>员工id错误<<\n");
			return -1;
		}
		else if(buf.mod == 60)//打印考勤
		{
			if(flag)
			{
				printf("员工ID\t员工姓名\t打卡时间\n");
				flag--;
			}
			printf("%d\t%s\t%s\n",buf.checklog.id,buf.checklog.name,buf.checklog.time);
		}
		else if(buf.mod == 62)//传输完毕
			break;
	}
	return 0;
}

int user_check_print(pack *packed,int curid)//普通用户打印考勤记录
{
	pack data = *packed;
	pack buf;
	int flag=1;
	
	//给服务器发送查询请求
	bzero(&buf,sizeof(buf));
	buf.mod=6;
	buf.code.id=curid;
	buf.cliinfo = data.cliinfo;
	ssize_t res_s = send(data.cliinfo.sfd,&buf,sizeof(buf),0);
	if(res_s<0)
	{
		ERR_MSG("send");
		return -1;
	}

	while(1)
	{
		//接收服务器回馈信号
		bzero(&buf, sizeof(buf));
		ssize_t res = recv(data.cliinfo.sfd, &buf, sizeof(buf), 0);
		if(res < 0)
		{
			ERR_MSG("recv");
			return -1;
		}
		else if(0 == res)
		{
			printf("对方关闭\n");
			return -1;
		}

		if(buf.mod==61)//查询出错
		{
			printf(">>员工id错误<<\n");
			return -1;
		}
		else if(buf.mod == 60)//打印考勤
		{
			if(flag)
			{
				printf("员工ID\t员工姓名\t打卡时间\n");
				flag--;
			}
			printf("%d\t%s\t%s\n",buf.checklog.id,buf.checklog.name,buf.checklog.time);
		}
		else if(buf.mod == 62)//传输完毕
			break;
	}
	return 0;
}

int manage_modify_message(pack *packed,int curid)//管理员信息修改
{
	int seekid;
	char mode[S]={0};

	printf("输入员工id>>");
	INT_INPUTCHECK(seekid);

	user_seek(packed,seekid);

	while(1)
    {
		printf("-----------------------\n");
		printf("-------0.返回上级目录----\n");
		printf("-------1.姓名----------\n");
		printf("-------2.性别----------\n");
		printf("-------3.年龄----------\n");
		printf("-------4.联系电话-------\n");
		printf("-------5.家庭住址-------\n");
		printf("-----------------------\n");

		bzero(mode,S);
		printf("请选择修改项>>");
		scanf("%s",mode);
		while(getchar()!=10);
		if(strlen(mode)>1)
		{
			printf("无效指令\n");
			continue;
		}

		switch(mode[0])
		{
		case '0'://返回
			return 0;
		case '1'://姓名
			modify_name(packed,seekid);
			break;
		case '2'://性别
			modify_sex(packed,seekid);
			break;
		case '3'://年龄
			modify_age(packed,seekid);
			break;
		case '4'://联系电话
			modify_phone(packed,seekid);
			break;
		case '5'://家庭住址
			modify_addr(packed,seekid);
			break;
		case '6'://薪资
			modify_salary(packed,seekid);
			break;
		default:
			printf("无效指令\n");
			break;
		}				
		printf("按任意键清屏\n");
		getchar();
		system("clear");
    }
	return 0;
}

int user_modify_message(pack *packed,int curid)//员工信息修改
{
	char mode[S]={0};
	user_seek(packed,curid);

	while(1)
    {
		printf("-----------------------\n");
		printf("-------0.返回上级目录----\n");
		printf("-------1.姓名----------\n");
		printf("-------2.性别----------\n");
		printf("-------3.年龄----------\n");
		printf("-------4.联系电话-------\n");
		printf("-------5.家庭住址-------\n");
		printf("-----------------------\n");

		bzero(mode,S);
		printf("请选择修改项>>");
		scanf("%s",mode);
		while(getchar()!=10);
		if(strlen(mode)>1)
		{
			printf("无效指令\n");
			continue;
		}

		switch(mode[0])
		{
		case '0'://返回
			return 0;
		case '1'://姓名
			modify_name(packed,curid);
			break;
		case '2'://性别
			modify_sex(packed,curid);
			break;
		case '3'://年龄
			modify_age(packed,curid);
			break;
		case '4'://联系电话
			modify_phone(packed,curid);
			break;
		case '5'://家庭住址
			modify_addr(packed,curid);
			break;
		case '6'://薪资
			modify_salary(packed,curid);
			break;
		default:
			printf("无效指令\n");
			break;
		}				
		printf("按任意键清屏\n");
		getchar();
		system("clear");
    }
	return 0;
}

int modify_name(pack *packed,int seekid)//员工姓名修改
{
	pack data = *packed;
	pack buf;
	char oldname[S]={};

	printf("输入新姓名>>");
	scanf("%s",oldname);
	while(getchar()!=10);


	//给服务器发送修改请求
	bzero(&buf,sizeof(buf));
	buf.mod=7;
	buf.staff.id=seekid;
	strcpy(buf.staff.name,oldname);
	buf.cliinfo = data.cliinfo;
	ssize_t res_s = send(data.cliinfo.sfd,&buf,sizeof(buf),0);
	if(res_s<0)
	{
		ERR_MSG("send");
		return -1;
	}

	//接收服务器回馈信号
	bzero(&buf, sizeof(buf));
	ssize_t res = recv(data.cliinfo.sfd, &buf, sizeof(buf), 0);
	if(res < 0)
	{
		ERR_MSG("recv");
		return -1;
	}
	else if(0 == res)
	{
		printf("对方关闭\n");
		return -1;
	}

	if(buf.mod==71)//查询出错
	{
		printf(">>员工姓名修改失败<<\n");
		return -1;
	}
	else if(buf.mod == 70)
		printf(">>员工姓名修改成功<<\n");

	return 0;
}

int modify_sex(pack *packed,int seekid)//员工性别修改
{
	pack data = *packed;
	pack buf;
	char sex[S]={};

	printf("输入新性别>>");
	scanf("%s",sex);
	while(getchar()!=10);


	//给服务器发送修改请求
	bzero(&buf,sizeof(buf));
	buf.mod=8;
	buf.staff.id=seekid;

	strcpy(buf.staff.sex,sex);
	buf.cliinfo = data.cliinfo;
	ssize_t res_s = send(data.cliinfo.sfd,&buf,sizeof(buf),0);
	if(res_s<0)
	{
		ERR_MSG("send");
		return -1;
	}

	//接收服务器回馈信号
	bzero(&buf, sizeof(buf));
	ssize_t res = recv(data.cliinfo.sfd, &buf, sizeof(buf), 0);
	if(res < 0)
	{
		ERR_MSG("recv");
		return -1;
	}
	else if(0 == res)
	{
		printf("对方关闭\n");
		return -1;
	}

	if(buf.mod==81)//查询出错
	{
		printf(">>员工性别修改失败<<\n");
		return -1;
	}
	else if(buf.mod == 80)
		printf(">>员工性别修改成功<<\n");

	return 0;
}

int modify_age(pack *packed,int seekid)//员工年龄修改
{
	pack data = *packed;
	pack buf;
	int age;

	printf("年龄修改为>>");
	INT_INPUTCHECK(age);


	//给服务器发送修改请求
	bzero(&buf,sizeof(buf));
	buf.mod=9;
	buf.staff.id=seekid;
	buf.staff.age = age;
	buf.cliinfo = data.cliinfo;
	ssize_t res_s = send(data.cliinfo.sfd,&buf,sizeof(buf),0);
	if(res_s<0)
	{
		ERR_MSG("send");
		return -1;
	}

	//接收服务器回馈信号
	bzero(&buf, sizeof(buf));
	ssize_t res = recv(data.cliinfo.sfd, &buf, sizeof(buf), 0);
	if(res < 0)
	{
		ERR_MSG("recv");
		return -1;
	}
	else if(0 == res)
	{
		printf("对方关闭\n");
		return -1;
	}

	if(buf.mod==91)//查询出错
	{
		printf(">>员工年龄修改失败<<\n");
		return -1;
	}
	else if(buf.mod == 90)
		printf(">>员工年龄修改成功<<\n");

	return 0;
}

int modify_phone(pack *packed,int seekid)//员工电话修改
{
	pack data = *packed;
	pack buf;
	char phone[S]={0};

	printf("联系电话修改为>>");
	scanf("%s",phone);
	while(getchar()!=10);


	//给服务器发送修改请求
	bzero(&buf,sizeof(buf));
	buf.mod=10;
	buf.staff.id=seekid;
	strcpy(buf.staff.phone,phone);
	buf.cliinfo = data.cliinfo;
	ssize_t res_s = send(data.cliinfo.sfd,&buf,sizeof(buf),0);
	if(res_s<0)
	{
		ERR_MSG("send");
		return -1;
	}

	//接收服务器回馈信号
	bzero(&buf, sizeof(buf));
	ssize_t res = recv(data.cliinfo.sfd, &buf, sizeof(buf), 0);
	if(res < 0)
	{
		ERR_MSG("recv");
		return -1;
	}
	else if(0 == res)
	{
		printf("对方关闭\n");
		return -1;
	}

	if(buf.mod==101)//查询出错
	{
		printf(">>员工联系电话修改失败<<\n");
		return -1;
	}
	else if(buf.mod == 100)
		printf(">>员工联系电话修改成功<<\n");

	return 0;
}

int modify_addr(pack *packed,int seekid)//员工地址修改
{
	pack data = *packed;
	pack buf;
	char addr[S]={0};

	printf("家庭住址修改为>>");
	scanf("%s",addr);
	while(getchar()!=10);


	//给服务器发送修改请求
	bzero(&buf,sizeof(buf));
	buf.mod=11;
	buf.staff.id=seekid;
	strcpy(buf.staff.addr,addr);
	buf.cliinfo = data.cliinfo;
	ssize_t res_s = send(data.cliinfo.sfd,&buf,sizeof(buf),0);
	if(res_s<0)
	{
		ERR_MSG("send");
		return -1;
	}

	//接收服务器回馈信号
	bzero(&buf, sizeof(buf));
	ssize_t res = recv(data.cliinfo.sfd, &buf, sizeof(buf), 0);
	if(res < 0)
	{
		ERR_MSG("recv");
		return -1;
	}
	else if(0 == res)
	{
		printf("对方关闭\n");
		return -1;
	}

	if(buf.mod==111)//查询出错
	{
		printf(">>员工家庭住址修改失败<<\n");
		return -1;
	}
	else if(buf.mod == 110)
		printf(">>员工家庭住址修改成功<<\n");

	return 0;
}

int modify_salary(pack *packed,int seekid)//员工薪资修改
{
	pack data = *packed;
	pack buf;
	int salary;

	printf("薪资修改为>>");
	INT_INPUTCHECK(salary);


	//给服务器发送修改请求
	bzero(&buf,sizeof(buf));
	buf.mod=12;
	buf.staff.id=seekid;
	buf.staff.salary = salary;
	buf.cliinfo = data.cliinfo;
	ssize_t res_s = send(data.cliinfo.sfd,&buf,sizeof(buf),0);
	if(res_s<0)
	{
		ERR_MSG("send");
		return -1;
	}

	//接收服务器回馈信号
	bzero(&buf, sizeof(buf));
	ssize_t res = recv(data.cliinfo.sfd, &buf, sizeof(buf), 0);
	if(res < 0)
	{
		ERR_MSG("recv");
		return -1;
	}
	else if(0 == res)
	{
		printf("对方关闭\n");
		return -1;
	}

	if(buf.mod==121)//查询出错
	{
		printf(">>员工薪资修改失败<<\n");
		return -1;
	}
	else if(buf.mod == 120)
		printf(">>员工薪资修改成功<<\n");

	return 0;
}

int user_quit(pack *packed,int seekid)//员工下线
{
	pack data = *packed;
	pack buf;

	buf.mod = 0;
	buf.cliinfo = data.cliinfo;
	ssize_t res_s = send(data.cliinfo.sfd,&buf,sizeof(buf),0);
	if(res_s<0)
	{
		ERR_MSG("send");
		return -1;
	}

	//接收服务器回馈信号
	bzero(&buf, sizeof(buf));
	ssize_t res = recv(data.cliinfo.sfd, &buf, sizeof(buf), 0);
	if(res < 0)
	{
		ERR_MSG("recv");
		return -1;
	}
	else if(0 == res)
	{
		printf(">>对方关闭<<\n");
		return -1;
	}

	if(buf.mod != 1)
	{
		printf(">>下线失败<<\n");
		return -1;
	}
	printf(">>下线成功<<\n");

	return 0;
}

