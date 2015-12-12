#include <QApplication>
#include <QtGui>
#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPolygonItem>
#include <QPolygonF>
#include <QVector>
#include <QLabel>
#include <QKeyEvent>
#include <QTimer>
#include <QString>
#include <QMenu>
#include <QLocale>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <cmath>
#include <X11/Xlib.h>
#include <QTranslator>

#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "../include/xwiimote.h"
#include "../include/main.h"
#include "../include/QProgressIndicator.h"

struct xwii_iface *iface;
struct xwii_event event;
Display *dpy;
Screen *s;
pthread_t main_tid;
int point, change;
coord wii_coord[4];
double matrix_A[9][9], matrix_x[9], matrix_res[20];
int reset_point, freeze, stop_ir, calibrated, click_enabled; //TODO booleani

CalibrationWindow *window;
ConfigurationWindow *config;
char *commands[4] = {FIRST_COMMAND, SECOND_COMMAND, THIRD_COMMAND, FOURTH_COMMAND};
char btaddress[19];

void start_calibration(){
	freeze = stop_ir = calibrated = FALSE;
	change = 1;
	window = new CalibrationWindow();

	int right_distance = s->width - DEFAULT_DISTANCE_FROM_BORDER - LABEL_SIZE;
	int bottom_distance = s->height - DEFAULT_DISTANCE_FROM_BORDER - LABEL_SIZE;
	window->point_array[0].default_x = window->point_array[0].default_y = window->point_array[1].default_y = window->point_array[3].default_x = DEFAULT_DISTANCE_FROM_BORDER;
	window->point_array[1].default_x = window->point_array[2].default_x = right_distance;
	window->point_array[2].default_y = window->point_array[3].default_y = bottom_distance;
	reset();
	window->resize(400, 400);
	window->setWindowTitle(QObject::tr("CalibrationWindow"));
	window->showFullScreen();
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
	window->instruction->setText(commands[0]);
	for(i=0;i<4;i++){
		window->point_array[i].label->setGeometry(window->point_array[i].default_x,window->point_array[i].default_y , LABEL_SIZE, LABEL_SIZE);
		window->point_array[i].runtime_x = window->point_array[i].default_x;
		window->point_array[i].runtime_y = window->point_array[i].default_y;
        window->point_array[i].label->show();
        window->spinners[i]->hide();
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

void inquiry(char *addr){
	inquiry_info *ii = NULL;
    int max_rsp = 255, num_rsp = 0;
    int dev_id, sock, len, flags;
    int i;
    char name[248] = { 0 };

    dev_id = hci_get_route(NULL);
    sock = hci_open_dev( dev_id );
    if (dev_id < 0 || sock < 0) {
        perror("opening socket");
        exit(1);
    }

    len  = 8;
    flags = IREQ_CACHE_FLUSH;
    ii = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));
	while(num_rsp==0) num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &ii, flags);
    if( num_rsp < 0 ) perror("hci_inquiry");

    for (i = 0; i < num_rsp; i++) {
        ba2str(&(ii+i)->bdaddr, addr);
        memset(name, 0, sizeof(name));
        if (hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(name), name, 0) < 0) strcpy(name, "[unknown]");
		if(!strcmp(name, "Nintendo RVL-CNT-01") || !strcmp(name, "Nintendo RVL-CNT-01-TR")) break;
    }

    free( ii );
	close( sock );
}

static void set_language(QTranslator *t){
	if(!strcmp(QLocale::system().name().toLatin1().data() , "it_IT"))
		t->load("resources/lightboard_it");
	else
		t->load("resources/lightboard_en");
}

int main(int argc, char *argv[]) {
    int ret = click_enabled = 0;
 	char *path = NULL;
	int status;
	char* command[5];
	command[0]="hidd";
	command[1]="-c";
	command[2]=(char *) malloc(sizeof(char)*19);
	command[3]=NULL;
	printf("Press sync button on Wiimote.\n");fflush(stdout);
	inquiry(command[2]);
	if(fork()==0) execvp(command[0],command);
	else wait(&status);

	strcpy(btaddress, command[2]);
	sleep(1);
	path = get_dev(1);
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
		if (signal(SIGUSR1, sig_handler) == SIG_ERR) printf("can't catch SIGUSR1\n");
		if (signal(SIGUSR2, sig_handler) == SIG_ERR) printf("can't catch SIGUSR2\n");

        dpy = XOpenDisplay(NULL);
		s = DefaultScreenOfDisplay(dpy);
        Q_INIT_RESOURCE(systray);
		QApplication app(argc, argv);
		QTranslator translator;
		set_language(&translator);
		app.installTranslator(&translator);

        if (!QSystemTrayIcon::isSystemTrayAvailable()) {
            return 1;
        }
        QApplication::setQuitOnLastWindowClosed(false);
		start_calibration();
		app.exec();
		pthread_join(tid, NULL);
	}
}
