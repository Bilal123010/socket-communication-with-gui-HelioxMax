#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#define LENGTH_NAME 31
#define LENGTH_MSG 101
#define LENGTH_SEND 201

void str_trim_lf (char* arr, int length) {
    int i;
    for (i = 0; i < length; i++) { // trim \n
        if (arr[i] == '\n') {
            arr[i] = '\0';
            break;
        }
    }
}

void str_overwrite_stdout() {
    printf("\r%s", "> ");
    fflush(stdout);
}

// Global variables
volatile sig_atomic_t flag = 0;
int sockfd = 0;
char nickname[LENGTH_NAME] = {};

GtkWidget *message_entry;
GtkWidget *message_view;

void catch_ctrl_c_and_exit(int sig) {
    flag = 1;
}

void recv_msg_handler() {
    char receiveMessage[LENGTH_SEND] = {};
    while (1) {
        int receive = recv(sockfd, receiveMessage, LENGTH_SEND, 0);
        if (receive > 0) {
            GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(message_view));
            GtkTextIter end;
            gtk_text_buffer_get_end_iter(buffer, &end);
            gtk_text_buffer_insert(buffer, &end, receiveMessage, -1);
            gtk_text_buffer_insert(buffer, &end, "\n", -1);
        } else if (receive == 0) {
            break;
        } else {
            // -1
        }
    }
}

void send_msg_handler() {
    char message[LENGTH_MSG] = {};
    while (1) {
        str_overwrite_stdout();
        while (fgets(message, LENGTH_MSG, stdin) != NULL) {
            str_trim_lf(message, LENGTH_MSG);
            if (strlen(message) == 0) {
                str_overwrite_stdout();
            } else {
                break;
            }
        }
        send(sockfd, message, LENGTH_MSG, 0);
        if (strcmp(message, "exit") == 0) {
            break;
        }
    }
    catch_ctrl_c_and_exit(2);
}

void send_button_clicked(GtkWidget *widget, gpointer data) {
    const gchar *message = gtk_entry_get_text(GTK_ENTRY(message_entry));
    send(sockfd, message, LENGTH_MSG, 0);
    if (strcmp(message, "exit") == 0) {
        catch_ctrl_c_and_exit(2);
    }
    gtk_entry_set_text(GTK_ENTRY(message_entry), "");
}

void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *send_button;
    GtkWidget *scroll_view;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Chat Application");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_widget_set_size_request(window, 400, 300);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    send_button = gtk_button_new_with_label("Send");
    g_signal_connect(send_button, "clicked", G_CALLBACK(send_button_clicked), NULL);

    message_entry = gtk_entry_new();
    gtk_widget_set_hexpand(message_entry, TRUE);
    gtk_entry_set_activates_default(GTK_ENTRY(message_entry), TRUE);

    message_view = gtk_text_view_new();
    gtk_widget_set_vexpand(message_view, TRUE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(message_view), FALSE);

    scroll_view = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scroll_view, TRUE);
    gtk_container_add(GTK_CONTAINER(scroll_view), message_view);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), scroll_view, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), message_entry, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), send_button, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
signal(SIGINT, catch_ctrl_c_and_exit);
// Naming
printf("Please enter your name: ");
if (fgets(nickname, LENGTH_NAME, stdin) != NULL) {
    str_trim_lf(nickname, LENGTH_NAME);
}
if (strlen(nickname) < 2 || strlen(nickname) >= LENGTH_NAME - 1) {
    printf("\nName must be more than one and less than thirty characters.\n");
    exit(EXIT_FAILURE);
}

// Create socket
sockfd = socket(AF_INET, SOCK_STREAM, 0);
if (sockfd == -1) {
    printf("Fail to create a socket.");
    exit(EXIT_FAILURE);
}

// Socket information
struct sockaddr_in server_info, client_info;
int s_addrlen = sizeof(server_info);
int c_addrlen = sizeof(client_info);
memset(&server_info, 0, s_addrlen);
memset(&client_info, 0, c_addrlen);
server_info.sin_family = PF_INET;
server_info.sin_addr.s_addr = inet_addr("127.0.0.1");
server_info.sin_port = htons(8888);

// Connect to Server
int err = connect(sockfd, (struct sockaddr *)&server_info, s_addrlen);
if (err == -1) {
    printf("Connection to Server error!\n");
    exit(EXIT_FAILURE);
}

// Names
getsockname(sockfd, (struct sockaddr *)&client_info, (socklen_t *)&c_addrlen);
getpeername(sockfd, (struct sockaddr *)&server_info, (socklen_t *)&s_addrlen);
printf("Connect to Server: %s:%d\n", inet_ntoa(server_info.sin_addr), ntohs(server_info.sin_port));
printf("You are: %s:%d\n", inet_ntoa(client_info.sin_addr), ntohs(client_info.sin_port));

send(sockfd, nickname, LENGTH_NAME, 0);

pthread_t send_msg_thread;
if (pthread_create(&send_msg_thread, NULL, (void *)send_msg_handler, NULL) != 0) {
    printf("Create pthread error!\n");
    exit(EXIT_FAILURE);
}

pthread_t recv_msg_thread;
if (pthread_create(&recv_msg_thread, NULL, (void *)recv_msg_handler, NULL) != 0) {
    printf("Create pthread error!\n");
    exit(EXIT_FAILURE);
}

GtkApplication *app;
int status;
app = gtk_application_new(NULL, G_APPLICATION_FLAGS_NONE);
g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
status = g_application_run(G_APPLICATION(app), argc, argv);
g_object_unref(app);

while (1) {
    if (flag) {
        printf("\nBye\n");
        break;
    }
}

close(sockfd);
return status;
}

