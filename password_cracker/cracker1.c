/**
 * password_cracker
 * CS 241 - Fall 2021
 */
#include "cracker1.h"
#include <string.h>
#include "format.h"
#include <crypt.h>
#include "utils.h"
#include <stdio.h>
#include "includes/queue.h"


typedef struct para_t {
    unsigned code;
    unsigned success_time;
    unsigned failure_time;
    queue* queue_;
} para_t;

typedef struct _task_t_ {
    char *usr_;
    unsigned len_pwd_;
    unsigned len_to_solve_;
    char *abst_;
    char *fx_;
    unsigned len_fx_;

} _task_t_;

void delete__task_t_(_task_t_ *result) {
    free(result->abst_);
    free(result->fx_);
    free(result->usr_);    
    free(result);
}

char *cracker(_task_t_ *curr_task, int code, int thread_count, unsigned *times) {
    char *trys = malloc(sizeof(char) * (curr_task->len_pwd_ + 1));
    long begin;
    long num_trys;
    memset(trys, 0, curr_task->len_pwd_ + 1);
    memset(trys, 'a', curr_task->len_pwd_);
    memcpy(trys, curr_task->fx_, curr_task->len_fx_);
    getSubrange(curr_task->len_to_solve_, thread_count, code, &begin, &num_trys);
    for (unsigned j = 0; j < begin; j++) {
        incrementString(trys);
    }
    const char *post_encrypt;
    struct crypt_data dt;
    dt.initialized = 0;

    for (unsigned j = 0; j < num_trys; j++) {
        post_encrypt = crypt_r(trys, "xx", &dt);
        if (strcmp(post_encrypt, curr_task->abst_) != 0) {
            incrementString(trys);
        } else {
            char *result = malloc(sizeof(char) * (curr_task->len_pwd_ + 1));
            memset(result, 0, curr_task->len_pwd_+1);
            memcpy(result, trys, curr_task->len_pwd_);
            *times = j + 1; 
            free(trys);
            return result;
        }
    }

    *times = num_trys;
    free(trys);
    return NULL;
}

void *initializer(void *arg) {
    para_t *paras = (para_t*) arg;
    int code = paras->code;
    queue *q = paras->queue_;
    unsigned num = 0;
    while (1) {
        _task_t_* result = (_task_t_*)queue_pull(q);
        if (result == NULL) {
            break;
        }
        v1_print_thread_start(code, result->usr_); 
        double initial = getThreadCPUTime();
        char* pwd = cracker(result, 1, 1, &num);
        double terminal = getThreadCPUTime();
        if (pwd == NULL) {
            v1_print_thread_result(code, result->usr_, pwd, num, terminal - initial, 1);
            paras->failure_time = paras->failure_time + 1;
        } else {
            v1_print_thread_result(code, result->usr_, pwd, num, terminal - initial, 0);
            paras->success_time = paras->success_time + 1;
        }
        free(pwd);
        delete__task_t_(result);
    }
    return NULL;
}

int start(size_t thread_count) {
    queue* q = queue_create(thread_count);
    pthread_t* threads[thread_count];
    para_t* paras[thread_count];

    for (unsigned j = 0; j < thread_count; j++) {
        paras[j]  = malloc(sizeof(para_t));
        threads[j] = malloc(sizeof(pthread_t));
        
        memset(paras[j], 0, sizeof(para_t));

        paras[j]->queue_   = q;
        paras[j]->code = j + 1;
        
        pthread_create(threads[j], NULL, &initializer, (void*)paras[j]); 
    }
    char usr_[1024];
    char abst_[1024];
    char given[1024];
    char line[1024];

    while(fgets(line, 768, stdin) != NULL) {
        sscanf(line, "%s %s %s", usr_, abst_, given);
        _task_t_ *result = malloc(sizeof(_task_t_));
        memset(result, 0, sizeof(_task_t_)); 
        int lth = strlen(usr_);
        result->usr_ = malloc(sizeof(char) * (lth + 1));
        memcpy(result->usr_, usr_, lth + 1);
        lth = strlen(abst_);
        result->abst_ = malloc(sizeof(char) * (lth + 1));
        memcpy(result->abst_, abst_, lth + 1);

        result->len_fx_ = getPrefixLength(given);
        result->len_pwd_ = strlen(given);
        result->len_to_solve_ = result->len_pwd_ - result->len_fx_;
        result->fx_ = malloc(sizeof(char) * (result->len_fx_ + 1));
        memset(result->fx_, 0, result->len_fx_ + 1);
        memcpy(result->fx_, given, result->len_fx_);
        queue_push(q, (void*)result);
    }
    for (unsigned j = 0; j < thread_count; j++) {
        queue_push(q, NULL);
    }
    for (unsigned j = 0; j < thread_count; j++) {
        pthread_join(*(threads[j]), NULL);
    }
    int success_ = 0;
    int faliure_ = 0;
    for (unsigned j = 0; j < thread_count; j++) {
        faliure_ = faliure_ + paras[j]->failure_time;
        success_ = success_ + paras[j]->success_time;

    }
    v1_print_summary(success_, faliure_);
    for (unsigned j = 0; j < thread_count; j++) {
        free(paras[j]);
        free(threads[j]);
    }
    queue_destroy(q);
    return 0;
}


