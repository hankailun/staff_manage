#include "server.h"

int net_init(char *ip,int port)//网络初始化
{
    int sfd;
    sfd = socket(AF_INET,SOCK_STREAM,0);
    if(sfd < 0)
    {
        ERR_MSG("socket");
        return -1;
    }

    //允许地址快速重用
    int value = 1;
    if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) < 0)
    {
        ERR_MSG("setsockopt");
        return -1;
    }

    //绑定服务器的ip和端口
    struct sockaddr_in sin;
    sin.sin_family      = AF_INET;
    sin.sin_port        = htons(port);
    sin.sin_addr.s_addr = inet_addr(ip);
    if(bind(sfd, (struct sockaddr*)&sin, sizeof(sin)) < 0)
    {
        ERR_MSG("bind");
        return -1;
    }

    //将套接字文件描述符设置为被动监听状态
    if(listen(sfd, 5) <0)
    {
        ERR_MSG("listen");
        return -1;
    }

    return sfd;
}

sqlite3 * sql_init()//数据库初始化
{
    sqlite3 *db=(sqlite3 *)malloc(sizeof(db));
    char *errmsg = NULL;
	char cmd[N]="";
	char cmd1[N]="";
	char cmd2[N]="";
	int ret_sq;

    //创建数据库
	if(sqlite3_open("./staff.db",&db)!=0)
	{
		ERR_SQLMSG("sqlite3_open",db);
		return NULL;
	}

    //创建员工账号表
	strcpy(cmd,"create table if not exists staff_passward(staffid int primary key ,\
            passward char,power int,state int)");
	ret_sq = sqlite3_exec(db,cmd,NULL,NULL,&errmsg);
	if(ret_sq!=0)
	{
		ERR_SQLMSG("sqlite3_exec_staff_message",db);
		return NULL;
	}

    //创建员工信息表
	strcpy(cmd1,"create table if not exists staff_message(staffid int primary key ,\
           staffname char,sex char,age int,salary int,phone long,addr char)");
	ret_sq = sqlite3_exec(db,cmd1,NULL,NULL,&errmsg);
	if(ret_sq!=0)
	{
		ERR_SQLMSG("sqlite3_exec_staff_message",db);
		return NULL;
	}

     //创建员工打卡记录表
	strcpy(cmd2,"create table if not exists staff_check(staffid int ,\
            staffname char,time char)");
	ret_sq = sqlite3_exec(db,cmd2,NULL,NULL,&errmsg);
	if(ret_sq!=0)
	{
		ERR_SQLMSG("sqlite3_exec_check_on",db);
		return NULL;
	}
	//初始化账号状态
	passward_init(db);
	
    return db;
}

int poll_client(int sfd,sqlite3 *db)//监听服务器事件
{
    int newfd ;//新连接客户端通讯的套接字
	int cmd = 0;//服务器命令


	//定义接收客户端的网络配置
    struct sockaddr_in cin;
    socklen_t addrlen = sizeof(cin);

	//定义集合结构体数组
    struct pollfd fds[2];

    //把需要监听的文件描述符以及事件添加到结构体中
    fds[0].fd = 0;
    fds[0].events = POLLIN;         //监听读事件

    fds[1].fd = sfd;                //监听sfd事件,链接客户端
    fds[1].events = POLLIN;

	sermsg cliInfo;
	printf("-------------------\n");
	printf("--------服务器------\n");
	printf("-------1.注册------\n");
	printf("-------2.修改权限---\n");
	printf("-------3.删除用户---\n");
	printf("-------4.退出------\n");
	printf("-------------------\n");


	

    while(1)
    {
		int ret = poll(fds, 2, -1);
        if(ret < 0)
        {
            ERR_MSG("poll");
            return -1;
        }
        else if(0 == ret)
        {
            fprintf(stderr, "超时了\n");
            continue;
        }

		if(fds[1].revents & POLLIN)//客户端链接，建立新线程
		{
			newfd  = accept(sfd, (struct sockaddr*)&cin, &addrlen);
			if(newfd < 0)
			{
				ERR_MSG("accept");
				return -1;
			}


			//printf("[%s:%d]链接成功\n", inet_ntoa(cin.sin_addr), ntohs(cin.sin_port));

			//线程传入参数
			
			cliInfo.newfd = newfd;
			cliInfo.cin = cin;
			cliInfo.db = db;

			//创建一个线程维护客户端
			pthread_t tid;
			if(pthread_create(&tid, NULL, recv_cli_msg, (void*)&cliInfo) != 0)
			{
				ERR_MSG("pthread_create");
				return -1;
			}
		}

		if(fds[0].revents & POLLIN)//数据输入
		{
			
			scanf("%d",&cmd);
			while(getchar()!=10);

			switch(cmd)
			{
			case 1://注册
				user_register(db);
				break;
			case 2://修改权限
				power_menu(db);
				break;
			case 3://删除用户
				user_delete(db);
				break;
			case 4://退出
				return 0;
			default:
				printf("无效的命令，请重新输入\n");
				break;
			}
			printf("按任意键清屏\n");
			getchar();
			system("clear");
			printf("-------------------------\n");
			printf("------员工管理系统服务器----\n");
			printf("---------1.注册-----------\n");
			printf("---------3.删除用户--------\n");
			printf("---------4.退出------------\n");
			printf("--------------------------\n");
		}

    }
    return 0;
}

