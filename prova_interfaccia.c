#include <gtk/gtk.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <unistd.h>
#include <time.h>





void point_f(){
    wait(3);
    g_print("hai mantenuto il tasto per 3 secondi, complimenti!");
    point++;
    g_print("%d",point);
}

/*fare un controllo sul massimo incremento possibile*/
void change_distance_from_border(gboolean action){
    static int change = 1;
    if(action==SUM){
        change++;
    }
    else if(action==SUBTRACT){
        if(change>1) change--;
    }
    int new_distance = DEFAULT_DISTANCE_FROM_BORDER*change;
    int new_right_distance = s->width - DEFAULT_DISTANCE_FROM_BORDER*change - LABEL_SIZE;
    int new_bottom_distance = s->height - DEFAULT_DISTANCE_FROM_BORDER*change - LABEL_SIZE;
    gtk_layout_move((GtkLayout *)layout,label1, new_distance, new_distance);
    gtk_layout_move((GtkLayout *)layout,label2, new_right_distance, new_distance);
    gtk_layout_move((GtkLayout *)layout,label3, new_right_distance, new_bottom_distance);
    gtk_layout_move((GtkLayout *)layout,label4, new_distance, new_bottom_distance);
}

/*controlla se è stato premuto ESC, funzione di callback del segnale associato*/
static gboolean key_event(GtkWidget *widget, GdkEventKey *event){
    if(event->keyval==GDK_KEY_Escape){
        gtk_main_quit();
        return TRUE;
    }
    if(event->keyval==GDK_KEY_minus){
        change_distance_from_border(SUBTRACT);
        return FALSE;
    }
    if(event->keyval==GDK_KEY_plus){
        change_distance_from_border(SUM);
        return FALSE;
    }
    return FALSE;
}

static gboolean reset(){
    point = 0;
    g_print("%d",point);
    return FALSE;
}

int main (int argc, char *argv[]){
    point = 0;
    gtk_init (&argc, &argv);
    /* Construct a GtkBuilder instance and load our UI description */
    builder = gtk_builder_new ();
    gtk_builder_add_from_file (builder, "spinners.ui", NULL);

    /*segnali*/
    window = gtk_builder_get_object(builder,"window1");
    layout = gtk_builder_get_object(builder, "layout1");
    g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
    /*si esce se viene premuto ESC e altre cose*/
    g_signal_connect(window, "key-press-event", G_CALLBACK(key_event), NULL);
    g_signal_connect(window, "key-release-event", G_CALLBACK(reset), NULL);

    //printf("%d %d", s->height, s->width);
    dpy = XOpenDisplay(NULL);
    s = DefaultScreenOfDisplay(dpy);
    /*creazione label compattare il codice*/
    int right_distance = s->width - DEFAULT_DISTANCE_FROM_BORDER - LABEL_SIZE; //mettere tutti questi valori in variabili
    //queste variabili si potranno poi modificare tramite tastiera
    int bottom_distance = s->height - DEFAULT_DISTANCE_FROM_BORDER - LABEL_SIZE;
    label1 = gtk_label_new("1");
    gtk_widget_set_size_request(label1,LABEL_SIZE,LABEL_SIZE);
    gtk_layout_put((GtkLayout *)layout,label1,DEFAULT_DISTANCE_FROM_BORDER, DEFAULT_DISTANCE_FROM_BORDER);
    label2 = gtk_label_new("2");
    gtk_widget_set_size_request(label2,LABEL_SIZE,LABEL_SIZE);
    gtk_layout_put((GtkLayout *)layout,label2,right_distance,DEFAULT_DISTANCE_FROM_BORDER);
    label3 = gtk_label_new("3");
    gtk_widget_set_size_request(label3,LABEL_SIZE,LABEL_SIZE);
    gtk_layout_put((GtkLayout *)layout,label3,right_distance,bottom_distance);
    label4 = gtk_label_new("4");
    gtk_widget_set_size_request(label4,LABEL_SIZE,LABEL_SIZE);
    gtk_layout_put((GtkLayout *)layout,label4,DEFAULT_DISTANCE_FROM_BORDER,bottom_distance);


    gtk_widget_show_all((GtkWidget *) window);

    gtk_window_fullscreen( (GtkWindow *) window);
    gtk_main ();

    return 0;
}
