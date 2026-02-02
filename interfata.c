#include <gtk/gtk.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include<stdio.h>
#include<string.h>
#include "client.h"

GtkWidget *tv, *ip, *pt, *cmd;
struct MesajData {
    char text[1200];
};
void mesaj(const char *m) 
{
    GtkTextBuffer *b = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    GtkTextIter i;
    gtk_text_buffer_get_end_iter(b, &i);
    char *tag_name = "negru"; 
    if (strstr(m, "!!!") != NULL) 
    {
        tag_name = "rosu"; 
    } 
    else if (strncmp(m, "Server:", 7) == 0) 
    {
        tag_name = "verde";  
    }
    gtk_text_buffer_insert_with_tags_by_name(b, &i, m, -1, tag_name, NULL);
    gtk_text_buffer_insert(b, &i, "\n\n", 2);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(tv), gtk_text_buffer_get_insert(b), 0.0, FALSE, 0.0, 1.0);
}
gboolean actualizeaza_interfata(gpointer data) {
    struct MesajData *m = (struct MesajData*)data;
    mesaj(m->text); 
    free(m); 
    return FALSE;
}
void *thread_ascultare(void *arg) {
    char buf[1024];
    while(1) {
        if(socket_sd() >= 0)
        {
            int n = read(socket_sd(), buf, 1024);
            if(n > 0) 
            {
                buf[n] = 0; 
                struct MesajData *m = malloc(sizeof(struct MesajData));
                if(!m)continue;
                if(strstr(buf, "ALERTA")) 
                {
                    snprintf(m->text, 1200, "!!! %s", buf); 
                }
                else 
                {
                    snprintf(m->text, 1200, "Server: %s", buf);
                }
                g_idle_add(actualizeaza_interfata, m);
            }
            else if (n == 0) 
            {
                 break;
            }
        }
        usleep(100000); 
    }
    return NULL;
}
void trimite(GtkWidget *w, gpointer d) 
{
    char buf[1024];
    const char *txt=gtk_entry_get_text(GTK_ENTRY(cmd));
    if(!strlen(txt)) return;
    if(socket_sd() < 0) 
    {
        mesaj("Eroare: Nu esti conectat la server!");
        return;
    }
    if(scrie_comanda(txt) != 0) 
    {
        mesaj("Eroare la trimiterea mesajului.");
        return;
    }
    snprintf(buf, 1024, "Tu: %s", txt);
    mesaj(buf);
    if(!strcmp(txt, "Logout")) 
    { 
	    inchidere();
	    mesaj("Deconectat."); 
    }
    gtk_entry_set_text(GTK_ENTRY(cmd), "");
}


void buton_conectare(GtkWidget *w, gpointer d) 
{
    if(socket_sd() >= 0)
    {
	    mesaj("Esti deja conectat");
	    return;
    }
    const char *ip_text=gtk_entry_get_text(GTK_ENTRY(ip));
    const char *port_text=gtk_entry_get_text(GTK_ENTRY(pt));
    if(strlen(ip_text) == 0 || strlen(port_text) == 0)
    { 
        mesaj("Eroare: IP sau Port lipsă!");
        return;
    }
    int port_val = atoi(port_text);
    if(port_val <= 0 || port_val > 65535) 
    {
        mesaj("Eroare: Port invalid (trebuie să fie intre 1-65535)!");
        return;
    }
    if(conectare(ip_text,port_val)==0)
    {
        mesaj("Conectat!");
         pthread_t th;
        pthread_create(&th, NULL, thread_ascultare, NULL);
        pthread_detach(th);
    }
    else mesaj("Eroare conexiune.");
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(win), box);

    GtkWidget *hb1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(box), hb1, 0, 0, 0);
    
    gtk_box_pack_start(GTK_BOX(hb1), gtk_label_new("IP:"), 0, 0, 0);
    gtk_box_pack_start(GTK_BOX(hb1), ip = gtk_entry_new(), 1, 1, 0);
    gtk_entry_set_text(GTK_ENTRY(ip), "127.0.0.1");
    
    gtk_box_pack_start(GTK_BOX(hb1), gtk_label_new("Port:"), 0, 0, 0);
    gtk_box_pack_start(GTK_BOX(hb1), pt = gtk_entry_new(), 0, 0, 0);
    gtk_entry_set_text(GTK_ENTRY(pt), "2908");
    
    GtkWidget *btn = gtk_button_new_with_label("Conn");
    g_signal_connect(btn, "clicked", G_CALLBACK(buton_conectare), NULL);
    gtk_box_pack_start(GTK_BOX(hb1), btn, 0, 0, 0);

    GtkWidget *scr = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scr, 400, 300);
    gtk_box_pack_start(GTK_BOX(box), scr, 1, 1, 0);
    gtk_container_add(GTK_CONTAINER(scr), tv = gtk_text_view_new());
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tv), 0);

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    gtk_text_buffer_create_tag(buffer, "rosu", "foreground", "red", "weight", PANGO_WEIGHT_BOLD, NULL);
    gtk_text_buffer_create_tag(buffer, "verde", "foreground", "green", NULL);
    gtk_text_buffer_create_tag(buffer, "negru", "foreground", "black", NULL);

    GtkWidget *hb2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(box), hb2, 0, 0, 0);
    
    gtk_box_pack_start(GTK_BOX(hb2), cmd = gtk_entry_new(), 1, 1, 0);
    g_signal_connect(cmd, "activate", G_CALLBACK(trimite), NULL);
    
    btn = gtk_button_new_with_label("Send");
    g_signal_connect(btn, "clicked", G_CALLBACK(trimite), NULL);
    gtk_box_pack_start(GTK_BOX(hb2), btn, 0, 0, 0);

    gtk_widget_show_all(win);

    mesaj("BUN VENIT! Comenzi disponibile:");
    mesaj("1. Autentificare:\n   Login <Rol> <Nume> <ID> <Parola>\n   Ex: Login Doctor Popescu 1 pass123");
    mesaj("2. Trimite date (Pacient):\n   Update <Parametru> <Valoare>\n   Ex: Update Puls 120\n   (Parametri: Puls, Tensiune, Temperatura, Saturatie)");
    mesaj("3. Cauta in istoric (Doctor):\n   Istoric [Comanda specifica]<Nume> <ID> [Data]\n   Ex: Istoric Ionescu 100");
    mesaj("4. Deconectare:\n   Logout");

    gtk_main();
    return 0;
}
