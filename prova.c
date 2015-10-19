#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <ncurses.h>
#include <poll.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/types.h>
#include "xwiimote.h"

static struct xwii_iface *iface;
static bool freeze = false;
static pthread_t main_tid;
#define IR_POINTER 1

static int enumerate(){
	struct xwii_monitor *mon;
	char *ent;
	int num = 0;

	mon = xwii_monitor_new(false, false);
	if (!mon) {
		printf("Cannot create monitor\n");
		return -EINVAL;
	}

	while ((ent = xwii_monitor_poll(mon))) {
		printf("  Found device #%d: %s\n", ++num, ent);
		free(ent);
	}

	xwii_monitor_unref(mon);
	return 0;
}

static char *get_dev(int num){
	struct xwii_monitor *mon;
	char *ent;
	int i = 0;

	mon = xwii_monitor_new(false, false);
	if (!mon) {
		printf("Cannot create monitor\n");
		return NULL;
	}

	while ((ent = xwii_monitor_poll(mon))) {
		if (++i == num)
			break;
		free(ent);
	}

	xwii_monitor_unref(mon);

	if (!ent)
		printf("Cannot find device with number #%d\n", num);

	return ent;
}

static void ir_show(const struct xwii_event *event){
	int i;
	for(i=0;i<IR_POINTER;i++){
		if (xwii_event_ir_is_valid(&event->v.abs[i])) {
			printf("IR X: %d\n", event->v.abs[i].x);
			printf("IR Y: %d\n", event->v.abs[i].y);
		}
	}
}

static void ir_clear(void){
	struct xwii_event ev;

	ev.v.abs[0].x = 1023;
	ev.v.abs[0].y = 1023;
	ev.v.abs[1].x = 1023;
	ev.v.abs[1].y = 1023;
	ev.v.abs[2].x = 1023;
	ev.v.abs[2].y = 1023;
	ev.v.abs[3].x = 1023;
	ev.v.abs[3].y = 1023;
	ir_show(&ev);
}

static void mp_clear(void){
	struct xwii_event ev;

	ev.v.abs[0].x = 0;
	ev.v.abs[0].y = 0;
	ev.v.abs[0].z = 0;
}

static void mp_toggle(void){
	int ret;

	if (xwii_iface_opened(iface) & XWII_IFACE_MOTION_PLUS) {
		xwii_iface_close(iface, XWII_IFACE_MOTION_PLUS);
		mp_clear();
	} else {
		ret = xwii_iface_open(iface, XWII_IFACE_MOTION_PLUS);
		if (ret)
			printf("Error: Cannot enable MP: %d",
				    ret);
		else
			printf("Info: Enable Motion Plus");
	}
}

static void accel_clear(void){
	struct xwii_event ev;

	ev.v.abs[0].x = 0;
	ev.v.abs[0].y = 0;
	ev.v.abs[0].z = 0;
}

static void accel_toggle(void){
	int ret;

	if (xwii_iface_opened(iface) & XWII_IFACE_ACCEL) {
		xwii_iface_close(iface, XWII_IFACE_ACCEL);
		accel_clear();
	} else {
		ret = xwii_iface_open(iface, XWII_IFACE_ACCEL);
		if (ret)
			printf("Error: Cannot enable accelerometer: %d",
				    ret);
		else
			printf("Info: Enable accelerometer");
	}
}

static int run_iface(struct xwii_iface *iface, struct xwii_event *event){
	int ret=0, fds_num;
	struct pollfd fds[2];

	memset(fds, 0, sizeof(fds));
	fds[0].fd = 0;
	fds[0].events = POLLIN;
	fds[1].fd = xwii_iface_get_fd(iface);
	fds[1].events = POLLIN;
	fds_num = 2;
	ret = xwii_iface_watch(iface, true);

	if(ret) printf("errore");
	while(true){
		ret = poll(fds, fds_num, -1);
		if(ret<0){
			if(errno != EINTR){
				ret = -errno;
				printf("error");
				break;
			}
		}

		ret = xwii_iface_dispatch(iface, event, sizeof(*event));
		if(ret){
			if(ret !=-EAGAIN){
				printf("error");
				break;
			}
		}
		else if (!freeze) {
			switch (event->type) {
				case XWII_EVENT_GONE:
					printf("Info: Device gone");
					fds[1].fd = -1;
					fds[1].events = 0;
					fds_num = 1;
					break;
				case XWII_EVENT_IR:
					//ir_show(&event);
					pthread_kill(main_tid,SIGUSR1); //manda un segnale al thread
					break;
				default: printf("error");
			}
		}
	}
}

void calibration_thread(struct xwii_event *event_p){
	int ret = run_iface(iface, event_p);
	/*xwii_iface_unref(iface);
	if (ret) {
		print_error("Program failed; press any key to exit");
		refresh();
		timeout(-1);
		getch();
	}
	endwin();*/
}

int main(int argc, char **argv){
	struct xwii_event event;

	void sig_handler(int signo){
		if (signo == SIGUSR1) ir_show(&event);
	}

	int ret = 0;
 	char *path = NULL;

	if(argc < 2) printf("usage: min 1 parameter \n");
		else if(!strcmp(argv[1], "list")){
			printf("Listing devices\n");
			ret = enumerate();
			printf("end of device list\n");
		}
	else{
		if (argv[1][0] != '/') path = get_dev(atoi(argv[1]));

		ret = xwii_iface_new(&iface, path ? path : argv[1]);
		free(path);
		if (ret) printf("Cannot create xwii_iface '%s' err:%d\n", argv[1], ret);
		else {
			ir_clear();
			ret = xwii_iface_open(iface, xwii_iface_available(iface) | XWII_IFACE_WRITABLE);
			if (ret) printf("Error: Cannot open interface: %d, use SUDO!", ret);
			accel_toggle();
			mp_toggle();

			pthread_t tid;
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			main_tid = pthread_self();
			pthread_create(&tid,&attr,(void*)calibration_thread, &event);
			if (signal(SIGUSR1, sig_handler) == SIG_ERR)
				printf("\ncan't catch SIGINT\n");
			while(true) sleep(1);
		}
  }
}
