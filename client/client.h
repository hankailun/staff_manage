
#ifndef  __CLIENT_H__
#define  __CLIENT_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sqlite3.h>
#include <time.h>

#define ERR_MSG(msg)do{\
	fprintf(stderr,"%d-%s-%s\n",__LINE__,__func__,__FILE__);\
	perror(msg);\
	}while(0)

#define ERR_SQLMSG(msg,db)do{\
	fprintf(stderr,"%d-%s-%s\n",__LINE__,__func__,__FILE__);\
	printf("%s:%s\n",msg,sqlite3_errmsg(db));\
	}while(0)

#define INT_INPUTCHECK(num)    \
            do{\
                int ret;\
                do{\
                    ret = scanf("%d",&num);\
                    while(getchar()!=10);\
                    if(!ret)\
                        printf("无效输入\n");\
                }while(!ret);\
            }while(0)

#define N 256

#define S 64



typedef struct Client
{
    int sfd;
    struct sockaddr_in sin;
	sqlite3 *db;
}client;

typedef struct staff_passward{
    int  id;
    char code[S];
    char newcode[S];
    int  power;
    int  state;
}passward;

typedef struct staff_message{
    int  id;
    char name[S];
    char sex[S];
    int  age;
    int  salary;
    char phone[S];
    char addr[S];
}message;

typedef struct staff_check{
    int  id;
    char name[S];
    char time[S];
}check;

typedef struct packed_data{
    int mod;
    char time[S];
    client cliinfo;
    passward code;
    message staff;
    check checklog;
}pack;


int net_init(client*info,char *ip,int port);//网络初始化

sqlite3 *sql_init(void);//数据库初始化

int mune2_manage(pack *packed,int id);//管理员二级菜单

int mune2_user(pack *packed,int id);//普通用户二级菜单

int mune1_select(pack *packed);//一级菜单

int user_login(pack *packed);//用户登录

int staff_message_insert(pack *packed);//员工信息填充

int message_seek(pack *packed,int seekid);//管理员信息查询

int user_seek(pack *packed,int curid);//普通用户信息查询

int staff_check_on(pack *packed,int curid);//员工打卡

int manage_check_print(pack *packed,int curid);//打印考勤记录

int manage_modify_message(pack *packed,int curid);//员工信息修改

int user_check_print(pack *packed,int curid);//打印考勤记录

int user_modify_message(pack *packed,int curid);//员工信息修改

int modify_name(pack *packed,int seekid);//员工姓名修改

int modify_sex(pack *packed,int seekid);//员工性别修改

int modify_age(pack *packed,int seekid);//员工年龄修改

int modify_phone(pack *packed,int seekid);//员工电话修改

int modify_addr(pack *packed,int seekid);//员工地址修改

int modify_salary(pack *packed,int seekid);//员工薪资修改

int user_quit(pack *packed,int seekid);//员工下线

int passward_modify(pack *data);//用户密码修改


#endif /*__CLIENT_H__*/
