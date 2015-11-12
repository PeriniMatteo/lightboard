#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QKeyEvent>
#include <QTimer>
#include <QString>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <X11/Xlib.h>
#include "../include/xwiimote.h"
#include "../include/main.h"

struct xwii_iface *iface;
struct xwii_event event;
Display *dpy;
Screen *s;
pthread_t main_tid;
int point, change; //interi
double matrix_A[9][9], matrix_x[9], matrix_res[20];
int reset_point, freeze, stop_ir, calibrated; //TODO booleani
CalibrationWindow *window;
char *command[4] = {FIRST_COMMAND, SECOND_COMMAND, THIRD_COMMAND, FOURTH_COMMAND};

CalibrationWindow::CalibrationWindow(QWidget *parent) : QWidget(parent) {
    int i;
    for(i=0;i<4;i++) point_array[i].label = new QLabel(QString::number(i+1), this);
    instruction = new QLabel(command[0], this);
    instruction->setGeometry((s->width/2)-250, (s->height/2)-250, 500, 500);
}

void CalibrationWindow::keyPressEvent(QKeyEvent *event) {
	if (event->key() == Qt::Key_Escape) {
		qApp->quit();
		QObject::deleteLater();
		exit(1);
	}
	if(event->key() == Qt::Key_Plus){
		change_distance_from_border(SUM);
	}
	if(event->key() == Qt::Key_Minus){
		change_distance_from_border(SUBTRACT);
	}
	if(event->key() == Qt::Key_R){
        change = 1;
		reset();
	}
}

void CalibrationWindow::setTimer() {
    QTimer::singleShot(1000, this, SLOT(post_sleep_calibration()));
}

void CalibrationWindow::post_sleep_calibration(){
    if(!reset_point) {
        instruction->setText(command[point]);
    }
	else reset();
	stop_ir = FALSE;
	if(point==4){
		qApp->quit();
        QObject::deleteLater();
        post_calibration();
	}
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

void reset(){
	int i;
	point = 0;
	for(i=0;i<4;i++){
		window->point_array[i].label->setGeometry(window->point_array[i].default_x,window->point_array[i].default_y , LABEL_SIZE, LABEL_SIZE);
		window->point_array[i].runtime_x = window->point_array[i].default_x;
		window->point_array[i].runtime_y = window->point_array[i].default_y;
        window->point_array[i].label->show();
	}
}

void sig_handler(int signo){
	if(!calibrated){
		if (signo==SIGUSR1){ //IR
			if(!freeze && xwii_event_ir_is_valid(&event.v.abs[0])) point_f(&event);
		}
		else if(signo==SIGUSR2){ //no IR
			if(stop_ir) reset_point = TRUE;
			else freeze=FALSE;
		}
	}
}

int main(int argc, char *argv[]) {
    int ret = 0;
	change = 1;
 	char *path = NULL;
    if(argc < 2) {
        printf("usage: min 1 parameter \n");
        exit(1);
    }
    if (argv[1][0] != '/') path = get_dev(atoi(argv[1]));

    ret = xwii_iface_new(&iface, path ? path : argv[1]);
    free(path);
    if (ret) printf("Cannot create xwii_iface '%s' err:%d\n", argv[1], ret);
    else {
        ret = xwii_iface_open(iface, xwii_iface_available(iface) | XWII_IFACE_WRITABLE);
        if (ret) printf("Error: Cannot open interface: %d, use SUDO!", ret);
        /*remove accelerometer and motion plus messages from the interface*/
        if (xwii_iface_opened(iface) & XWII_IFACE_ACCEL) xwii_iface_close(iface, XWII_IFACE_ACCEL);
        if (xwii_iface_opened(iface) & XWII_IFACE_MOTION_PLUS) xwii_iface_close(iface, XWII_IFACE_MOTION_PLUS);

		pthread_t tid;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		main_tid = pthread_self();
		pthread_create(&tid,&attr, calibration_thread,(void*)&event);
		if (signal(SIGUSR1, sig_handler) == SIG_ERR) printf("\ncan't catch SIGUSR1\n");
		if (signal(SIGUSR2, sig_handler) == SIG_ERR) printf("\ncan't catch SIGUSR2\n");

        dpy = XOpenDisplay(NULL);
		s = DefaultScreenOfDisplay(dpy);
		QApplication app(argc, argv);
		window = new CalibrationWindow();

		int right_distance = s->width - DEFAULT_DISTANCE_FROM_BORDER - LABEL_SIZE;
		int bottom_distance = s->height - DEFAULT_DISTANCE_FROM_BORDER - LABEL_SIZE;
		window->point_array[0].default_x = window->point_array[0].default_y = window->point_array[1].default_y = window->point_array[3].default_x = DEFAULT_DISTANCE_FROM_BORDER;
		window->point_array[1].default_x = window->point_array[2].default_x = right_distance;
		window->point_array[2].default_y = window->point_array[3].default_y = bottom_distance;
		reset();
		window->resize(400, 400);
		window->setWindowTitle("CalibrationWindow");
		window->showFullScreen();
		app.exec();
		pthread_join(tid, NULL); //TODO !!!
	}
}