void* recv_cli_msg(void* arg)//与客户端交互线程
{
	ssize_t res;
    pack buf;
	int userid=0;
    //分离线程，线程退出后自动回收资源;
    pthread_detach(pthread_self());

	//线程传参
    sermsg cliInfo = *(sermsg*)arg;
    int newfd = cliInfo.newfd;
    struct sockaddr_in cin = cliInfo.cin;
	//data.serinfo = cliInfo;

   	
    while(1)
    {   
        bzero(&buf, sizeof(buf));
        //循环接受客户端的信息
		buf.serinfo = cliInfo;
        res = recv(newfd, &buf, sizeof(buf), 0); 
        if(res < 0)
        {
            ERR_MSG("recv");
            break;
        }
        else if(0 == res)
        {
			printf("[%s:%d]:下线\n", inet_ntoa(cin.sin_addr), ntohs(cin.sin_port));
			buf.code.id = userid;
			state_change(0,&buf);//用户状态更改为离线	
            break;
        }
		buf.serinfo = cliInfo;
		int  mode = buf.mod;
		switch(mode)
		{
		case 0://用户下线
			buf.code.id = userid;
			user_quit(&buf);
			break;
		case 1://新用户注册，信息插入,给客户端发送注册情况
			//user_register(&buf);
			break;
		case 2://用户登录
			userid = user_login(&buf);
			break;
		case 3://员工信息填充
			staff_message_insert(&buf);
			break;
		case 4://信息查询
			message_seek(&buf);//用户信息查询
			break;
		case 5://员工打卡
			staff_check_on(&buf);//打卡信息插入数据库
			break;
		case 6://查看考勤
			staff_check_print(&buf);//考勤查询
			break;
		case 7://修改姓名
			staff_modify_name(&buf);//修改姓名
			break;
		case 8://修改性别
			staff_modify_sex(&buf);//修改性别
			break;
		case 9://修改年龄
			staff_modify_age(&buf);//修改年龄
			break;
		case 10://修改电话
			staff_modify_phone(&buf);//修改电话
			break;
		case 11://修改地址
			staff_modify_addr(&buf);//修改地址
			break;
		case 12://修改薪资
			staff_modify_salary(&buf);//修改薪资
			break;
		case 13://用户修改密码
			modify_passward(&buf);//修改密码
			break;
		case 14://锁定账号
			//lock_curuser(&buf);
			break;
		default:
			break;
		}	
    }   
    close(newfd);
    pthread_exit(NULL);

}

int user_register(sqlite3 *db)//用户注册
{
	char **db_data = NULL;
	int row,column;
	char *cmd = NULL;
	char cmd1[1024];
	char *errmsg;
	
	//注册成功讲用户账号放入数据库的用户账号表中
	int staffid;
	char passward[16]="";

	printf("请输入id>>");
	scanf("%d",&staffid);
	while(getchar()!=10);

	printf("请输入密码>>");
	scanf("%s",passward);
	while(getchar()!=10);
	
	//获取用户账号表单
	cmd = sqlite3_mprintf("select * from staff_passward where staffid='%d'",staffid);
	if(cmd==NULL)
	{
		ERR_SQLMSG("sqlite3_mprintf",db);
		return -1;
	}
	
	if(sqlite3_get_table(db, cmd, &db_data, &row, &column, &errmsg) !=0)
		{   
			ERR_SQLMSG("sqlite3_get_table",db);
			return -1; 
		} 
	sqlite3_free(cmd);

	//判断用户是否已经存在
	if(row==0)     //该账号无人注册
	{			
		//注册成功，用户名及密码插入表单
		sprintf(cmd1,"insert into staff_passward values(%d,'%s',1,0)",staffid,passward);

		int ret_sq = sqlite3_exec(db,cmd1,NULL,NULL,&errmsg);
		if(ret_sq!=0)
		{
			ERR_SQLMSG("sqlite3_exec",db);
			return -1;
		}
		printf(">>当前id注册成功<<\n");
		user_message_insert(db,staffid);
	}
	else
		printf(">>当前id已注册，请重新注册<<\n");

	return 0;
	///}}}
}

