#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>

struct msgbuf {
	long mtype; // тип сообщения 
	char mtext[255]; // текст сообщения
};

int clients[255]; // массив клиентов
int num = 0; // количество клиентов

/* функция приема и сохранения ppid клиентов (выполняется в потоке) */
void* pidcli(){
	key_t key = ftok("pidfile",16L);
	int msg_id = msgget(key, IPC_CREAT|0666); // получение доступа
	struct msgbuf buf1;
	int id;
        // получение сообщения и его обработка
	while((id = msgrcv(msg_id,&buf1,sizeof(struct msgbuf),3000L,0)) != -1){
		int n;
		sscanf(buf1.mtext,"%d",&n);
		clients[num] = n;
		printf("Client %d connect!\n",n);
		num++;
	}	
	
}

int main(){
	pthread_t thread1; // поток для приема pid
	int result = pthread_create(&thread1, NULL, pidcli, NULL);

	key_t key = ftok("file",16L);
	int msg_id = msgget(key, IPC_CREAT|0666); // получение доступа

	struct msgbuf buf1;
	int id;
	// получение 
	while((id = msgrcv(msg_id,&buf1,sizeof(struct msgbuf),2000L,0)) != -1){
		int pid; char str[255]; // полученная из сообщения информация
		sscanf(buf1.mtext,"%d:%s",&pid,str);
		printf("%d -> %s\n",pid,str);		
		int i;
		// рассылаем всем клиентам сообщение
		for(i = 0; i < num; i++){
			// кроме того, кто прислал
			if(clients[i] != pid){
				char filename[255] = "";
				sprintf(filename,"+%d",clients[i]);
				key_t key1 = ftok(filename,16L);
				int msg_id = msgget(key1, IPC_CREAT|0666); // получение доступа к очереди клиента
				struct msgbuf buf;
				buf.mtype = 1234L;
				sprintf(buf.mtext,"%s\n",str);
				int id = msgsnd(msg_id,&buf,sizeof(struct msgbuf),IPC_NOWAIT); // отправка
			}
		}
	}	
	return 0;
}
