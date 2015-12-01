#ifndef __MAIN_H
#define __MAIN_H
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
#include <QProgressBar>
#include <QCheckBox>
#include <QSlider>
#include "../include/QProgressIndicator.h"

#define LABEL_SIZE 20
#define SUM TRUE
#define SUBTRACT FALSE
#define DEFAULT_DISTANCE_FROM_BORDER 30
#define FALSE 0
#define TRUE 1
#define AREA_WIIMOTE_IR_CAMERA 786432
#define FIRST_COMMAND "Point your IR pen on the number 1 and hold for 1 second"
#define SECOND_COMMAND "Now release your IR pen and point it on the number 2 and hold for 1 second"
#define THIRD_COMMAND "Now repeat for number 3"
#define FOURTH_COMMAND "Finally number 4"
typedef struct coord {
	int x;
	int y;
} coord;

typedef struct point_s {
	QLabel *label;
	int default_x;
	int default_y;
	int runtime_x;
	int runtime_y;
	int ir_x;
	int ir_y;
} point_s;

class CalibrationWindow : public QWidget {
	Q_OBJECT
	public:
		point_s point_array[4];
		QLabel *instruction;
		QProgressIndicator *spinners[4];
		CalibrationWindow(QWidget *parent = 0);
		void keyPressEvent(QKeyEvent * e);
		void setTimer();
	private slots:
    	void post_sleep_calibration();
};

class ConfigurationWindow : public QWidget {
	Q_OBJECT
	public:
		QGridLayout *gridLayout;
		QLabel *btAddressLabel;
		QLabel *btAddressValue;
		QPushButton *calibrateButton;
		QLabel *coverageLabel;
		QLabel *coverageValue;
		QGraphicsScene *scene;
		QGraphicsView *view;
		QPolygonF *polygon;
		QLabel *batteryLabel;
		QProgressBar *batteryValue;
		QCheckBox *checkbox;
		QLabel *sensibilityLabel;
		QSlider *slider;
		ConfigurationWindow(QWidget *parent = 0);
	private:
		QAction *openConfigurationAction;
		QAction *informationAction;
	    QAction *quitAction;
		QMenu *trayIconMenu;
		QSystemTrayIcon *trayIcon;
		void createActions();
	    void createTrayIcon();
		void setIcon();
		void setPolygon();
	private slots:
		void openConfiguration();
		void information();
		void startCalibration();
		void changeMode();
		void exitApp();
};

void reset();
void *calibration_thread(void *);
void change_distance_from_border(int); //TODO booleano
void point_f(struct xwii_event *event);
void post_calibration();

#endif
