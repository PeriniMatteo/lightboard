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

static struct xwii_iface *iface;
static pthread_t main_tid;

GObject *layout, *window;
GtkBuilder *builder;
GtkWidget *spinner;
Display *dpy;
Screen *s;
int point, change;
double matrix_A[9][9], matrix_x[9], matrix_res[20];
gboolean reset_point, freeze, stop_ir, calibrated;

point_s point_array[4];

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

static void reset(){
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
	reset();
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

void diagonal(){
	int i, j, k;
	float temp=0;
	for(i=0; i<8; i++){
		if(matrix_A[i][i]==0){
			for(j=0; j<8; j++){
				if(j==i) continue;
				if(matrix_A[j][i] !=0 && matrix_A[i][j]!=0){
					for(k=0; k<8; k++){
						temp = matrix_A[j][k];
						matrix_A[j][k] = matrix_A[i][k];
						matrix_A[i][k] = temp;
					}
					temp = matrix_x[j];
					matrix_x[j] = matrix_x[i];
					matrix_x[i] = temp;
					break;
				}
			}
		}
	}
}

void post_calibration(){
	int i, j, k;
	printf("coordinate IR\n");
	printf("%d %d\n",point_array[0].ir_x,point_array[0].ir_y );
	printf("%d %d\n",point_array[1].ir_x,point_array[1].ir_y );
	printf("%d %d\n",point_array[2].ir_x,point_array[2].ir_y );
	printf("%d %d\n",point_array[3].ir_x,point_array[3].ir_y );
	printf("coordinate schermo\n");
	printf("%d %d\n",point_array[0].runtime_x,point_array[0].runtime_y );
	printf("%d %d\n",point_array[1].runtime_x,point_array[1].runtime_y );
	printf("%d %d\n",point_array[2].runtime_x,point_array[2].runtime_y );
	printf("%d %d\n",point_array[3].runtime_x,point_array[3].runtime_y );
	fflush(stdout);
	calibrated = TRUE;
	gtk_widget_destroy((GtkWidget *)window);

	//creazione matrice
	for(i=0;i<8;i=i+2){
		matrix_A[i][0] = point_array[i/2].ir_x;
		matrix_A[i][1] = point_array[i/2].ir_y;
		matrix_A[i][2] = 1;
		matrix_A[i][3] = matrix_A[i][4] = matrix_A[i][5] = 0;
		matrix_A[i][6] = -(point_array[i/2].runtime_x * point_array[i/2].ir_x);
		matrix_A[i][7] = -(point_array[i/2].runtime_x * point_array[i/2].ir_y);

		matrix_A[i+1][0] = matrix_A[i+1][1] = matrix_A[i+1][2] = 0;
		matrix_A[i+1][3] = point_array[i/2].ir_x;
		matrix_A[i+1][4] = point_array[i/2].ir_y;
		matrix_A[i+1][5] = 1;
		matrix_A[i+1][6] = -(point_array[i/2].runtime_y * point_array[i/2].ir_x);
		matrix_A[i+1][7] = -(point_array[i/2].runtime_y * point_array[i/2].ir_y);

		matrix_x[i] = point_array[i/2].runtime_x;
		matrix_x[i+1] = point_array[i/2].runtime_y;
	}
	printf("matrice A | matrice x\n");
	for(i=0;i<8;i++){
		for(j=0;j<8;j++){
			printf("%d ", (int)matrix_A[i][j]);
		}
		printf("| %d", (int)matrix_x[i]);
		printf("\n");
	}
	/*http://www.alexeypetrov.narod.ru/Eng/C/gauss_about.html*/
	/*calcolo matrice*/
	diagonal();
   	//process rows
	for(k=0; k<8; k++){
		for(i=k+1; i<8; i++){
			if(matrix_A[k][k]==0){
				printf("\nSolution is not exist.\n");
				return;
			}
			float M = matrix_A[i][k] / matrix_A[k][k];
			for(j=k; j<8; j++)
				matrix_A[i][j] -= M * matrix_A[k][j];
			matrix_x[i] -= M*matrix_x[k];
		}
	}
	for(i=8-1; i>=0; i--){
		float s = 0;
		for(j = i; j<8; j++)
			s = s+matrix_A[i][j]*matrix_res[j];
		matrix_res[i] = (matrix_x[i] - s) / matrix_A[i][i];
	}
	printf("Result:\n");
	for(i=0; i<8; i++){
		printf("X%d = %lf\n", i, matrix_res[i]);
	}
	fflush(stdout);
}

gboolean post_sleep_calibration(){
	if(!reset_point) gtk_widget_destroy((GtkWidget *) spinner);
	else reset();
	stop_ir = FALSE;
	if(point==4) post_calibration();
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
			/* Construct a GtkBuilder instance and load our UI description */
		    builder = gtk_builder_new ();
		    gtk_builder_add_from_file (builder, "../spinners.ui", NULL);

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