int user_delete(sqlite3 *db)//用户删除
{
	char **db_data = NULL;
	int row,column;
	char *cmd = NULL;
	char cmd1[1024]={0};
	char cmd2[1024]={0};
	char cmd3[1024]={0};
	char *errmsg;
	
	int staffid;

	printf("请输入id>>");
	scanf("%d",&staffid);
	while(getchar()!=10);

	
	//获取用户账号表单
	cmd = sqlite3_mprintf("select * from staff_passward where staffid='%d'",staffid);
	if(cmd==NULL)
	{
		ERR_SQLMSG("sqlite3_mprintf",db);
		return -1;
	}
	
	if(sqlite3_get_table(db, cmd, &db_data, &row, &column, &errmsg) !=0)
		{   
			ERR_SQLMSG("sqlite3_get_table",db);
			return -1; 
		} 
	sqlite3_free(cmd);

	//判断用户是否已经存在
	if(row==0)     //该账号无人注册
	{			
		printf(">>当前账户不存在<<\n");
		return 1;
	}
	else
	{
		sprintf(cmd1,"delete from staff_passward where staffid = %d",staffid);
		int ret_sq = sqlite3_exec(db,cmd1,NULL,NULL,&errmsg);
		if(ret_sq!=0)
		{
			ERR_SQLMSG("sqlite3_exec",db);
			return -1;
		}
		sprintf(cmd2,"delete from staff_check where staffid = %d",staffid);
		ret_sq = sqlite3_exec(db,cmd2,NULL,NULL,&errmsg);
		if(ret_sq!=0)
		{
			ERR_SQLMSG("sqlite3_exec",db);
			return -1;
		}
		sprintf(cmd3,"delete from staff_message where staffid = %d",staffid);
		ret_sq = sqlite3_exec(db,cmd3,NULL,NULL,&errmsg);
		if(ret_sq!=0)
		{
			ERR_SQLMSG("sqlite3_exec",db);
			return -1;
		}
		printf(">>当前账户已注销<<\n");
	}

	return 0;
	///}}}
}

int user_login(pack *packed)//用户登录
{
	///{{{
	char **db_data = NULL;
	int row,column;
	char *cmd = NULL;
	char *errmsg;
	pack data = *packed;

	int newfd = data.serinfo.newfd;
	sqlite3 *db = data.serinfo.db;

	//获取用户账号表单
	cmd = sqlite3_mprintf("select * from staff_passward where staffid='%d'",data.code.id);
	if(cmd==NULL)
	{
		ERR_SQLMSG("sqlite3_mprintf",db);
		return -1;
	}

	if(sqlite3_get_table(db, cmd, &db_data, &row, &column, &errmsg) !=0)
	{   
		ERR_SQLMSG("sqlite3_get_table",db);
		return -1; 
	}
	sqlite3_free(cmd);

	//判断用户,密码是否正确
	if(row==0)         //未找到用户id
	{
		//请检查用户id
		data.mod = 21;
		printf("用户id不存在，请检查账号\n");
	}
	else   //搜到用户
	{
		if(ctoi(db_data[7])==1) //账号重复登录
		{
			//该账号在线，请检查账号或联系客服进行申诉
			data.mod = 22;
		}
		else//验证密码
		{

			if(strncmp(db_data[5],data.code.code,strlen(db_data[5])) == 0)
			{	

				if(strlen(db_data[5])==strlen(data.code.code))
				{
					data.mod = 20;//登录成功
					data.code.power = ctoi(db_data[6]);
				}
				else
				{
					data.mod = 23;//密码错误
				}
			}
			else
			{
				data.mod = 23;//密码错误
			}
		}
	}

	//给用户发送登录情况

	ssize_t res_s = send(newfd,&data,sizeof(pack),0);
	if(res_s<0)
	{
		ERR_MSG("send");
		return -1;
	}

	if(data.mod!=20)
		return -1;
    state_change(1,&data);//登录成功修改状态

	return data.code.id;
	///}}}
}

