/*
** TODO: dividere in più file
** gcc `pkg-config --cflags gtk+-3.0` -o prova prova.c monitor.c core.c -ludev -lX11 -lpthread `pkg-config --libs gtk+-3.0`
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
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/types.h>
#include "xwiimote.h"

static struct xwii_iface *iface;
static pthread_t main_tid;
#define IR_POINTER 1

/*INTERFACCIA*/
#define LABEL_SIZE 20
#define SUM TRUE
#define SUBTRACT FALSE
#define DEFAULT_DISTANCE_FROM_BORDER 30

GObject *layout, *window;
GtkBuilder *builder;
GtkWidget *spinner;
Display *dpy;
Screen *s;
int point, change;
gboolean reset_point, freeze, stop_ir;

typedef struct point_s {
	GtkWidget *label;
	int default_x;
	int default_y;
	int runtime_x;
	int runtime_y;
	int ir_x;
	int ir_y;
} point_s;

point_s point_array[4];

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

static int run_iface(struct xwii_iface *iface, struct xwii_event *event){
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
/*INTERFACCIA*/
/* TODO fare un controllo sul massimo incremento possibile*/
void change_distance_from_border(gboolean action){
    int i;
	if(action==SUM){
        change++;
    }
    else if(action==SUBTRACT){
        if(change>1) change--;
    }
    int new_distance = DEFAULT_DISTANCE_FROM_BORDER*change;
    int new_right_distance = s->width - DEFAULT_DISTANCE_FROM_BORDER*change - LABEL_SIZE;
    int new_bottom_distance = s->height - DEFAULT_DISTANCE_FROM_BORDER*change - LABEL_SIZE;
	point_array[0].runtime_x = point_array[0].runtime_y = point_array[1].runtime_y = point_array[3].runtime_x = new_distance;
	point_array[1].runtime_x = point_array[2].runtime_x = new_right_distance;
	point_array[2].runtime_y = point_array[3].runtime_y = new_bottom_distance;
	for(i=0;i<4;i++) gtk_layout_move((GtkLayout *)layout,point_array[i].label, point_array[i].runtime_x, point_array[i].runtime_y);
}

static gboolean reset(){
    point = 0;
	change = 1;

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
	}

	gtk_widget_show_all((GtkWidget *) window);
    return FALSE;
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

gboolean post_sleep_calibration(){
	if(!reset_point) gtk_widget_destroy((GtkWidget *) spinner);
	else reset();
	stop_ir = FALSE;

	if(point==4){ // TODO eliminare
		printf("%d %d\n",point_array[0].ir_x,point_array[0].ir_y );
		printf("%d %d\n",point_array[1].ir_x,point_array[1].ir_y );
		printf("%d %d\n",point_array[2].ir_x,point_array[2].ir_y );
		printf("%d %d\n",point_array[3].ir_x,point_array[3].ir_y );
		fflush(stdout);
		gtk_widget_destroy((GtkWidget *)window);
	}

	return FALSE;
}

void point_f(struct xwii_event *event){
	freeze = TRUE;
	stop_ir = TRUE;
	reset_point=FALSE; //se dopo il timeout questo reset_point diventa TRUE, viene resettato tutto il processo di calibrazione
	if(point<4){
		spinner = gtk_spinner_new();
		gtk_spinner_start (GTK_SPINNER (spinner));
		gtk_widget_set_size_request(spinner,40,40); //mettere in variabili
		gtk_layout_put((GtkLayout *)layout,spinner,(gint) point_array[point].runtime_x-10,point_array[point].runtime_y-10); /*aggiustamento per centrare il numero nello spinner*/
		gtk_widget_destroy(GTK_WIDGET(point_array[point].label));
		gtk_widget_show(spinner);

		point_array[point].ir_x = event->v.abs[0].x;
		point_array[point].ir_y = event->v.abs[0].y;
		point++;
		g_timeout_add(1000, (GSourceFunc)post_sleep_calibration, NULL);
	}
}

int main(int argc, char **argv){
	struct xwii_event event;
	reset_point=FALSE;

	void sig_handler(int signo){
		if (signo == SIGUSR1){ //IR
			if(!freeze && xwii_event_ir_is_valid(&event.v.abs[0])) point_f(&event);
			//ir_show(&event);
		}
		else if(signo==SIGUSR2){ //no IR
			if(stop_ir) reset_point = TRUE;
			else freeze=FALSE;
		}
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
			/* Construct a GtkBuilder instance and load our UI description */
		    builder = gtk_builder_new ();
		    gtk_builder_add_from_file (builder, "spinners.ui", NULL);

			/*segnali - eliminare builder*/
		    window = gtk_builder_get_object(builder,"window1");
		    layout = gtk_builder_get_object(builder, "layout1");
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
