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

extern struct xwii_iface *iface;
extern pthread_t main_tid;

extern GObject *layout, *window;
extern GtkBuilder *builder;
extern GtkWidget *spinner;
extern Display *dpy;
extern Screen *s;
extern int point, change;
extern double matrix_A[9][9], matrix_x[9], matrix_res[20];
extern gboolean reset_point, freeze, stop_ir, calibrated;

point_s point_array[4];

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
