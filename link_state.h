#ifndef LINK_STATE_H
#define LINK_STATE_H

#define SLEEP_LO s_to_ms(10)
#define SLEEP_HI s_to_ms(20)
#define CHANGES 10
#define BUFF_SIZE 1024

typedef struct machine {
	char name[50];
	char ip[50];
	int port;
} Machine;

int check_argc(int argc, int n, char **argv);
int s_to_ms(int s) { return s * 1000000; }
void parse_hosts(FILE *fp, int n, Machine h_table[n]);
void parse_costs(FILE *fp, int n, int c_table[n][n]);
void print_machine(Machine m);
void print_host_table(int n, Machine h_table[n]);
void print_cost_table(int n, int c_table[n][n]);
void sleep_timer(int duration);


#endif /* LINK_STATE_H */
