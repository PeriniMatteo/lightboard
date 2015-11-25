#include <QTimer>
#include <QObject>
#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include "../include/xwiimote.h"
#include "../include/main.h"
#include "../include/QProgressIndicator.h"
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

extern struct xwii_iface *iface;
extern pthread_t main_tid;
extern CalibrationWindow *window;
extern int point, change;
extern Display *dpy;
extern Screen *s;
extern double matrix_A[9][9], matrix_x[9], matrix_res[20];
extern int reset_point, freeze, stop_ir, calibrated, click_enabled;

static int run_iface(struct xwii_iface *iface, struct xwii_event *event){
	int ret=0, fds_num, valid_source;
	struct pollfd fds[2];
	int default_wait_time = 400;
	static int go = 1;

	memset(fds, 0, sizeof(fds));
	fds[0].fd = 0;
	fds[0].events = POLLIN;
	fds[1].fd = xwii_iface_get_fd(iface);
	fds[1].events = POLLIN;
	fds_num = 2;
	ret = xwii_iface_watch(iface, true);
	if(ret) printf("error xwii_iface_watch");

	while(true){
		ret = poll(fds, fds_num, default_wait_time);
		if(ret<0){
			if(errno != EINTR){
				ret = -errno;
				printf("error poll");
				break;
			}
		}
		else if(!ret) pthread_kill(main_tid, SIGUSR2); //no IR, send signal to main thread

		ret = xwii_iface_dispatch(iface, event, sizeof(*event));

		if(ret){
			if(ret !=-EAGAIN){
				printf("error xwii_iface_dispatch");
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
					if(!calibrated)
						pthread_kill(main_tid,SIGUSR1); //send signal to the main thread
					else{
						float new_x,new_y;
						valid_source = 0;
						if(xwii_event_ir_is_valid(&event->v.abs[0])){
							valid_source = 1;
							new_x = ((matrix_res[0]*event->v.abs[0].x) + (matrix_res[1]*event->v.abs[0].y) + matrix_res[2]) /
									((matrix_res[6]*event->v.abs[0].x) + (matrix_res[7]*event->v.abs[0].y) + 1);
							new_y = ((matrix_res[3]*event->v.abs[0].x) + (matrix_res[4]*event->v.abs[0].y) + matrix_res[5]) /
									((matrix_res[6]*event->v.abs[0].x) + (matrix_res[7]*event->v.abs[0].y) + 1);

							XTestFakeMotionEvent (dpy, 0, new_x, new_y, CurrentTime);
							XSync(dpy, 0);

							if(go && click_enabled){
								XTestFakeButtonEvent (dpy, 1, True,  CurrentTime);
								XSync(dpy, 1);
								go=0;
							}
						}
						if (!valid_source) {
							if(!go){
								XTestFakeButtonEvent (dpy, 1, False,  CurrentTime);
								XSync(dpy, 1);
								go=1;
							}
						}
					}
					break;
				default: printf("error");
			}
		}
	}
}

void *calibration_thread(void *event_p){
	int ret = run_iface(iface, (struct xwii_event *)event_p);
	xwii_iface_unref(iface);
	if (ret) {
		printf("Program failed");
		pthread_exit(&ret);
	}
}

/* TODO make a control about maximum increasing*/
void change_distance_from_border(int action){
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
	window->point_array[0].runtime_x = window->point_array[0].runtime_y = window->point_array[1].runtime_y = window->point_array[3].runtime_x = new_distance;
	window->point_array[1].runtime_x = window->point_array[2].runtime_x = new_right_distance;
	window->point_array[2].runtime_y = window->point_array[3].runtime_y = new_bottom_distance;
	for(i=0;i<4;i++) window->point_array[i].label->setGeometry(window->point_array[i].runtime_x, window->point_array[i].runtime_y, LABEL_SIZE, LABEL_SIZE);
}

void point_f(struct xwii_event *event){
	freeze = stop_ir = TRUE;
	reset_point = FALSE; //if after timeout this variable is set to TRUE, reset()

	if(point<4){
		window->spinners[point]->setGeometry(window->point_array[point].runtime_x - LABEL_SIZE+5,window->point_array[point].runtime_y - LABEL_SIZE/2 , LABEL_SIZE*2, LABEL_SIZE*2); //TODO adjust constant
        window->spinners[point]->startAnimation();
        window->spinners[point]->show();
		window->point_array[point].label->hide();

		window->point_array[point].ir_x = event->v.abs[0].x;
		window->point_array[point].ir_y = event->v.abs[0].y;
		point++;
		window->setTimer(); //sets a 1'' timeout, then calls post_sleep_calibration function
	}
}

static void diagonal(){
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
	calibrated = TRUE;

	//matrices creation
	for(i=0;i<8;i=i+2){
		matrix_A[i][0] = window->point_array[i/2].ir_x;
		matrix_A[i][1] = window->point_array[i/2].ir_y;
		matrix_A[i][2] = 1;
		matrix_A[i][3] = matrix_A[i][4] = matrix_A[i][5] = 0;
		matrix_A[i][6] = -(window->point_array[i/2].runtime_x * window->point_array[i/2].ir_x);
		matrix_A[i][7] = -(window->point_array[i/2].runtime_x * window->point_array[i/2].ir_y);

		matrix_A[i+1][0] = matrix_A[i+1][1] = matrix_A[i+1][2] = 0;
		matrix_A[i+1][3] = window->point_array[i/2].ir_x;
		matrix_A[i+1][4] = window->point_array[i/2].ir_y;
		matrix_A[i+1][5] = 1;
		matrix_A[i+1][6] = -(window->point_array[i/2].runtime_y * window->point_array[i/2].ir_x);
		matrix_A[i+1][7] = -(window->point_array[i/2].runtime_y * window->point_array[i/2].ir_y);

		matrix_x[i] = window->point_array[i/2].runtime_x;
		matrix_x[i+1] = window->point_array[i/2].runtime_y;
	}

	/*http://www.alexeypetrov.narod.ru/Eng/C/gauss_about.html*/
	/*linear system calculation*/
	diagonal();
   	//process rows
	for(k=0; k<8; k++){
		for(i=k+1; i<8; i++){
			if(matrix_A[k][k]==0){
				printf("\nSolution is not exist.\n"); /*TODO insert an error system*/
				return;
			}
			float M = matrix_A[i][k] / matrix_A[k][k];
			for(j=k; j<8; j++)
				matrix_A[i][j] -= M * matrix_A[k][j];
			matrix_x[i] -= M*matrix_x[k];
		}
	}
	for(i=0;i<8;i++) matrix_res[i] = 0;
	for(i=8-1; i>=0; i--){
		float s = 0;
		for(j = i; j<8; j++)
			s = s+matrix_A[i][j]*matrix_res[j];
		matrix_res[i] = (matrix_x[i] - s) / matrix_A[i][i];
	}
}
