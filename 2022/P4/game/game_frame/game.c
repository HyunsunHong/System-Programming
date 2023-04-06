#include <gtk/gtk.h>

int r;
int c;
GtkWidget *image;
GtkWidget *fixed;

gboolean my_keypress_function (GtkWidget *widget, GdkEventKey *event, gpointer data) {
    if (event->keyval == GDK_KEY_Right){
        g_print("RIGHT KEY PRESSED!\n");
	if(r < 799) {
		r += 100;
	}	
    }
    else if (event->keyval == GDK_KEY_Left){
        g_print("LEFT KEY PRESSED!\n");
	if(r > 0) {
		r -= 100;
	}
    }
    else if (event->keyval == GDK_KEY_Up){
        g_print("UP KEY PRESSED!\n");
	if(c > 0) {
		c -= 100;
	}
    }
    else if (event->keyval == GDK_KEY_Down){
        g_print("DOWN KEY PRESSED!\n");
	if(c < 799) {
		c += 100;
	}
    }

    gtk_fixed_move(GTK_FIXED(fixed), image, r, c);
    return TRUE;
}

int main(int argc, char *argv[]) {

  GtkWidget *window;

  gtk_init(&argc, &argv);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "GUI_TEST");
  gtk_window_set_default_size(GTK_WINDOW(window), 800, 800);
  gtk_container_set_border_width(GTK_CONTAINER(window), 0);
	
  fixed = gtk_fixed_new();
  gtk_container_add(GTK_CONTAINER(window), fixed);

  r = 0, c = 0;
  image = gtk_image_new_from_file("smile.png");
  gtk_fixed_put(GTK_FIXED(fixed), image, r, c);

  g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(my_keypress_function), NULL);
  gtk_widget_show_all(window); 

  gtk_main();

  return 0;
}
