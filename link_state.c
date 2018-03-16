#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include "link_state.h"

//  ps ax | grep <executable>
//  kill -SIGKILL <PID>

pthread_mutex_t my_lock;
int sock, port, router_id, num_nodes, SENDIT = 0;
int input[3], output[3]; // host1, host2, weight
int **cost_table;
Machine *host_table;

int main(int argc, char **argv) {
    // check command line arguments
    check_argc(argc, 5, argv);
    
    // initialize RNG
    srand(time(NULL));
    
    router_id = atoi(argv[1]);
    num_nodes = atoi(argv[2]);
    FILE *f_costs = fopen(argv[3], "r");
    FILE *f_hosts = fopen(argv[4], "r");

    cost_table = (int**) malloc(sizeof(int*) * num_nodes);
    allocate_rows(cost_table);

    host_table = (Machine*) malloc(sizeof(Machine*) * num_nodes);
    
    parse_costs(f_costs, cost_table);
    parse_hosts(f_hosts, host_table);
    
    print_cost_table(cost_table);
    print_host_table(host_table);
    
    pthread_mutex_init(&my_lock, NULL);
    
    struct sockaddr_in my_addr;
    socklen_t addr_size;
    
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(host_table[router_id].port);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset((char*) my_addr.sin_zero, '\0', sizeof(my_addr.sin_zero));
    addr_size = sizeof my_addr;
    
    // create socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Could not create socket");
        return -1;
    }
    
    // bind socket
    if (bind(sock, (struct sockaddr *)&my_addr, sizeof my_addr) != 0) {
        perror("Could not bind socket");
        return -1;
    }
    
    // begin threads
    pthread_t recv_thr, link_thr;
    pthread_create(&recv_thr, NULL, receive_updates, NULL);
    pthread_create(&link_thr, NULL, link_state, NULL);

    int i;
    for (i = 0; i < CHANGES; i++) {
        // update neighbor cost table
        update_cost();
        
        // sleep for 10s
        printf("Sleeping for %ds...\n", SLEEP_LO);
        sleep(SLEEP_LO);
    }

    // wait 60s then exit
    printf("No more changes to be made to Router %d. Exiting in %ds.\n", router_id, WAIT);
    sleep(WAIT);

    free_rows(cost_table);
    free(cost_table);
    free(host_table);
    return 0;
}

int check_argc(int argc, int n, char **argv) {
    if (argc != n) {
        fprintf(stderr, "Usage: %s <router id> <number of machines> <costs file> <hosts file>\n", argv[0]);
        exit(-1);
    }
    return n;
}

void allocate_rows(int **c_table) {
    int i;
    for (i = 0; i < num_nodes; i++) {
        c_table[i] = (int*) malloc(sizeof(int) * num_nodes);
    }
}

void free_rows(int **c_table) {        
    int i;
    for (i = 0; i < num_nodes; i++) {
        free(c_table[i]);
    }
}

void parse_costs(FILE *fp, int **c_table) {
    int i, j;
    for (i = 0; i < num_nodes; i++) {
        for (j = 0; j < num_nodes; j++) {
            if (fscanf(fp, "%d", &c_table[i][j]) != 1) {
                break;
            }
        }
    }
}

void parse_hosts(FILE *fp, Machine *h_table) {
    int i;
    for (i = 0; i < num_nodes; i++) {
        fscanf(fp, "%s%s%d", h_table[i].name, h_table[i].ip, &h_table[i].port);
    }
}

void print_machine(Machine m) {
    printf("NAME: %s\tIP: %s\tPORT: %d\n", m.name, m.ip, m.port);
}

void print_host_table(Machine *h_table) {
    int i;
    for (i = 0; i < num_nodes; i++) {
        print_machine(h_table[i]);
    }
    printf("\n");
}

