#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

#define PERIOD 0.5

static void sig_usr(int sig); // 추가
static void sig_thread(int sig);
static void sig_thread_end(int sig);
static void * sleepy_wait(void * arg);
static void * period_send(void * arg);

int sigxcpu_counter = 0;
int sigusr1_counter = 0; // 추가
int sigxcpu_counter_last_sent = 0;
int sigusr1_counter_last_sent = 0;
int sigxcpu_counter_difference = 0;
int sigusr1_counter_difference = 0; // 추가
int xcpu_zero_sent = 0;
int usr1_zero_sent = 0; // 추가

int sleepy_wait_continue = 1;

char *command_post = "curl [https://$](https://$/){KUBERNETES_SERVICE_HOST}:${KUBERNETES_SERVICE_PORT}/apis/mcoperator.sdv.com/v1/namespaces/default/mckubes/${HOSTNAME}.${NODENAME}.rtmonitorobj --cacert /var/run/secrets/kubernetes.io/serviceaccount/ca.crt --header \"Authorization: Bearer $(cat /var/run/secrets/kubernetes.io/serviceaccount/token)\" -X POST -H 'Content-Type: application/yaml' -d \"---\n"
"apiVersion: mcoperator.sdv.com/v1\n"
"kind: McKube\n"
"metadata:\n"
"  name: ${HOSTNAME}.${NODENAME}.rtmonitorobj\n"
"spec:\n"
"  node: ${NODENAME}\n"
"  podname: ${HOSTNAME}\n"
"  pressuredDeadlinesTotal: 0\n"
"  pressuredDeadlinesPeriod: 0\" >>/dev/null 2>>/dev/null && exit";

char *command_patch = "curl [https://$](https://$/){KUBERNETES_SERVICE_HOST}:${KUBERNETES_SERVICE_PORT}/apis/mcoperator.sdv.com/v1/namespaces/default/mckubes/${HOSTNAME}.${NODENAME}.rtmonitorobj --cacert /var/run/secrets/kubernetes.io/serviceaccount/ca.crt --header \"Authorization: Bearer $(cat /var/run/secrets/kubernetes.io/serviceaccount/token)\" -X PATCH -H 'Content-Type: application/merge-patch+json' -d \'{ \"spec\": { \"pressuredDeadlinesTotal\": %d, \"pressuredDeadlinesPeriod\": %d } }\' >>/dev/null 2>>/dev/null && exit";

void monitor() {
	signal(SIGXCPU, sig_thread);
	signal(SIGTERM, sig_thread_end);
	signal(SIGUSR1, sig_usr); // 추가
	pthread_t this_thread;
	pthread_create(&this_thread, NULL, sleepy_wait, NULL);

	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGXCPU);
	sigaddset(&set, SIGTERM);
	//sigaddset(&set, SIGUSR1);
	sigprocmask(SIG_BLOCK, &set, NULL);
	pthread_create(&this_thread, NULL, period_send, NULL);
}

static void sig_usr(int sig) { // 추가
	sigusr1_counter++;
}

static void sig_thread(int sig) {
	sigxcpu_counter++;
}

static void * sleepy_wait(void * arg) {
	setpriority(PRIO_PROCESS, 0, 19);
	while(sleepy_wait_continue) {
		sleep(PERIOD);
	}
}

static void sigcal(){ // 추가
	char command_insert_numbers[2048];
	sigxcpu_counter_difference = sigxcpu_counter-sigxcpu_counter_last_sent;
	sigusr1_counter_difference = sigusr1_counter-sigusr1_counter_last_sent;
	if (sigxcpu_counter_difference > 0) {
		xcpu_zero_sent = 0;
	}
	if (sigusr1_counter_difference > 0) {
		usr1_zero_sent = 0;
	}
	if(sigusr1_counter_difference >= 0 && !usr1_zero_sent){
		if (sigxcpu_counter_difference >= 0 && !xcpu_zero_sent) {
			sprintf(command_insert_numbers, command_patch, sigusr1_counter_difference, sigxcpu_counter_difference);
		}
		else{
			sprintf(command_insert_numbers, command_patch, sigusr1_counter_difference, 0);
		}
		system(command_insert_numbers);
		sigxcpu_counter_last_sent = sigxcpu_counter;
		sigusr1_counter_last_sent = sigusr1_counter;
		if (sigxcpu_counter_difference == 0) {
			xcpu_zero_sent = 1;
		}
		if (sigusr1_counter_difference == 0) {
			usr1_zero_sent = 1;
		}
	}
}
/*
static void sigusr1cal(){
	char command_insert_numbers[2048];
	sigusr1_counter_difference = sigusr1_counter-sigusr1_counter_last_sent;
	if (sigusr1_counter_difference > 0) {
		usr1_zero_sent = 0;
	}
	if (sigusr1_counter_difference >= 0 && !usr1_zero_sent) {
		sprintf(command_insert_numbers, command_patch, sigusr1_counter, sigusr1_counter_difference);
		system(command_insert_numbers);	
		sigusr1_counter_last_sent = sigusr1_counter;
		if (sigusr1_counter_difference == 0) {
			usr1_zero_sent = 1;
		}
	}
}
*/
static void * period_send(void * arg) {
	setpriority(PRIO_PROCESS, 0, 19);
	system(command_post);	
	while(sleepy_wait_continue) {
		sigcal();
		//sigusr1cal();
		printf("usr: %d, x: %d\n", sigusr1_counter, sigxcpu_counter);
		sleep(PERIOD);
	}
}

static void sig_thread_end(int sig) {
	char command_insert_numbers[2048];
	sprintf(command_insert_numbers, command_patch, sigxcpu_counter, 0);
	system(command_insert_numbers);
	sleepy_wait_continue = 0;
	exit(1);
}
