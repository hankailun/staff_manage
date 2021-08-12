#ifndef  __SERVER_H__
#define  __SERVER_H__

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sqlite3.h>
#include <pthread.h>
#include <poll.h>
#include <time.h>

#define ERR_MSG(msg) do{\
    printf("%d_%s_%s\n", __LINE__, __func__, __FILE__);\
    perror(msg);\
}while(0)

#define ERR_SQLMSG(msg,db) do{\
    printf("%d_%s_%s\n", __LINE__, __func__, __FILE__);\
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

#define N 1024

#define S 64
//线程用的结构体
typedef struct server_msg
{
    int newfd;
    struct sockaddr_in cin;
	sqlite3 *db;
}sermsg;

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
    sermsg serinfo;
    passward code;
    message staff;
    check checklog;
}pack;


int net_init(char *ip,int port);//网络初始化

sqlite3 * sql_init();//数据库初始化

int poll_client(int sfd,sqlite3 *db);//监听服务器事件

void* recv_cli_msg(void* arg);//与客户端交互线程

int user_register(sqlite3 *db);//用户注册

int user_delete(sqlite3 *db);//用户删除

void staff_message_insert(pack *packed);//员工信息填充

int user_login(pack *packed);//用户登录

int modify_passward(pack *packed);//修改密码

int user_quit(pack *packed);//用户下线

int state_change(int mode,pack *packed);//用户状态更改

int power_menu(sqlite3 *db);//用户权限修改

int power_change(sqlite3 *db,int staffid,int power);//权限修改

int ctoi(char *str);//数字字符串转整数

int passward_init(sqlite3 *db);//初始化账号状态

int message_seek(pack *packed);//查询员工信息

int staff_check_print(pack *packed);//考勤查询

int get_staff_message(pack* packed,message *staff);//获取员工信息

int get_staff_passward(pack* packed,passward *code);//获取员工账号信息

int get_staff_checklog(pack* packed,check *checklog);//获取员工打卡信息

void staff_check_on(pack *packed);//打卡信息插入数据库

int staff_modify_name(pack *packed);//修改姓名数据

int staff_modify_sex(pack *packed);//修改性别数据

int staff_modify_age(pack *packed);//修改年龄数据

int staff_modify_phone(pack *packed);//修改电话数据

int staff_modify_addr(pack *packed);//修改地址数据

int staff_modify_salary(pack *packed);//修改地址数据

int user_message_insert(sqlite3 *db,int staffid);//员工基本信息插入

#endif  /*__SERVER_H__*/