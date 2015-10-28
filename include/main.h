#ifndef __MAIN_H
#define __MAIN_H

#define LABEL_SIZE 20
#define SUM TRUE
#define SUBTRACT FALSE
#define DEFAULT_DISTANCE_FROM_BORDER 30

typedef struct point_s {
	GtkWidget *label;
	int default_x;
	int default_y;
	int runtime_x;
	int runtime_y;
	int ir_x;
	int ir_y;
} point_s;

char *get_dev(int);
int run_iface(struct xwii_iface *, struct xwii_event *);
void reset();
void calibration_thread(struct xwii_event *);
void change_distance_from_border(gboolean);
void diagonal();
void post_calibration();
gboolean post_sleep_calibration();
void point_f(struct xwii_event *event);

#endif
