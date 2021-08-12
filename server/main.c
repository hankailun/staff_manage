#include "server.h"

int main(int argc,char *argv[])
{
    
    char ip[32]="";
	int  port = 0;
    //设置服务器网络
    
    if(argc<2)
	{
		printf("请输入本机IP地址>>");
		scanf("%s",ip);
		while(getchar()!=10);

		printf("请输入绑定端口号>>");
		scanf("%d",&port);
		while(getchar()!=10);
	}
    
    //网络初始化
    int sfd = net_init(ip,port);

    //数据库初始化
    sqlite3 *db = NULL;

	db = sql_init();
	if(db==NULL)
	{
		printf("数据库初始化失败\n");
		return -1;
	}


    //监听客户端事件
    int res_p = poll_client(sfd,db);
	if(res_p<0)
	{
		printf("监听客户端异常\n");
		return -1;
	}


    //资源释放
    sqlite3_close(db);

    close(sfd);

    return 0;
}