int state_change(int mode,pack *packed)//用户状态更改
{

	pack data = *packed;
	sqlite3 *db = data.serinfo.db;
	char cmd[N] = {0};
	char *errmsg = NULL;
	
	sprintf(cmd,"update staff_passward set state=%d where staffid=%d",mode,data.code.id);

	int ret = sqlite3_exec(db,cmd,NULL,NULL,&errmsg);	
	if(ret!=0)
	{
		ERR_SQLMSG("sqlite3_exec",db);
		return -1;
	}
	return 0;	
}

int ctoi(char *str)//数字字符串转整数
{
	char *cnum = str;
	int num=0;
	while(*cnum)
	{
		num = 10*num + (*cnum-'0');
		cnum++;
	}
	return num;
}

void staff_message_insert(pack *packed)//员工信息插入
{

	char *errmsg;
	int errflag=0;
	pack data = *packed;
	char cmd[1024]={0};
	sqlite3 *db = data.serinfo.db;

	sprintf(cmd,"insert into staff_message values(%d,'%s','%s',%d,%d,'%s','%s')",\
				data.code.id,data.staff.name,data.staff.sex,data.staff.age,\
				data.staff.salary,data.staff.phone,data.staff.addr);

	int ret_sq = sqlite3_exec(db,cmd,NULL,NULL,&errmsg);
	if(ret_sq!=0)
	{
		ERR_SQLMSG("sqlite3_exec",db);
		errflag=1;
	}

	//给客户端发送信息填充反馈
	if(errflag)
		data.mod=30;
	data.mod=31;

	ssize_t res_s = send(data.serinfo.newfd,&data,sizeof(data),0);
	if(res_s<0)
	{
		ERR_MSG("send");
	}

}

int passward_init(sqlite3 *db)//初始化用户账号表状态
{

	char cmd[N]="update staff_passward set state=0 where state=1";
	char *errmsg;
	int ret = sqlite3_exec(db,cmd,NULL,NULL,&errmsg);
	if(ret!=0)
	{
		ERR_SQLMSG("sqlite3_exec",db);
		return -1;
	}

	return 0;

}

int message_seek(pack *packed)//查询员工信息
{
	pack data = *packed;
	pack buf;
	message staff;
	
	if(get_staff_message(packed,&staff))//获取员工信息
		buf.mod = 41;
	else{
		buf.mod = 40;
		buf.staff = staff;
	}
	//给客户端发送查询到的信息
	ssize_t res_s = send(data.serinfo.newfd,&buf,sizeof(buf),0);
	if(res_s<0)
	{
		ERR_MSG("send");
	}
	return 0;
}

void staff_check_on(pack *packed)//打卡信息插入数据库
{
	char *errmsg;
	int errflag=0;
	pack data = *packed;
	char cmd[1024]={0};
	sqlite3 *db = data.serinfo.db;
	message staff;

	if(get_staff_message(packed,&staff))//获取员工信息
		errflag=1;
	else{
		data.staff = staff;
	}

	sprintf(cmd,"insert into staff_check values(%d,'%s','%s')",\
				data.code.id,data.staff.name,data.time);

	int ret_sq = sqlite3_exec(db,cmd,NULL,NULL,&errmsg);
	if(ret_sq!=0)
	{
		ERR_SQLMSG("sqlite3_exec",db);
		errflag=1;
	}

	//给客户端发送打卡反馈
	if(errflag)
		data.mod=50;
	else
		data.mod=51;

	ssize_t res_s = send(data.serinfo.newfd,&data,sizeof(data),0);
	if(res_s<0)
	{
		ERR_MSG("send");
	}
}

int get_staff_message(pack* packed,message *staff)//获取员工个人信息
{
	pack data = *packed;
	sqlite3 *db = data.serinfo.db;
	char **db_data = NULL;

	char *errmsg;
	int row,column;
	char cmd[N];
	sprintf(cmd,"select * from staff_message where staffid=%d",data.code.id);
	if(sqlite3_get_table(db, cmd, &db_data, &row, &column, &errmsg) !=0)
	{   
		ERR_SQLMSG("sqlite3_get_table",db);
		return -1; 
	}


	if(column==0)
		return -1;
	else
	{
		staff->id = atoi(db_data[7]);
		strcpy(staff->name,db_data[8]);
		strcpy(staff->sex,db_data[9]);
		staff->age = atoi(db_data[10]);
		staff->salary = atoi(db_data[11]);
		strcpy(staff->phone ,db_data[12]);
		strcpy(staff->addr,db_data[13]);
	}
	return 0;
}