void print_cost_table(int **c_table) {
    int i, j;
    for (i = 0; i < num_nodes; i++) {
        for (j = 0; j < num_nodes; j++) {
            printf("%d\t",c_table[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

void send_info() {
    int sock, i;
    struct sockaddr_in des_addr[num_nodes];
    socklen_t addr_size[num_nodes];
    
    // configure address
    for (i = 0; i < num_nodes; i++) {
        des_addr[i].sin_family = AF_INET;
        des_addr[i].sin_port = htons(host_table[i].port);
        inet_pton(AF_INET, host_table[i].ip, &des_addr[i].sin_addr.s_addr);
        memset(des_addr[i].sin_zero, '\0', sizeof(des_addr[i].sin_zero));
        addr_size[i] = sizeof des_addr[i];
    }
    
    // create UDP socket
    sock = socket(PF_INET, SOCK_DGRAM, 0);
    
    for (i = 0; i < num_nodes; i++) {
        if (i != router_id) {
            sendto(sock, &output, sizeof output, 0, (struct sockaddr *)&des_addr[i], addr_size[i]);
        }
    }
}

void receive_info() {
    // receive msg from other nodes
    int bytes = recvfrom(sock, &input, sizeof input, 0, NULL, NULL);
    
    switch(bytes) {
	case 0:
	    printf("No messages available to be received.\n");
	    break;
	case -1:
	    perror("Could not receive message");
	    printf("\n");
	    break;
	default:
	    printf("Received info.\n");
        SENDIT = 1;
    }
}

void *receive_updates() {
    int host1, host2, weight;
    
    while (1) {
        receive_info();
        host1 = ntohl(input[0]);
        host2 = ntohl(input[1]);
        weight = ntohl(input[2]);
        
        pthread_mutex_lock(&my_lock);
        cost_table[host1][host2] = weight;
        cost_table[host2][host1] = weight;

        pthread_mutex_unlock(&my_lock);
    }
}

// implemented using Dijkstra
void *link_state() {
    while (1) {
        int i, j, src, count;
        int dist[num_nodes];
        int visited[num_nodes];
        int temp_costs[num_nodes][num_nodes];
        
        pthread_mutex_lock(&my_lock);

        for (src = 0; src < num_nodes; src++) {
            for (i = 0; i < num_nodes; i++) {
                dist[i] = INFINITY;
                visited[i] = 0;
            }
            dist[src] = 0;
            
            for (count = 0; count < num_nodes - 1; count++) {
                int u, v;
                
                u = min_distance(dist, visited);
                visited[u] = 1;
                
                for (v = 0; v < num_nodes; v++) {
		  //check not visited , not same ID, and cost_table[u][v] < dist[v]
                    if (visited[v] == 0 && cost_table[u][v] && dist[u] != INFINITY && dist[u] + cost_table[u][v] < dist[v]) {
                        dist[v] = dist[u] + cost_table[u][v];
                    }
                }
            }
            
            for (i = 0; i < num_nodes; i++) {
                temp_costs[src][i] = dist[i];
                temp_costs[i][src] = dist[i];
            }
        }
        
        if (SENDIT) {
            printf("Current least costs:\n");
            for (i = 0; i < num_nodes; i++) {
                for (j = 0; j < num_nodes; j++) {
                    printf("%d\t", temp_costs[i][j]);
                }
                printf("\n");
            }
            SENDIT = 0;
        }
            
        pthread_mutex_unlock(&my_lock);
        
        // sleep for 10-20s
        int dur = rand() % (SLEEP_HI + 1 - SLEEP_LO) + SLEEP_LO;
        printf("Sleeping for %ds...\n", dur);
        sleep(dur);
    }
}

int min_distance(int *dist, int *visited) {
    int min, min_index, i;
    min = INFINITY;
    
    for (i = 0; i < num_nodes; i++) {
        if (visited[i] == 0 && dist[i] < min) {
            min = dist[i];
            min_index = i;
        }
    }
    
    return min_index;
}

void update_cost() {
    int neighbor, new_cost;
    
    printf("Enter a neighbor between 0 and %d and new node, or type '-1 -1' to quit:\n", num_nodes - 1);
    scanf("%d %d", &neighbor, &new_cost);
        
    if (neighbor == -1 && new_cost == -1) {
        printf("Killing process %d...\n", getpid());
        kill(getpid(), SIGKILL);
    } else {
        SENDIT = 1;
    }
  
    pthread_mutex_lock(&my_lock);

    cost_table[router_id][neighbor] = new_cost;
    cost_table[neighbor][router_id] = new_cost;
    output[0] = htonl(router_id);
    output[1] = htonl(neighbor);
    output[2] = htonl(new_cost);

    // send msg to to other nodes using UDP
    send_info();
    
    pthread_mutex_unlock(&my_lock);
}
