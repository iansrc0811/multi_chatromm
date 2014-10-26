#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>
#define MAXSIZE 100

struct ChatRoomUser{
	char name[MAXSIZE];
	int connfd;
	struct ChatRoomUser *next;
};
struct ChatRoomUser *chatRoom[50];//struct ChatRoomUser* 陣列,用來表示聊天室array 每一個chatRoom[i]為一個聊天室

void* chatRoom_thread(void* arg);//function prototype

int main()
{
	int listenfd = 0, connfd = 0,i=0;

	struct sockaddr_in serv_addr; //
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, 0, sizeof(serv_addr));// fill zero in "sockaddr_in ser_addr"

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(62058);//port number

	int on = 1;
	int status = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const 		char *)&on, sizeof(on));
	if (status == -1)
		perror("setsockopt() error");
	bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

	listen(listenfd, 100);

	for(i =0;i<50;i++){
		chatRoom[i] = NULL;//將所有指標設為空值
	}

	while (1){
		//每個新連線都是一個thread
		//每個connfd都不同 後面會用來辨識使用者
		connfd = accept(listenfd, NULL, NULL);

		pthread_t id;
		pthread_create(&id, NULL,&chatRoom_thread, &connfd );//博大精深的pthread_create 用來呼叫另一個function , 也就是thread要執行的任務
		//magic! pass "connfd" to "chatRoom_thread" function,
		//using the "function pointer
		//the "chat" function must be "void* function_name(void* arg)
		//if not understand function pointer, check http://kheresy.wordpress.com/2010/11/03/function_pointer/
		//and google "pthread_create"

	}

	return 0;
}

void* chatRoom_thread(void* arg)//thread要執行的function
{
	int connfd = *(int*)arg;//"connfd" pass to "arg" by the fourth parameter in pthread_create()
	//change  void* to int* , then dereference to "int" , so we write (int*)arg ,then add "*" in front of it.
	//Finally we get "connfd" by the function pthread_create()

	char userName[MAXSIZE]={0};
	char input_roomNum[MAXSIZE]={0};
	char recv_Buffer[MAXSIZE]={0};
	char send_Buffer[MAXSIZE]={0};
	char text1[] = {"Hello >_^  please enter your name: "};
	char text2[] = {"please choose the room number(0~10):"};
	char text3[] = {"Type --- to leave chatroom\n"};//text1 2 3 是登入時要顯示的字
	write(connfd, text1, strlen(text1));  //用write()寫出text1
	read(connfd,userName,sizeof(userName));//read()讀使用者輸入的name
	//printf("\n!!%s!!\n",name);
	write(connfd, text2, strlen(text2));
	read(connfd,input_roomNum,sizeof(input_roomNum));//room 為使用者輸入的房號
	write(connfd, text3, strlen(text3));
	int roomNum=atoi(input_roomNum);//room轉型int
	//int i=0;
	userName[strlen(userName)-2]='\0'; //change the last 2 char to '\0' 不然輸出有問題
	printf("[%s]log in room[%d]\n",userName,roomNum);

	struct ChatRoomUser *me = (struct ChatRoomUser *)malloc(sizeof(struct ChatRoomUser));//malloc a memory space depend on the size of "struct ChatRoomUser"
	//"me" is the newly added user 建立一個user
	strcpy(me->name,userName);
	me->connfd=connfd;
	me->next=NULL;//"next"用來當作linked list的接點

	struct ChatRoomUser *now =NULL;

	if(chatRoom[roomNum] == NULL) //roomNum 為使用者輸入的房號 用來當作array index
		//chatRoom[roomNum]的型態為struct ChatRoomUse *, 表示他是一個struct ChatRoomUse型態的「動態陣列」,再這裡把他當成一個聊天室 房號為roomNum
	{
		chatRoom[roomNum]=me;//如果沒有roomNum這個index 就把struct Room *chatRoom[roomNum]指向"me"
	}
	else{
		for(now=chatRoom[roomNum];now->next!=NULL;now=now->next); //find the last index in "chatRoom[roomNum]
		now->next=me;  	//add "me" to the next of "now"  (this is linked list)  把使用者加到這個linked list的最後面
	}
	while(1)//開始聊天
	{
		memset(recv_Buffer, 0, sizeof(recv_Buffer));
		read(connfd,recv_Buffer,sizeof(recv_Buffer));//recv_Buffer存輸入的對話
		recv_Buffer[strlen(recv_Buffer)-2]='\0'; //this step to solve the strange problem, the last two chars have problem
		if(strcmp(recv_Buffer,"---")==0) break;// 輸入--- 就break

		memset(send_Buffer, 0, sizeof(send_Buffer));
		strcat (send_Buffer,"[");
		strcat(send_Buffer,userName);
		strcat(send_Buffer,"]:");//transmit  對話
		strcat(recv_Buffer,"\n");

		for(now=chatRoom[roomNum];now!=NULL;now=now->next)//傳送給chatRoom[roomNum]裡的每一個user
		{
			if(now->connfd != connfd){//不要傳給自己(用connfd判斷）
				write(now->connfd,send_Buffer,strlen(send_Buffer));// [userName]:
				write(now->connfd,recv_Buffer,strlen(recv_Buffer));//對話內容\n
				//這樣寫好像怪怪的 忘記為什麼要這樣寫了
			}
		}

	}
	struct ChatRoomUser *delete ;//delete struct
	struct ChatRoomUser *prev ;
	//log out

    if(chatRoom[roomNum]->connfd==connfd)//如果要刪掉chatRoom[roomNum]的第一個使用者時
	{
		delete=chatRoom[roomNum];
		chatRoom[roomNum] = chatRoom[roomNum]->next;//chatRoom[roomNum]改成chatRoom[roomNum]的next 然後刪掉目前chatRoom[roomNum]
		free(delete);
		//for(i=(strlen(name)-2);i<strlen(name);i++)
		//name[i]='\0';
		printf("[%s]log out room[%d]\n",userName,roomNum);
	}else{

		for(prev=chatRoom[roomNum];prev->next->connfd!=connfd;prev=prev->next);//再chatRoom[roomNum]找到要刪的user(當connfd相同就是了）
		delete=prev->next;
		prev->next=delete->next;
		//  原本linkedlist裡是  prev-----------prev->next-------------------prev->next->next
		//要把中間的prev->next刪掉
		//變成 prev-------------------prev->next->next ,
		//用delet把prev->next存起來 把prev指向delet->next（就是prev->next->next）再把delet刪掉 就完成了
		free(delete);
		printf("[%s]log out room[%d]\n",userName,roomNum);
	}
      close(connfd);		//關掉
}