int get_staff_passward(pack* packed,passward *code)//获取员工账号信息
{
	pack data = *packed;
	sqlite3 *db = data.serinfo.db;
	char **db_data = NULL;

	char *errmsg;
	int row,column;
	char cmd[N];
	sprintf(cmd,"select * from staff_passward where staffid=%d",data.code.id);
	if(sqlite3_get_table(db, cmd, &db_data, &row, &column, &errmsg) !=0)
	{   
		ERR_SQLMSG("sqlite3_get_table",db);
		return -1; 
	}

	if(column==0)
		return -1;
	else
	{
		code->id = atoi(db_data[4]);
		strcpy(code->code,db_data[5]);
		code->power = atoi(db_data[6]);
		code->state = atoi(db_data[7]);	
	}
	return 0;
}

int get_staff_checklog(pack* packed,check *checklog)//获取员工打卡信息
{
	pack data = *packed;
	sqlite3 *db = data.serinfo.db;
	char **db_data = NULL;

	char *errmsg;
	int row,column;
	char cmd[N];
	sprintf(cmd,"select * from staff_check where staffid=%d",data.code.id);
	if(sqlite3_get_table(db, cmd, &db_data, &row, &column, &errmsg) !=0)
	{   
		ERR_SQLMSG("sqlite3_get_table",db);
		return -1; 
	}

	if(column==0)
		return 0;	
	else
	{
		for(int i=0;i<row;i++)
		{
			(checklog+i)->id = ctoi(db_data[3*(i+1)]);
			strcpy((checklog+i)->name,db_data[3*(i+1)+1]);
			strcpy((checklog+i)->time,db_data[3*(i+1)+2]);	
		}		
	}
	return row;
}

int staff_check_print(pack *packed)//考勤查询
{
	pack data = *packed;
	pack buf;
	check checklog[N];
	int num = get_staff_checklog(packed,checklog);//获取员工信息
	if(num<=0)
	{
		buf.mod = 61;
		//给客户端发送查询到的信息
		ssize_t res_s = send(data.serinfo.newfd,&buf,sizeof(buf),0);
		if(res_s<0)
		{
			ERR_MSG("send");
		}
		return -1;
	}		
	else
	{
		for(int i=0;i<num;i++)
		{
			buf.mod = 60;
			buf.checklog = *(checklog+i);
			//给客户端发送查询到的信息
			ssize_t res_s = send(data.serinfo.newfd,&buf,sizeof(buf),0);
			if(res_s<0)
			{
				ERR_MSG("send");
			}
		}
		//给客户端发送数据传输完毕信号
		buf.mod = 62;
		ssize_t res_s = send(data.serinfo.newfd,&buf,sizeof(buf),0);
		if(res_s<0)
		{
			ERR_MSG("send");
		}
	}
	return 0;	
}

int staff_modify_name(pack *packed)//修改姓名
{
	char *errmsg;
	pack data = *packed;
	pack buf;
	sqlite3 *db = data.serinfo.db;
	char cmd[N]={0};
	int errflag=0;

	sprintf(cmd,"update staff_message set staffname='%s' where staffid=%d",data.staff.name,data.staff.id);
	int ret = sqlite3_exec(db,cmd,NULL,NULL,&errmsg);
	if(ret!=0)
	{
		ERR_SQLMSG("sqlite3_exec",db);
		errflag = 1;
	}

	if(errflag)
		buf.mod = 71;
	else
		buf.mod = 70;
	//给客户端发送查询到的信息
	ssize_t res_s = send(data.serinfo.newfd,&buf,sizeof(buf),0);
	if(res_s<0)
	{
		ERR_MSG("send");
	}

	return 0;
}

