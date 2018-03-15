#ifndef LINK_STATE_H
#define LINK_STATE_H

#define SLEEP_LO 10
#define SLEEP_HI 20
#define CHANGES 2
#define INFINITY 2147483647
#define BUFF_SIZE 1024

typedef struct machine {
	char name[50];
	char ip[50];
	int port;
} Machine;

int check_argc(int argc, int n, char **argv);
int s_to_ms(int s) { return s * 1000000; }
int ms_to_s(int ms) { return ms / 1000000; }
void allocate_rows(int **c_table);
void free_rows(int **c_table);
void parse_hosts(FILE *fp, Machine *h_table);
void parse_costs(FILE *fp, int **c_table);
void print_machine(Machine m);
void print_host_table(Machine *h_table);
void print_cost_table(int **c_table);
void send_info();
void receive_info();
void *receive_updates();
void *link_state();
int min_distance(int *dist, int *visited);
void update_cost();
void sleep_timer(int duration);

#endif /* LINK_STATE_H */
