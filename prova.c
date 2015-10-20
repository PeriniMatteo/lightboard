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
static bool freeze = false;
static pthread_t main_tid;
#define IR_POINTER 1

/*INTERFACCIA*/
#define LABEL_SIZE 20
#define SUM TRUE
#define SUBTRACT FALSE
#define DEFAULT_DISTANCE_FROM_BORDER 30

GObject *layout, *window;
GtkBuilder *builder;
GtkWidget *label1, *label2, *label3, *label4, *spinner;
Display *dpy;
Screen *s;
int point, change;

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
		else printf("NO IR");
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
/*fare un controllo sul massimo incremento possibile*/
void change_distance_from_border(gboolean action){
    if(action==SUM){
        change++;
    }
    else if(action==SUBTRACT){
        if(change>1) change--;
    }
    int new_distance = DEFAULT_DISTANCE_FROM_BORDER*change;
    int new_right_distance = s->width - DEFAULT_DISTANCE_FROM_BORDER*change - LABEL_SIZE;
    int new_bottom_distance = s->height - DEFAULT_DISTANCE_FROM_BORDER*change - LABEL_SIZE;
    gtk_layout_move((GtkLayout *)layout,label1, new_distance, new_distance);
    gtk_layout_move((GtkLayout *)layout,label2, new_right_distance, new_distance);
    gtk_layout_move((GtkLayout *)layout,label3, new_right_distance, new_bottom_distance);
    gtk_layout_move((GtkLayout *)layout,label4, new_distance, new_bottom_distance);
}

static gboolean reset(){
    point = 0;
	change = 1;
    //g_print("%d",point);

	//gtk_container_foreach((GtkContainer *)layout,(GtkCallback)gtk_widget_destroy, NULL);
	gtk_widget_destroy((GtkWidget *) layout);
	layout = (GObject *) gtk_layout_new(NULL, NULL);
	gtk_container_add (GTK_CONTAINER (window), (GtkWidget *) layout);
	int right_distance = s->width - DEFAULT_DISTANCE_FROM_BORDER - LABEL_SIZE; //mettere tutti questi valori in variabili
	//queste variabili si potranno poi modificare tramite tastiera
	int bottom_distance = s->height - DEFAULT_DISTANCE_FROM_BORDER - LABEL_SIZE;
	label1 = gtk_label_new("1");
	gtk_widget_set_size_request(label1,LABEL_SIZE,LABEL_SIZE);
	gtk_layout_put((GtkLayout *)layout,label1,DEFAULT_DISTANCE_FROM_BORDER, DEFAULT_DISTANCE_FROM_BORDER);
	label2 = gtk_label_new("2");
	gtk_widget_set_size_request(label2,LABEL_SIZE,LABEL_SIZE);
	gtk_layout_put((GtkLayout *)layout,label2,right_distance,DEFAULT_DISTANCE_FROM_BORDER);
	label3 = gtk_label_new("3");
	gtk_widget_set_size_request(label3,LABEL_SIZE,LABEL_SIZE);
	gtk_layout_put((GtkLayout *)layout,label3,right_distance,bottom_distance);
	label4 = gtk_label_new("4");
	gtk_widget_set_size_request(label4,LABEL_SIZE,LABEL_SIZE);
	gtk_layout_put((GtkLayout *)layout,label4,DEFAULT_DISTANCE_FROM_BORDER,bottom_distance);

	gtk_widget_show_all((GtkWidget *) window);

    return FALSE;
}

/*controlla se è stato premuto ESC, funzione di callback del segnale associato*/
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

void point_f(){
	GValue x = G_VALUE_INIT;
	g_value_init (&x, G_TYPE_INT);
	GValue y = G_VALUE_INIT;
	g_value_init (&y, G_TYPE_INT);
	point++;
	switch(point){
		case 1:
			spinner = gtk_spinner_new ();
			printf("n");
			gtk_spinner_start (GTK_SPINNER (spinner));
			gtk_container_child_get_property((GtkContainer *)layout,label1,"x", &x);
			gtk_container_child_get_property((GtkContainer *)layout,label1,"y", &y);
			gtk_widget_set_size_request(spinner,40,40);
			gtk_layout_put((GtkLayout *)layout,spinner,(gint) g_value_get_int(&x)-10,(gint) g_value_get_int(&y)-10); /*aggiustamento per centrare il numero nello spinner*/
			gtk_widget_destroy(GTK_WIDGET(label1));
			gtk_widget_show(spinner);
			break;
		case 100:
			g_print("primo!");
			gtk_widget_destroy(GTK_WIDGET(spinner));
			break;
		case 200:
			g_print("secondo!");
			gtk_widget_destroy(GTK_WIDGET(label2));
			break;
		case 300:
			g_print("terzo!");
			gtk_widget_destroy(GTK_WIDGET(label3));
			break;
		case 400:
			g_print("quarto!");
			gtk_widget_destroy(GTK_WIDGET(label4));
	}
}

int main(int argc, char **argv){
	struct xwii_event event;

	void sig_handler(int signo){
		if (signo == SIGUSR1){
			point_f();
			//ir_show(&event);
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
			if (xwii_iface_opened(iface) & XWII_IFACE_ACCEL)
				xwii_iface_close(iface, XWII_IFACE_ACCEL);
			if (xwii_iface_opened(iface) & XWII_IFACE_MOTION_PLUS)
				xwii_iface_close(iface, XWII_IFACE_MOTION_PLUS);

			pthread_t tid;
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			main_tid = pthread_self();
			pthread_create(&tid,&attr,(void*)calibration_thread, &event);
			if (signal(SIGUSR1, sig_handler) == SIG_ERR) printf("\ncan't catch SIGINT\n");

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

			//printf("%d %d", s->height, s->width);
		    dpy = XOpenDisplay(NULL);
		    s = DefaultScreenOfDisplay(dpy);
		    /*creazione label compattare il codice*/

			reset();
		    gtk_window_fullscreen( (GtkWindow *) window);
		    gtk_main ();

			while(true) sleep(1);
		}
  }
}