int staff_modify_sex(pack *packed)//修改性别
{
	char *errmsg;
	pack data = *packed;
	pack buf;
	sqlite3 *db = data.serinfo.db;
	char cmd[N]={0};
	int errflag=0;

	sprintf(cmd,"update staff_message set sex='%s' where staffid=%d",data.staff.sex,data.staff.id);
	int ret = sqlite3_exec(db,cmd,NULL,NULL,&errmsg);
	if(ret!=0)
	{
		ERR_SQLMSG("sqlite3_exec",db);
		errflag = 1;
	}

	if(errflag)
		buf.mod = 81;
	else
		buf.mod = 80;
	//给客户端发送查询到的信息
	ssize_t res_s = send(data.serinfo.newfd,&buf,sizeof(buf),0);
	if(res_s<0)
	{
		ERR_MSG("send");
	}

	return 0;
}

int staff_modify_age(pack *packed)//修改年龄
{
	char *errmsg;
	pack data = *packed;
	pack buf;
	sqlite3 *db = data.serinfo.db;
	char cmd[N]={0};
	int errflag=0;

	sprintf(cmd,"update staff_message set age=%d where staffid=%d",data.staff.age,data.staff.id);
	int ret = sqlite3_exec(db,cmd,NULL,NULL,&errmsg);
	if(ret!=0)
	{
		ERR_SQLMSG("sqlite3_exec",db);
		errflag = 1;
	}

	if(errflag)
		buf.mod = 91;
	else
		buf.mod = 90;
	//给客户端发送查询到的信息
	ssize_t res_s = send(data.serinfo.newfd,&buf,sizeof(buf),0);
	if(res_s<0)
	{
		ERR_MSG("send");
	}

	return 0;
}

int staff_modify_phone(pack *packed)//修改电话
{
	char *errmsg;
	pack data = *packed;
	pack buf;
	sqlite3 *db = data.serinfo.db;
	char cmd[N]={0};
	int errflag=0;

	sprintf(cmd,"update staff_message set phone='%s' where staffid=%d",data.staff.phone,data.staff.id);
	int ret = sqlite3_exec(db,cmd,NULL,NULL,&errmsg);
	if(ret!=0)
	{
		ERR_SQLMSG("sqlite3_exec",db);
		errflag = 1;
	}

	if(errflag)
		buf.mod = 100;
	else
		buf.mod = 100;
	//给客户端发送查询到的信息
	ssize_t res_s = send(data.serinfo.newfd,&buf,sizeof(buf),0);
	if(res_s<0)
	{
		ERR_MSG("send");
	}

	return 0;
}

int staff_modify_addr(pack *packed)//修改地址
{
	char *errmsg;
	pack data = *packed;
	pack buf;
	sqlite3 *db = data.serinfo.db;
	char cmd[N]={0};
	int errflag=0;

	sprintf(cmd,"update staff_message set addr='%s' where staffid=%d",data.staff.addr,data.staff.id);
	int ret = sqlite3_exec(db,cmd,NULL,NULL,&errmsg);
	if(ret!=0)
	{
		ERR_SQLMSG("sqlite3_exec",db);
		errflag = 1;
	}

	if(errflag)
		buf.mod = 111;
	else
		buf.mod = 110;
	//给客户端发送查询到的信息
	ssize_t res_s = send(data.serinfo.newfd,&buf,sizeof(buf),0);
	if(res_s<0)
	{
		ERR_MSG("send");
	}

	return 0;
}

int staff_modify_salary(pack *packed)//修改薪资
{
	char *errmsg;
	pack data = *packed;
	pack buf;
	sqlite3 *db = data.serinfo.db;
	char cmd[N]={0};
	int errflag=0;

	sprintf(cmd,"update staff_message set salary=%d where staffid=%d",data.staff.salary,data.staff.id);
	int ret = sqlite3_exec(db,cmd,NULL,NULL,&errmsg);
	if(ret!=0)
	{
		ERR_SQLMSG("sqlite3_exec",db);
		errflag = 1;
	}

	if(errflag)
		buf.mod = 121;
	else
		buf.mod = 120;
	//给客户端发送查询到的信息
	ssize_t res_s = send(data.serinfo.newfd,&buf,sizeof(buf),0);
	if(res_s<0)
	{
		ERR_MSG("send");
	}

	return 0;
}

int user_quit(pack *packed)//用户下线
{
	pack data = *packed;
	if(state_change(0,packed))//用户状态更改
		data.mod = 0;//失败
	data.mod =1;

	//给用户发送注册情况
	ssize_t res_s = send(data.serinfo.newfd,&data,sizeof(pack),0);
	if(res_s<0)
	{
		ERR_MSG("send");
		return -1;
	}

	return 0;
}

