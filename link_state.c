#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "link_state.h"

/*
 *  pthread
 */
//pthread_t thr;
//int pthread_create(&thr, NULL, link_state(), NULL); //3rd arg is function handler: (void *) handler_name(void *args)

/*
 *  mutex
 */
//pthread_mutex_t mylock;
//int pthread_mutex_init(&mylock, NULL);
//int pthread_mutex_lock(&mylock);
//int pthread_mutex_unlock(&mylock);

int main(int argc, char **argv) {
    // check command line arguments
    check_argc(argc, 5, argv);

    int router_id = atoi(argv[1]);
    int num_nodes = atoi(argv[2]);
    FILE *f_costs = fopen(argv[3], "r");
    FILE *f_hosts = fopen(argv[4], "r");

    int cost_table[num_nodes][num_nodes];
    Machine host_table[num_nodes];
    
    parse_costs(f_costs, num_nodes, cost_table);
    parse_hosts(f_hosts, num_nodes, host_table);
    
    srand(time(NULL));

    print_host_table(num_nodes, host_table);
    print_cost_table(num_nodes, cost_table);
    
    // configure address
    int sock, port, bytes;
    char buffer[BUFF_SIZE];
    struct sockaddr_in serv_addr;
    socklen_t addr_size;
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = ??? // what port number to use?
    inet_pton(AF_inet, ???, &serv_addr.sin_addr.s_addr); // what ip?
    memset(serv_addr.sin_zero, '\0', sizeof(serv_addr.sin_zero));
    addr_size = sizeof serv_addr;
    
    // create UDP socket
    sock = socket(PF_INET, SOCK_DGRAM, 0);
    
    for (int i = 0; i < CHANGES; i++) {
        char *id, *new_cost;
        // read change from keyboard
        if (fgets(buffer, BUFF_SIZE, stdin)) {
            if (sscanf(buffer, "%s%s", id, new_cost))
        }
        
        // update neighbor cost table
        
        // send msg to to other nodes using UDP
        sendto(sock, buffer, bytes, 0, (struct sockaddr *)&serv_addr, addr_size);
        
        // sleep for 10s
        usleep(SLEEP_LO);
    }

    return 0;
}

void receive_info() {
    // receive msg from other nodes
    bytes = recvfrom(sock, buffer, BUFF_SIZE, 0, NULL, NULL);
    
    // update neighbor cost table
}

void link_state() {
    
    // sleep for 10-20s
    int dur = rand() % (SLEEP_HI + 1 - SLEEP_LO) + SLEEP_LO;
    usleep(dur);
}


int check_argc(int argc, int n, char **argv) {
    if (argc != n) {
        fprintf(stderr, "Usage: %s <router id> <number of machines> <costs file> <hosts file>\n", argv[0]);
        exit(-1);
    }
    return n;
}

void parse_costs(FILE *fp, int n, int c_table[n][n]) {
    for (int i = 0; i < n; i++) {
        fscanf(fp, "%d%d%d%d", &c_table[i][0], &c_table[i][1], &c_table[i][2], &c_table[i][3]);
    }
}

void parse_hosts(FILE *fp, int n, Machine h_table[n]) {
    for (int i = 0; i < n; i++) {
        fscanf(fp, "%s%s%d", h_table[i].name, h_table[i].ip, &h_table[i].port);
    }
}

void print_machine(Machine m) {
    printf("NAME: %s\tIP: %s\tPORT: %d\n", m.name, m.ip, m.port);
}

void print_host_table(int n, Machine h_table[n]) {
    for (int i = 0; i < n; i++) {
        print_machine(h_table[i]);
    }
}

void print_cost_table(int n, int c_table[n][n]) {
    //pthread_mutex_lock(&mylock);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            printf("%d\t",c_table[i][j]);
        }
        printf("\n");
    }
    //pthread_mutex_unlock(&mylock);
}

/*
void sleep_timer(int duration) {
    printf("Sleeping for: \n");
    for (int i = duration; i > 0; i--) {
        printf("\r%d", i);
        //usleep(s_to_micros(3));
    }
    printf("\n");
}
*/
