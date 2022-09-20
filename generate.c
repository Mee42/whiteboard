#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdbool.h>


#define true 1
#define false 0

typedef struct {
    int x, y;
} point_t;

void tcp(void);
char* read_index_file(void);

int main(void) {

    tcp();
    return 0;


    #define points_size 10

    point_t points[points_size] = {
{ 0, 0},
{ 21, -27},
{ 21, -27},
{ 22, 30},
{ 22, 30},
{ -28, 29},
{ -28, 29},
{ -27, -23},
{ -27, -23},
{ 21, -27},
    };


    // linearly interpolate between them
    const double point_count = 100; // number of points between each defined point
    point_t* buffer = malloc(sizeof(point_t) * point_count * points_size); // most likely bigger buffer than we need but whatever
    point_t* buffer_head = buffer; // this is the spot to write to, it always points to the next open slot

    for(int point_index = 0;  point_index < points_size - 1; point_index++) {
        point_t one = points[point_index];
        point_t two = points[point_index + 1];
        // 100 intersperced points
        for(int progression = 0; progression < point_count; progression++) {
            double x = (int) (one.x + (two.x - one.x) * (progression/point_count));
            double y = (int) (one.y + (two.y - one.y) * (progression/point_count));
            if(buffer_head != buffer && buffer_head[-1].x == x && buffer_head[-1].y == y) {
                // skip
            } else {
                // add it
                buffer_head->x = x; // HERE
                buffer_head->y = y;
                buffer_head++;
            }
        }
    }

    // lets write to the serial port directly!
    FILE* serialPort = fopen("/dev/ttyACM0", "a+");

    for(point_t* buf_iter = buffer; buf_iter < buffer_head; buf_iter++) {
        fprintf(serialPort, "r %i, %i\n", buf_iter->x, buf_iter->y);
        printf("Sending (%i, %i)\n", buf_iter->x, buf_iter->y);
        if(buf_iter < buffer + 10) {
            sleep(1);
        }
        usleep(1000 * 100);
    }
    printf("size: %i\n", buffer_head - buffer);
    free(buffer);
    buffer = buffer_head = 0;
}





void tcp(void) {
    int socket_desc, client_sock, client_size;
    struct sockaddr_in server_addr, client_addr;
    char server_message[2000], client_message[2000];
    
    // Clean buffers:
    memset(server_message, '\0', sizeof(server_message));
    memset(client_message, '\0', sizeof(client_message));
    
    // Create socket:
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    
    if(socket_desc < 0){
        printf("Error while creating socket\n");
        return;
    }
    printf("Socket created successfully\n");
    
    // Set port and IP:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2005);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // Bind to the set port and IP:
    if(bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr))<0){
        printf("Couldn't bind to the port\n");
        return;
    }
    printf("Done with binding\n");
    
    // Listen for clients:
    if(listen(socket_desc, 1) < 0){
        printf("Error while listening\n");
        return;
    }
    printf("\nListening for incoming connections.....\n");
    while(true) {
        // Accept an incoming connection:
        client_size = sizeof(client_addr);
        client_sock = accept(socket_desc, (struct sockaddr*)&client_addr, &client_size);
        
        if (client_sock < 0){
            printf("Can't accept\n");
            return;
        }
        printf("Client connected at IP: %s and port: %i\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        // Receive client's message:
        if (recv(client_sock, client_message, sizeof(client_message), 0) < 0){
            printf("Couldn't receive\n");
            return;
        }
        printf("Msg from client: %s\n", client_message);
        
        // Respond to client:
        char* file_buff = read_index_file();
        
        if (send(client_sock, file_buff, strlen(file_buff), 0) < 0){
            printf("Can't send\n");
            return;
        }
        close(client_sock);
    }
    close(socket_desc);
}

char* read_index_file(void) {
    char* buffer = NULL;
    size_t length;
    FILE * f = fopen ("./website/index.html", "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        length = ftell (f);
        fseek(f, 0, SEEK_SET);
        buffer = malloc(length);
        if (buffer) {
            fread(buffer, 1, length, f);
        }
        fclose (f);
    }
    return buffer;
}