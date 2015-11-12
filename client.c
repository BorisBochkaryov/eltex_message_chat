#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <ncurses.h>
#include <string.h>
#include <dirent.h>
#include <locale.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

struct msgbuf {
	long mtype; // тип сообщения 
	char mtext[255]; // текст сообщения 
};

struct winsize w; // информация об окне
WINDOW *wndmess, *wndtext; // subwindow

// перерисовка экрана
void update_screen(WINDOW *wnd){
	box(wnd,0,0);
	wrefresh(wnd);
}

int clients[255];
int num = 0, numm = 0;

/* функция приема сообщений (выполняется в потоке) */
void* users(){
	char filename[255] = "";
	sprintf(filename,"+%d",getpid());
	key_t key = ftok(filename,16L);
	int msg_id = msgget(key, IPC_CREAT|0666); // получение доступа
	int t;
	struct msgbuf buf1;
	// считывание сообщений, предназначенных клиенту
	while((t = msgrcv(msg_id,&buf1,sizeof(struct msgbuf),1234L,0)) != -1){
		move(numm+1,1);
		numm++;
		printw("%s",buf1.mtext);
		refresh();
		memset(&buf1,0,sizeof(struct msgbuf));
	}
	exit(0);
}
/* функция для считывания и отправки (выполняется в потоке) */
void* text(){
	char* temp = (char*)malloc(sizeof(char)*255);
	while(1){
		int ch = getch();
		werase(wndtext); // очистка subwindow
		box(wndtext, 0, 0);
		// в зависимости от клавиши
		switch(ch){
			case '\n':{ // enter
				key_t key = ftok("filemsg",16L);
				int msg_id = msgget(key, IPC_CREAT|0666); // получение доступа
				struct msgbuf buf;
				buf.mtype = 2000L;
				sprintf(buf.mtext,"%d:%s",getpid(),temp);
				int id = msgsnd(msg_id,&buf,sizeof(struct msgbuf),IPC_NOWAIT); // отправка
				memset(temp,0,sizeof(temp));
			} break;
			case 263: { // backspace
				temp[strlen(temp)-1] = '\0';
				move(w.ws_row*0.9, 1);
				printw("%s",temp); 
				} break;
			default: { // обычная буква
				sprintf(temp,"%s%c",temp,ch);
				move(w.ws_row*0.9, 1);
				printw("%s",temp);
			}
		}	
		wrefresh(wndtext);
	}
	exit(0);
}

int main(){
	{ // отправка pid серверу
		key_t key = ftok("pidfile",16L);
		int msg_server = msgget(key, IPC_CREAT|0666); // получение доступа
		struct msgbuf buf_pid;
		buf_pid.mtype = 3000L;
		sprintf(buf_pid.mtext,"%d",getpid());
		int id = msgsnd(msg_server,&buf_pid,sizeof(struct msgbuf),IPC_NOWAIT);
	}
	// инициализация ncurses
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);

	refresh();
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

 	// задаем subwindows 
	wndmess = newwin(w.ws_row, w.ws_col*0.75, 0, 0);
	box(wndmess,0,0);
	wrefresh(wndmess);

	wndtext = newwin(w.ws_row*0.15+1, w.ws_col*0.75, w.ws_row*0.85, 0);
	box(wndtext,0,0);
	wrefresh(wndtext);

	move(w.ws_row*0.9, 1);

	// создаем потоки
	pthread_t thread1,thread2;
	int result1 = pthread_create(&thread2, NULL, text, NULL);
	int result = pthread_create(&thread1, NULL, users, NULL);
	// бесконечный цикл работы
	while(1){
	}

 	// удаление subwindows
	delwin(wndmess);
	delwin(wndtext);
	wmove(stdscr, 8, 1);

	endwin();
	exit(EXIT_SUCCESS);
}
