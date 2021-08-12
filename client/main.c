#include "client.h"

int main(int argc, const char *argv[])
{

	char ip[32]="";
	int  port=0;

	//手动配置网络
	if(argc<2)
	{
		printf("请输入服务器IP地址>>");
		scanf("%s",ip);
		while(getchar()!=10);

		printf("请输入服务器端口号>>");
		scanf("%d",&port);
		while(getchar()!=10);
	}

   
    client info;
	pack data ;
	int ret_s,ret_m;

	//网络初始化
	int ret_n = net_init(&info,ip,port);
	if(ret_n<0)
	{
		printf("网络初始化失败\n");
		return -1;
	}

	//数据库初始化
	sqlite3 *db = NULL;
	db = sql_init();
	if(db==NULL)
	{
		printf("数据库初始化失败\n");
		return -1;
	}

	info.db = db;
	data.cliinfo = info;
	

	while(1)
	{
		//一级菜单选择
		ret_m =mune1_select(&data);

		if(ret_m>0)//登录成功
		{
			//二级菜单，与服务器交互，主体功能实现
			if(data.code.power == 0)
				ret_s = mune2_manage(&data,ret_m);//管路员登录			
			else
				ret_s = mune2_user(&data,ret_m);//普通用户登录
			if(ret_s==1)//用户下线，返回一级菜单
				continue;
			else if(ret_s==0)//用户退出
				break;				
		}
		else if(ret_m == 0)//一级菜单退出，用户退出
			break;           
	}	 
    close(info.sfd);
    sqlite3_close(db);
	return 0;
}
