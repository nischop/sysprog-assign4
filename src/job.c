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
        manager -> n_jobs--;
        return 0;
    }
    return 1;
}
/*--------------------------------------------------------------------*/
/*
 * TODO: Implement any necessary job-control code in job.c 
 */
