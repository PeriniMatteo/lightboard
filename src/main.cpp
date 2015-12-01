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
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <cmath>
#include <X11/Xlib.h>

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

static uint8_t get_battery(void)
{
	uint8_t capacity;
	if (xwii_iface_get_battery(iface, &capacity)) printf("Error: Cannot read battery capacity");
	else return capacity;
}

static char *get_coverage(void){
	if(calibrated){
		float distance_01 = sqrt( pow((wii_coord[1].x - wii_coord[0].x),2) + pow((wii_coord[1].y - wii_coord[0].y),2) );
		float distance_03 = sqrt( pow((wii_coord[3].x - wii_coord[0].x),2) + pow((wii_coord[3].y - wii_coord[0].y),2) );
		float area_013 = (distance_01*distance_03)/2;
		float distance_23 = sqrt( pow((wii_coord[3].x - wii_coord[2].x),2) + pow((wii_coord[3].y - wii_coord[2].y),2) );
		float distance_12 = sqrt( pow((wii_coord[2].x - wii_coord[1].x),2) + pow((wii_coord[2].y - wii_coord[1].y),2) );
		float area_123 = (distance_23*distance_12)/2;
		float area = area_013+area_123;
		char coverage[4];
		snprintf(coverage, sizeof(coverage), "%d%%", (int)(area*100)/AREA_WIIMOTE_IR_CAMERA);
		return coverage;
	}
	else return "0%";
}

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

CalibrationWindow::CalibrationWindow(QWidget *parent) : QWidget(parent) {
    int i;
    for(i=0;i<4;i++) point_array[i].label = new QLabel(QString::number(i+1), this);
    for(i=0;i<4;i++) spinners[i] = new QProgressIndicator(this);
    instruction = new QLabel(commands[0], this);
    instruction->setGeometry((s->width/2)-250, (s->height/2)-250, 500, 500);
}

void CalibrationWindow::keyPressEvent(QKeyEvent *event) {
	if (event->key() == Qt::Key_Escape) {
		close();
        config = new ConfigurationWindow();
        config->resize(350, 400);
        config->setWindowTitle(tr("ConfigurationWindow"));
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
        instruction->setText(commands[point]);
        spinners[point-1]->hide();
    }
	else{
		change = 1;
		reset();
	}
	stop_ir = FALSE;
	if(point==4){
		post_calibration();
		close();
        config = new ConfigurationWindow();
        config->resize(350, 400);
        config->setWindowTitle(tr("ConfigurationWindow"));
	}
}

void ConfigurationWindow::createActions(){
    openConfigurationAction = new QAction(tr("Configuration"), this);
    connect(openConfigurationAction, SIGNAL(triggered()), this, SLOT(openConfiguration()));

    informationAction = new QAction(tr("Information"), this);
    connect(informationAction, SIGNAL(triggered()), this, SLOT(information()));

    quitAction = new QAction(tr("Quit"), this);
    connect(quitAction, SIGNAL(triggered()), this, SLOT(exitApp()));
}
void ConfigurationWindow::exitApp(){
    qApp->quit();
    QObject::deleteLater();
	xwii_iface_unref(iface);
	char* command[5];
	command[0]="bt-input";
	command[1]="-d";
	command[2]=btaddress;
	command[3]=NULL;
	if(fork()==0) execvp(command[0],command);
	else wait((void *)NULL);
    exit(1);
}
void ConfigurationWindow::createTrayIcon()
{
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(openConfigurationAction);
    trayIconMenu->addAction(informationAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
}
void ConfigurationWindow::setIcon(){
     QIcon icon = QIcon(":/images/icon.png");
     trayIcon->setIcon(icon);
     setWindowIcon(icon);
}
ConfigurationWindow::ConfigurationWindow(QWidget *parent) : QWidget(parent) {
    gridLayout = new QGridLayout();
    btAddressLabel = new QLabel(tr("Connected to:"));
    btAddressValue = new QLabel(btaddress);
    calibrateButton = new QPushButton(tr("Calibrate"),this);
	connect(calibrateButton, SIGNAL(clicked()), this, SLOT(startCalibration()));
    coverageLabel = new QLabel(tr("Coverage:"));
    coverageValue = new QLabel();
	coverageValue->setText(get_coverage());
    scene = new QGraphicsScene(this);
    view = new QGraphicsView(this);
	view->setScene(scene);
	batteryLabel = new QLabel(tr("Battery:"));
    batteryValue = new QProgressBar();
    batteryValue->setMinimum(0);
    batteryValue->setMaximum(100);
    batteryValue->setValue(get_battery());
    checkbox = new QCheckBox(tr("Move only"), this);
    checkbox->setChecked(true);
	connect(checkbox, SIGNAL(clicked(bool)), this, SLOT(changeMode()));
    sensibilityLabel = new QLabel(tr("Sensibility:"));
    slider = new QSlider(Qt::Horizontal,0);
    gridLayout->setVerticalSpacing(10);
    gridLayout->setRowStretch(3, 10);

    gridLayout->addWidget(btAddressLabel,0,0,1,1);
    gridLayout->addWidget(btAddressValue,0,1,1,1);
    gridLayout->addWidget(calibrateButton,1,0,1,2);
    gridLayout->addWidget(coverageLabel,2,0,1,1);
    gridLayout->addWidget(coverageValue,2,1,1,1);
    gridLayout->addWidget(view,3,0,3,2);
    gridLayout->addWidget(batteryLabel,6,0,1,1);
    gridLayout->addWidget(batteryValue,6,1,1,1);
    gridLayout->addWidget(checkbox,7,0,1,1);
    gridLayout->addWidget(sensibilityLabel,8,0,1,1);
    gridLayout->addWidget(slider,8,1,1,1);

    setLayout(gridLayout);
    createActions(); /*create callback function for menu fields*/
    createTrayIcon(); /*create tray icon*/
    setIcon();
    trayIcon->show();
}

void ConfigurationWindow::information(){}
void ConfigurationWindow::openConfiguration(){
    show();
	setPolygon();
}

void ConfigurationWindow::startCalibration(){
	::start_calibration();
	trayIcon->~QSystemTrayIcon();
	close();
}

void ConfigurationWindow::changeMode(){
	::click_enabled = 1-::click_enabled;
}

void ConfigurationWindow::setPolygon(){
	int i;
	polygon = new QPolygonF(4);
	for(i=0;i<4;i++){
		int provax = (wii_coord[i].x*view->width())/1024;
		int provay = (wii_coord[i].y*view->height())/768;
		polygon->append(QPointF(provax,provay));
	}
	QGraphicsPolygonItem *polygonItem = new QGraphicsPolygonItem(*polygon, 0, scene);
  	polygonItem->setPen( QPen(Qt::darkGreen) );
  	polygonItem->setBrush( Qt::yellow );
	config->resize(351, 400); //TODO hardcoded
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

int main(int argc, char *argv[]) {
    int ret = click_enabled = 0;
 	char *path = NULL;
	int status;
	char* command[5];
	command[0]="bt-input";
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
        if (!QSystemTrayIcon::isSystemTrayAvailable()) {
            /*QMessageBox::critical(0, QObject::tr("Systray"),
                               QObject::tr("I couldn't detect any system tray "
                                           "on this system."));*/
            return 1;
        }
        QApplication::setQuitOnLastWindowClosed(false);
		start_calibration();
		app.exec();
		pthread_join(tid, NULL);
	}
}
