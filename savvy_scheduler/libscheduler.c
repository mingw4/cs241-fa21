/**
 * savvy_scheduler
 * CS 241 - Fall 2021
 */
#include "libpriqueue/libpriqueue.h"
#include "libscheduler.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "print_functions.h"
#include "includes/callbacks.h"
#include "includes/vector.h"

/**
 * The struct to hold the information about a given job
 */
typedef struct _job_info {
    int id;

    /* TODO: Add any other information and bookkeeping you need into this
     * struct. */
    double t_left_;
    double t_ini_;
    double reach_;
    double initiation_;
    double place_;
    double dur_;
} job_info;

//helpers
double t_pend;
double t_rspd;
double period_;
comparer_t comparision_func;
size_t n;
priqueue_t pqueue;
scheme_t pqueue_scheme;

void scheduler_start_up(scheme_t s) {
    switch (s) {
    case FCFS:
        comparision_func = comparer_fcfs;
        break;
    case PRI:
        comparision_func = comparer_pri;
        break;
    case PPRI:
        comparision_func = comparer_ppri;
        break;
    case PSRTF:
        comparision_func = comparer_psrtf;
        break;
    case RR:
        comparision_func = comparer_rr;
        break;
    case SJF:
        comparision_func = comparer_sjf;
        break;
    default:
        printf("Did not recognize scheme\n");
        exit(1);
    }
    priqueue_init(&pqueue, comparision_func);
    pqueue_scheme = s;
    // Put any additional set up code you may need here
    t_rspd = 0;
    n = 0;
    t_pend = 0;
    period_ = 0;
}

static int break_tie(const void *a, const void *b) {
    return comparer_fcfs(a, b);
}

int comparer_fcfs(const void *a, const void *b) {
    // TODO: Implement me!
    job_info* j_i_a = ((job*) a)->metadata;
    job_info* j_i_b = ((job*) b)->metadata;
    if (j_i_b->reach_ < j_i_a->reach_) {
        return 1;
    }
    if (j_i_b->reach_ > j_i_a->reach_) {
        return -1;
    }
    return 0;
}

int comparer_ppri(const void *a, const void *b) {
    // Complete as is
    return comparer_pri(a, b);
}

int comparer_pri(const void *a, const void *b) {
    // TODO: Implement me!
    job_info* j_i_a = ((job*) a)->metadata;
    job_info* j_i_b = ((job*) b)->metadata;
    if (j_i_b->place_ < j_i_a->place_) {
        return 1;
    }
    if (j_i_b->place_ > j_i_a->place_) {
        return -1;
    }
    return break_tie(a, b);
}

int comparer_psrtf(const void *a, const void *b) {
    // TODO: Implement me!
    job_info* j_i_a = ((job*) a)->metadata;
    job_info* j_i_b = ((job*) b)->metadata;
    if (j_i_b->t_left_ < j_i_a->t_left_) {
        return 1;
    }
    if (j_i_b->t_left_ > j_i_a->t_left_) {
        return -1;
    }
    return break_tie(a, b);
}

int comparer_rr(const void *a, const void *b) {
    // TODO: Implement me!
    job_info* j_i_a = ((job*) a)->metadata;
    job_info* j_i_b = ((job*) b)->metadata;
    if (j_i_b->t_ini_ < j_i_a->t_ini_) {
        return 1;
    }
    if (j_i_b->t_ini_ > j_i_a->t_ini_) {
        return -1;
    }
    return break_tie(a, b);
    return 0;
}

int comparer_sjf(const void *a, const void *b) {
    // TODO: Implement me!
    job_info* j_i_a = ((job*) a)->metadata;
    job_info* j_i_b = ((job*) b)->metadata;
    if (j_i_b->dur_ < j_i_a->dur_) {
        return 1;
    }
    if (j_i_b->dur_ > j_i_a->dur_) {
        return -1;
    }
    return break_tie(a, b);
}

// Do not allocate stack space or initialize ctx. These will be overwritten by
// gtgo
void scheduler_new_job(job *newjob, int job_number, double time,
                       scheduler_info *sched_data) {
    // TODO: Implement me!
    job_info *j_i_ = calloc(1,sizeof(job_info));
    j_i_->id = job_number;
    j_i_->dur_ = sched_data->running_time;
    j_i_->t_ini_ = time;
    j_i_->place_ = sched_data->priority;
    j_i_->reach_ = time;
    j_i_->initiation_ = -1;
    j_i_->t_left_ = sched_data->running_time;
    newjob->metadata = j_i_;
    priqueue_offer(&pqueue, newjob);
    n++;
}

job *scheduler_quantum_expired(job *job_evicted, double time) {
    // TODO: Implement me!
    job* j_ = priqueue_peek(&pqueue);
    if (!j_ && !job_evicted) {
        return NULL;
    }
    job_info* j_i_ = j_->metadata;
    if (0 > j_i_->initiation_) {
        j_i_->initiation_ = time;
    }
    if (j_ && !job_evicted) {
        j_i_->t_ini_ = time;
    }
    if (RR == pqueue_scheme) {
        priqueue_offer(&pqueue, j_);
        priqueue_poll(&pqueue);
        return priqueue_peek(&pqueue);
    }
    if (PPRI == pqueue_scheme) {
        if (j_ == job_evicted) {
            return job_evicted;
        } else {
            return j_;
        }
    }
    if (PSRTF == pqueue_scheme) {
        if (j_ == job_evicted) {
            job_info* j_e_i_ = job_evicted->metadata;
            j_e_i_ ->t_left_ = j_e_i_->t_left_ + j_e_i_->t_ini_ - time;
            j_e_i_->t_ini_ = time;
            return job_evicted;
        } else {
            job_info* j_e_i_ = job_evicted->metadata;
            j_e_i_ ->t_left_ = j_e_i_->t_left_ + j_e_i_->t_ini_ - time;
            j_i_->t_ini_ = time;
            return j_;
        }
    }
    return job_evicted;
}

void scheduler_job_finished(job *job_done, double time) {
    // TODO: Implement me!
    job_info* j_i_ = job_done->metadata;
    t_pend = t_pend - j_i_->dur_ - j_i_->reach_ + time;
    t_rspd = t_rspd - j_i_->initiation_ - j_i_->reach_ + time;
    period_ = period_ + time - j_i_->reach_;
    free(j_i_);
    priqueue_poll(&pqueue);
}

static void print_stats() {
    fprintf(stderr, "turnaround     %f\n", scheduler_average_turnaround_time());
    fprintf(stderr, "total_waiting  %f\n", scheduler_average_waiting_time());
    fprintf(stderr, "total_response %f\n", scheduler_average_response_time());
}

double scheduler_average_waiting_time() {
    // TODO: Implement me!
    return t_pend/n;
}

double scheduler_average_turnaround_time() {
    // TODO: Implement me!
    return period_/n;
}

double scheduler_average_response_time() {
    // TODO: Implement me!
    return t_rspd/n;
}

void scheduler_show_queue() {
    // OPTIONAL: Implement this if you need it!
}

void scheduler_clean_up() {
    priqueue_destroy(&pqueue);
    print_stats();
}