int modify_passward(pack *packed)//修改密码
{
 	char **db_data = NULL;
	int row,column;
	char cmd[N];
	char cmd1[N];
	char *errmsg;
	pack data = *packed;

	int newfd = data.serinfo.newfd;
	sqlite3 *db = data.serinfo.db;

	//获取用户账号表单
	sprintf(cmd,"select * from staff_passward where staffid='%d'",data.code.id);
	if(sqlite3_get_table(db, cmd, &db_data, &row, &column, &errmsg) !=0)
	{   
		ERR_SQLMSG("sqlite3_get_table",db);
		return -1; 
	}

	//判断用户,密码是否正确
	if(row==0)         //未找到用户id
	{
		//请检查用户id
		data.mod = 131;
		printf("用户id不存在，请检查账号\n");
	}
	else   //搜到用户
	{
		if(ctoi(db_data[7])==1) //账号重复登录
		{
			//该账号在线，请检查账号或联系客服进行申诉
			data.mod = 132;
		}
		else//验证密码
		{

			if(strncmp(db_data[5],data.code.code,strlen(db_data[5])) == 0)
			{	

				if(strlen(db_data[5])==strlen(data.code.code))
				{
					data.mod = 130;//密码验证成功
				}
				else
				{
					data.mod = 133;//密码错误
				}
			}
			else
			{
				data.mod = 133;//密码错误
			}
		}
	}
	if(data.mod == 130)
	{
		sprintf(cmd1,"update staff_passward set passward='%s' where staffid=%d",data.code.newcode,data.code.id);
		int ret = sqlite3_exec(db,cmd1,NULL,NULL,&errmsg);
		if(ret!=0)
		{
			ERR_SQLMSG("sqlite3_exec",db);
			data.mod = 134;//密码修改失败
		}
	}

	//给用户发送登录情况
	ssize_t res_s = send(newfd,&data,sizeof(pack),0);
	if(res_s<0)
	{
		ERR_MSG("send");
		return -1;
	}
	return 0;
}

int power_menu(sqlite3 *db)//权限修改菜单
{
	int staffid;
	char mode[S];

	printf("请输入员工id>>");
	scanf("%d",&staffid);
	while(getchar()!=10);

	while(1)
	{
		printf("-------------------\n");
		printf("-----1.升为管理员----\n");
		printf("-----2.降为普通用户--\n");
        printf("-----3.返回---------\n");
		printf("-------------------\n");

		bzero(mode,S);
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
		case '1'://升管理员
			power_change(db,staffid,0);
			break;
        case '2'://降普通用户
			power_change(db,staffid,1);
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

	return 0;
}

int power_change(sqlite3 *db,int staffid,int power)//权限修改
{
	char cmd[N] = {0};
	char *errmsg = NULL;
	
	sprintf(cmd,"update staff_passward set power=%d where staffid=%d",power,staffid);

	int ret = sqlite3_exec(db,cmd,NULL,NULL,&errmsg);	
	if(ret!=0)
	{
		ERR_SQLMSG("sqlite3_exec",db);
		return -1;
	}
	return 0;
}

int user_message_insert(sqlite3 *db,int staffid)//员工基本信息插入
{
	char name[S]={0};
	char sex[S]={0};
	int age;
	int salary;
	char phone[S]={0};
	char addr[S]={0};
	char cmd[N]={0};
	char *errmsg=NULL;

	printf("请输入姓名>>");
	scanf("%s",name);
	while(getchar()!=10);

	printf("请输入性别>>");
	scanf("%s",sex);
	while(getchar()!=10);

	printf("请输入年龄>>");
	INT_INPUTCHECK(age);

	printf("请输入薪资>>");
	INT_INPUTCHECK(salary);

	printf("请输入联系电话>>");
	scanf("%s",phone);
	while(getchar()!=10);
	while (strlen(phone)!=11)
	{
		printf("无效数据，请重新输入>>");
		scanf("%s",phone);
		while(getchar()!=10);
	}

	printf("请输入家庭住址>>");
	scanf("%s",addr);
	while(getchar()!=10);

	sprintf(cmd,"insert into staff_message values(%d,'%s','%s',%d,%d,'%s','%s')",\
					staffid,name,sex,age,salary,phone,addr);
	int ret_sq = sqlite3_exec(db,cmd,NULL,NULL,&errmsg);
	if(ret_sq!=0)
	{
		ERR_SQLMSG("sqlite3_exec",db);
		return -1;
	}
	return 0;
}