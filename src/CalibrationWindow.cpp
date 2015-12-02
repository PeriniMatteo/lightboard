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
#include <X11/Xlib.h>
#include "../include/QProgressIndicator.h"
#include "../include/main.h"
extern Screen *s;
extern char *commands[4];
extern ConfigurationWindow *config;
extern int change, point;
extern int reset_point, stop_ir; //boolean

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
