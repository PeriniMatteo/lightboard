#include <QApplication>
#include <QtGui>
#include <QWidget>
#include <QPolygonF>
#include <QLabel>
#include <QSystemTrayIcon>
#include <QPushButton>
#include <QGridLayout>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPolygonItem>
#include <QProgressBar>
#include <QCheckBox>
#include <QSlider>
#include <QMenu>
#include <X11/Xlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "../include/xwiimote.h"
#include "../include/main.h"
extern struct xwii_iface *iface;
extern char btaddress[19];
extern int click_enabled, calibrated; //bool
extern int wait_time;
extern coord wii_coord[4];
extern ConfigurationWindow *config;

extern void start_calibration();

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
		command[0]="hidd";
		command[1]="--kill";
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
		connect(slider, SIGNAL(valueChanged(int)), this, SLOT(changeSensibility()));
		slider->setValue(50);
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

void ConfigurationWindow::changeSensibility(){
	::wait_time = 200 + slider->value()*6; //TODO hardcoded
}

void ConfigurationWindow::setPolygon(){
	int i;
	polygon = new QPolygonF(4);
	for(i=0;i<4;i++){
		int polygon_x = (wii_coord[i].x*view->width())/1024;
		int polygon_y = (wii_coord[i].y*view->height())/768;
		*polygon << QPointF(polygon_x,polygon_y);
	}
	QGraphicsPolygonItem *polygonItem = new QGraphicsPolygonItem(*polygon);
	scene->addItem(polygonItem);
	polygonItem->setPen( QPen(Qt::darkGreen) );
	polygonItem->setBrush( Qt::yellow );
	config->resize(351, 400); //TODO hardcoded
}
