/*
** TODO: dividere in più file
** gcc `pkg-config --cflags gtk+-3.0` -o lightboard main.c monitor.c core.c -ludev -lX11 -lXtst -lpthread `pkg-config --libs gtk+-3.0`
*/
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
#include <X11/extensions/XTest.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/types.h>
#include "../include/xwiimote.h"
#include "../include/main.h"

struct xwii_iface *iface;
pthread_t main_tid;

GObject *layout, *window;
GtkWidget *spinner;
Display *dpy;
Screen *s;
int point, change;
double matrix_A[9][9], matrix_x[9], matrix_res[20];
gboolean reset_point, freeze, stop_ir, calibrated;

point_s point_array[4];

char *get_dev(int num){
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

int run_iface(struct xwii_iface *iface, struct xwii_event *event){
	int ret=0, fds_num;
	struct pollfd fds[2];
	int default_wait_time = 400;

	memset(fds, 0, sizeof(fds));
	fds[0].fd = 0;
	fds[0].events = POLLIN;
	fds[1].fd = xwii_iface_get_fd(iface);
	fds[1].events = POLLIN;
	fds_num = 2;
	ret = xwii_iface_watch(iface, true);
	if(ret) printf("errore");

	while(true){
		ret = poll(fds, fds_num, default_wait_time);
		if(ret<0){
			if(errno != EINTR){
				ret = -errno;
				printf("error");
				break;
			}
		}
		else if(!ret) pthread_kill(main_tid, SIGUSR2); //non viene rilevato nessun cambiamento nei dati IR

		ret = xwii_iface_dispatch(iface, event, sizeof(*event));

		if(ret){
			if(ret !=-EAGAIN){
				printf("error");
				break;
			}
		}
		else{
			switch (event->type) {
				case XWII_EVENT_GONE:
					printf("Info: Device gone");
					fds[1].fd = -1;
					fds[1].events = 0;
					fds_num = 1;
					break;
				case XWII_EVENT_IR:
					pthread_kill(main_tid,SIGUSR1); //manda un segnale al thread
					break;
				default: printf("error");
			}
		}
	}
}

void reset(){
    point = 0;

	gtk_widget_destroy((GtkWidget *) layout);
	layout = (GObject *) gtk_layout_new(NULL, NULL);
	gtk_container_add (GTK_CONTAINER (window), (GtkWidget *) layout);

	int i;
	char str[2];
	for(i=0;i<4;i++){
		sprintf(str, "%d", i+1);
		point_array[i].label = gtk_label_new(str);
		gtk_widget_set_size_request(point_array[i].label,LABEL_SIZE,LABEL_SIZE);
		gtk_layout_put((GtkLayout *)layout,point_array[i].label,point_array[i].default_x, point_array[i].default_y);
		point_array[i].runtime_x = point_array[i].default_x;
		point_array[i].runtime_y = point_array[i].default_y;
	}
	gtk_widget_show_all((GtkWidget *) window);
}


/* TODO controlla se è stato premuto ESC, funzione di callback del segnale associato*/
static gboolean key_event(GtkWidget *widget, GdkEventKey *event){
    if(event->keyval==GDK_KEY_Escape){
		gtk_widget_destroy(widget);
        return TRUE;
    }
    if(event->keyval==GDK_KEY_minus)
		change_distance_from_border(SUBTRACT);
    if(event->keyval==GDK_KEY_plus)
		change_distance_from_border(SUM);
	if(event->keyval==GDK_KEY_a)
		reset();
    return FALSE;
}

int main(int argc, char **argv){
	struct xwii_event event;
	reset_point=FALSE;
	change = 1;

	void sig_handler(int signo){
		if (!calibrated && signo==SIGUSR1){ //IR
			if(!freeze && xwii_event_ir_is_valid(&event.v.abs[0])) point_f(&event);
		}
		else if(!calibrated && signo==SIGUSR2){ //no IR
			if(stop_ir) reset_point = TRUE;
			else freeze=FALSE;
		}
		else if(calibrated && signo==SIGUSR1){
			float new_x,new_y;
			if(xwii_event_ir_is_valid(&event.v.abs[0])){
				new_x = ((matrix_res[0]*event.v.abs[0].x) + (matrix_res[1]*event.v.abs[0].y) + matrix_res[2]) /
						((matrix_res[6]*event.v.abs[0].x) + (matrix_res[7]*event.v.abs[0].y) + 1);
				new_y = ((matrix_res[3]*event.v.abs[0].x) + (matrix_res[4]*event.v.abs[0].y) + matrix_res[5]) /
						((matrix_res[6]*event.v.abs[0].x) + (matrix_res[7]*event.v.abs[0].y) + 1);

				XTestFakeMotionEvent (dpy, 0, new_x, new_y, CurrentTime);
  				XSync(dpy, 0);

			}
		}
		else if(calibrated && signo==SIGUSR2){
			printf("no");
		}
	}

	int ret = 0;
 	char *path = NULL;


	if(argc < 2) printf("usage: min 1 parameter \n");
	else{
		if (argv[1][0] != '/') path = get_dev(atoi(argv[1]));

		ret = xwii_iface_new(&iface, path ? path : argv[1]);
		free(path);
		if (ret) printf("Cannot create xwii_iface '%s' err:%d\n", argv[1], ret);
		else {
			ret = xwii_iface_open(iface, xwii_iface_available(iface) | XWII_IFACE_WRITABLE);
			if (ret) printf("Error: Cannot open interface: %d, use SUDO!", ret);
			/*rimuove i messaggi dell'accelerometro e del motion plus*/
			if (xwii_iface_opened(iface) & XWII_IFACE_ACCEL) xwii_iface_close(iface, XWII_IFACE_ACCEL);
			if (xwii_iface_opened(iface) & XWII_IFACE_MOTION_PLUS) xwii_iface_close(iface, XWII_IFACE_MOTION_PLUS);

			pthread_t tid;
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			main_tid = pthread_self();
			pthread_create(&tid,&attr,(void*)calibration_thread, &event);
			if (signal(SIGUSR1, sig_handler) == SIG_ERR) printf("\ncan't catch SIGUSR1\n");
			if (signal(SIGUSR2, sig_handler) == SIG_ERR) printf("\ncan't catch SIGUSR2\n");

			gtk_init (&argc, &argv);

			/*segnali*/
			window = (GObject *) gtk_window_new(GTK_WINDOW_TOPLEVEL);
			layout = (GObject *) gtk_layout_new(NULL, NULL);

		    g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
			/*si esce se viene premuto ESC e altre cose*/
		    g_signal_connect(window, "key-press-event", G_CALLBACK(key_event), NULL);


		    dpy = XOpenDisplay(NULL);
		    s = DefaultScreenOfDisplay(dpy);
			int right_distance = s->width - DEFAULT_DISTANCE_FROM_BORDER - LABEL_SIZE;
			int bottom_distance = s->height - DEFAULT_DISTANCE_FROM_BORDER - LABEL_SIZE;
			point_array[0].default_x = point_array[0].default_y = point_array[1].default_y = point_array[3].default_x = DEFAULT_DISTANCE_FROM_BORDER;
			point_array[1].default_x = point_array[2].default_x = right_distance;
			point_array[2].default_y = point_array[3].default_y = bottom_distance;

			reset();
		    gtk_window_fullscreen( (GtkWindow *) window);
		    gtk_main ();

			while(true) sleep(1);
		}
  }
}
