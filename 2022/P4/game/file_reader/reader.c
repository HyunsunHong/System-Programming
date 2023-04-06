#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <fcntl.h>

struct user{
	GtkWidget * image;
	GtkWidget * fixed;
	int r;
	int c;
};

int map[9][9] = {0, };
int user_cnt = 0;
int user_register[4] = {0, };
pthread_mutex_t mutex;
GtkWidget * window;
GtkWidget * fixed;
struct user user_list[4];
char * png_list[4];

/*
gboolean my_keypress_function (GtkWidget *widget, GdkEventKey *event, gpointer data, struct * user) {
    
    GtkWidget * image = user->image;
    GtkWidget * fixed = user->fixed;
    int r = user->r;
    int c = user->c; 

    if (event->keyval == GDK_KEY_Right){
        //g_print("RIGHT KEY PRESSED!\n");
	if(r < 799) {
		r += 100;
	}	
    }
    else if (event->keyval == GDK_KEY_Left){
        //g_print("LEFT KEY PRESSED!\n");
	if(r > 0) {
		r -= 100;
	}
    }
    else if (event->keyval == GDK_KEY_Up){
        //g_print("UP KEY PRESSED!\n");
	if(c > 0) {
		c -= 100;
	}
    }
    else if (event->keyval == GDK_KEY_Down){
        //g_print("DOWN KEY PRESSED!\n");
	if(c < 799) {
		c += 100;
	}
    }

    gtk_fixed_move(GTK_FIXED(fixed), image, r, c);
    return TRUE;
}
*/
int command_line_interface(int argc, char *argv[]) {
	
	if(argc < 2) {
		printf("Command lien interface fail\n");
		exit(1);
	}
	int fd = open(argv[1], O_RDONLY);
	if(fd == -1) {
		perror(argv[1]);
		exit(1);
	}
	return fd;
}

int possible(int r, int c) {
	if(r < 0 || r > 8) return 0;
	if(c < 0 || c > 8) return 0;

	if(map[r][c] != -1) return 0;
	return 1;
}

void * thread_gtr_main(void * arg) {
	
	gtk_widget_show_all(window);
	gtk_main();
	pthread_exit(0);
}

int dr(unsigned char w) {
	if(w == 'U') return -50;
        if(w == 'D') return 50;
	if(w == 'L') return 0;
	return 0;	
}

int dc(unsigned char w) {
	if(w == 'U') return 0;
        if(w == 'D') return 0;
	if(w == 'L') return -50;
	return 50;	
}

void * thread_reading(void * arg) {

  int fd = *((int *)arg);
  unsigned char buf[4];
	
  for(int r = 0; r <= 8; r++) {
  	for(int c = 0; c <= 8; c++){
		map[r][c] = -1;
	}
  }  
	
  while((read(fd, buf, 1)) > 0) {		  
 	read(fd, buf + 1, 3);
	for(int i = 0; i < 4; i++) 
		printf("%c", buf[i]);

	buf[0] -= '0';
	buf[1] -= '0';

	if(buf[0] == 0) {
		int err = 0;
		if(user_cnt == 4) {
			printf("Full!\n");
			err = 1;
		}
	       	if(buf[1] > 3 || buf[1] < 0 || user_register[buf[1]]) {
			printf("Invalid user name!\n");
			err = 1;
		}	
		if(err) 
			continue;
		
		user_list[buf[1]].image = gtk_image_new_from_file(png_list[buf[1]]);	
		int rand_r;
		int rand_c;
		srand(time(NULL));
		do {
			rand_r = rand()%8 + 1;
			rand_c = rand()%8 + 1;
		} while(!possible(rand_r, rand_c));
		
		user_cnt++;
		user_register[buf[1]] = 1;
		user_list[buf[1]].r = rand_r*50; 
		user_list[buf[1]].c = rand_c*50;

		gtk_fixed_put(GTK_FIXED(user_list[buf[1]].fixed), user_list[buf[1]].image, user_list[buf[1]].r, user_list[buf[1]].c);	
	}
	else if(buf[0] == 1) {
		if(!user_register[buf[1]]) {
			printf("Invalid user name!\n");
			continue;
		}
		int next_r = user_list[buf[1]].r + dr(buf[2]);
	        int next_c = user_list[buf[1]].c + dc(buf[2]);	

		if(possible(next_r/50, next_c/50)) {
			map[user_list[buf[1]].r/50][user_list[buf[1]].c/50] = -1;
			map[next_r/50][next_c/50] = buf[1];
			user_list[buf[1]].r = next_r;
			user_list[buf[1]].c = next_c;
			gtk_fixed_move(GTK_FIXED(user_list[buf[1]].fixed), user_list[buf[1]].image, user_list[buf[1]].r, user_list[buf[1]].c);	
		}
	}
  	else if(buf[0] == 2) {
	 	user_register[buf[1]] = 0;
		user_cnt--;
		map[user_list[buf[1]].r/50][ user_list[buf[1]].c/50] = -1;
		gtk_widget_destroy(user_list[buf[1]].image);
	}
	gtk_widget_show_all(window);
	sleep(1);
   }
	
}


int main(int argc, char *argv[]) {
 
  int fd = command_line_interface(argc, argv);

  gtk_init(&argc, &argv);
  
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "GUI_TEST");
  gtk_window_set_default_size(GTK_WINDOW(window), 1000, 1000);
  gtk_container_set_border_width(GTK_CONTAINER(window), 0);
 
  fixed = gtk_fixed_new();
  gtk_container_add(GTK_CONTAINER(window), fixed);
  png_list[0] = "smile1.png";
  png_list[1] = "smile2.png";
  png_list[2] = "smile3.png";
  png_list[3] = "smile4.png";
  for(int i = 0; i < 4; i++) {
  	user_list[i].fixed = fixed;
  }

  pthread_t tid[2];
  
  pthread_create(&tid[0], NULL, thread_gtr_main, NULL);
  pthread_create(&tid[1], NULL, thread_reading, &fd);

  pthread_join(tid[0], NULL);
  pthread_join(tid[1], NULL);

  /*  
  r = 0, c = 0;
  image = gtk_image_new_from_file("smile.png");
  gtk_fixed_put(GTK_FIXED(fixed), image, r, c);
  gtk_widget_show_all(window); 
  
*/	


  return 0;
}
