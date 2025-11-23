/*--------------------------------------------------------------------*/
/* job.c */
/* Author: Jongki Park, Kyoungsoo Park */
/*--------------------------------------------------------------------*/

#include "job.h"

extern struct job_manager *manager;
/*--------------------------------------------------------------------*/
void init_job_manager() {
    manager = (struct job_manager *)calloc(1, sizeof(struct job_manager));
    if (manager == NULL) {
        fprintf(stderr, "[Error] job manager allocation failed\n");
        exit(EXIT_FAILURE);
    }

    /*
     * TODO: Init job manager
     */
    manager -> jobs = calloc(MAX_JOBS, sizeof(struct job));
    if (manager -> jobs == NULL) {
        fprintf(stderr, "[Error] jobs allocation failed \n");
        exit(EXIT_FAILURE);
    }
    manager -> n_jobs = 0;
    //Initialize each job id to index of array
    int i;
    for (i = 0; i < MAX_JOBS; i++) {
        manager -> jobs[i].job_id = i;
    }
}
/*--------------------------------------------------------------------*/
/* This is just a placeholder for compilation. You can modify it if you want. */
struct job *find_job_by_jid(int job_id) {
    /*
     * TODO: Implement find_job_by_jid()
     */
    int i;
    for(i = 0; i < MAX_JOBS; i++) {
        if(manager -> jobs[i].job_id == job_id && manager -> jobs[i].used == 1) {
            return &manager -> jobs[i];
        }
    }
    return NULL;
}
/*--------------------------------------------------------------------*/
int remove_pid_from_job(struct job *job, pid_t pid) {

    /*
     * TODO: Implement remove_pid_from_job()
    */
    int i;
    for(i = 0; i < MAX_PIDS; i++) {
        if (job->pids[i] == pid) {
            job->pids[i] = 0;
            if (job->remaining_processes > 0) {
                job->remaining_processes--;
            }
            return 1;
        }
    }
    return 0;
}
/*--------------------------------------------------------------------*/
int delete_job(int jobid) {
    
    /*
     * TODO: Implement delete_job()
     */
    struct job *djob;
    if ((djob = find_job_by_jid(jobid)) != NULL) {
        djob -> pgid = 0;
        djob -> remaining_processes = 0;
        djob -> used = 0;
        djob -> jobstate = UNKNOWN;
        memset(djob->pids, 0, sizeof(djob->pids));
        manager -> n_jobs--;
        return 0;
    }
    return 1;
}
/*--------------------------------------------------------------------*/
/*
 * TODO: Implement any necessary job-control code in job.c 
 */

struct job *add_job() {
    int i;
    for(i = 0; i < MAX_JOBS; i++) {
        if(!manager->jobs[i].used) {
            struct job *newjob = &manager->jobs[i];
            newjob -> pgid = 0;
            newjob -> remaining_processes = 0;
            newjob -> used = 1;
            newjob -> jobstate = UNKNOWN;
            manager->n_jobs++;
            return newjob;
        }
    }
    return NULL;
 }

int add_pid_to_job(struct job *job, pid_t pid) {
    int i;
    for(i = 0; i < MAX_PIDS; i++) {
        if(job->pids[i] == 0) {
            job->pids[i] = pid;
            job->remaining_processes++;
            return 0;
        }
    }
    return 1;
}
