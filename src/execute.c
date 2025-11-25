/*--------------------------------------------------------------------*/
/* execute.c */
/* Author: Jongki Park, Kyoungsoo Park */
/*--------------------------------------------------------------------*/

#include "dynarray.h"
#include "token.h"
#include "util.h"
#include "lexsyn.h"
#include "snush.h"
#include "execute.h"
#include "job.h"

extern struct job_manager *manager;
extern volatile sig_atomic_t sigchld_flag;
extern volatile sig_atomic_t sigint_flag;

/*--------------------------------------------------------------------*/
void block_signal(int sig, int block) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, sig);

    if (sigprocmask(block ? SIG_BLOCK : SIG_UNBLOCK, &set, NULL) < 0) {
        fprintf(stderr, 
            "[Error] block_signal: sigprocmask(%s, sig=%d) failed: %s\n",
            block ? "SIG_BLOCK" : "SIG_UNBLOCK", sig, strerror(errno));
        exit(EXIT_FAILURE);
    }
}
/*--------------------------------------------------------------------*/
void handle_sigchld(void) {

    /*
     * TODO: Implement handle_sigchld() in execute.c
     * Call waitpid() to wait for the child process to terminate.
     * If the child process terminates, handle the job accordingly.
     * Be careful to handle the SIGCHLD signal flag and unblock SIGCHLD.
    */
    pid_t pid;
    int status;
    int i;
    int j;
    if (sigchld_flag) {
        block_signal(SIGCHLD, 1);
        sigchld_flag = 0;
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            int found = 0;
            for(i = 0; i < MAX_JOBS; i++) {
                struct job *lookjob = &manager->jobs[i];
                if (!lookjob->used) {
                    continue;
                }
                for (j = 0; j < lookjob->remaining_processes; j++) {
                    if (lookjob->pids[j] == pid) {
                        found = 1;
                        remove_pid_from_job(lookjob, pid);
                        if (lookjob->remaining_processes == 0) {
                            fprintf(stdout,
                                "[%d] Process group: %d done\n", lookjob->job_id, lookjob->pgid);
                            delete_job(lookjob->job_id);
                        } 
                        break;
                    }
                } 
                if(found) {break;}
            }
        }
        block_signal(SIGCHLD, 0);
    }

}
/*--------------------------------------------------------------------*/
void handle_sigint(void) {
    
    /*
     * TODO: Implement handle_sigint() in execute.c
     * Find the foreground job and send signal to every process in the
     * process group.
     * Be careful to handle the SIGINT signal flag and unblock SIGINT.
     */
    int i;
    if (sigint_flag) {
        block_signal(SIGINT, 1);
        sigint_flag = 0;
        for (i = 0; i < MAX_JOBS; i++) {
            struct job *findjob = &manager->jobs[i];
            if (findjob->jobstate == FOREGROUND && findjob->used == 1) {
                int fgpgid = findjob->pgid;
                if (fgpgid > 0) {
                    killpg(fgpgid, SIGINT);
                    break;
                }
            }
        }
        block_signal(SIGINT, 0);

    }
    
}
/*--------------------------------------------------------------------*/
void dup2_e(int oldfd, int newfd, const char *func, const int line) {
    int ret;

    ret = dup2(oldfd, newfd);
    if (ret < 0) {
        fprintf(stderr, 
            "Error dup2(%d, %d): %s(%s) at (%s:%d)\n", 
            oldfd, newfd, strerror(errno), errno_name(errno), func, line);
        exit(EXIT_FAILURE);
    }
}
/*--------------------------------------------------------------------*/
/* Do not modify this function. It is used to check the signals and 
 * handle them accordingly. It is called in the main loop of snush.c.
 */
void check_signals(void) {
    handle_sigchld();
    handle_sigint();
}
/*--------------------------------------------------------------------*/
void redout_handler(char *fname) {
    /*
     TODO: Implement redout_handler in execute.c
    */
   int fd;
   fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0600);
   if (fd < 0) {
    error_print(NULL, PERROR);
    exit(EXIT_FAILURE);
   }

   dup2_e(fd, STDOUT_FILENO, __func__, __LINE__);
   close(fd);
}
/*--------------------------------------------------------------------*/
void redin_handler(char *fname) {
    int fd;

    fd = open(fname, O_RDONLY);
    if (fd < 0) {
        error_print(NULL, PERROR);
        exit(EXIT_FAILURE);
    }

    dup2_e(fd, STDIN_FILENO, __func__, __LINE__);
    close(fd);
}
/*--------------------------------------------------------------------*/
void build_command_partial(DynArray_T oTokens, int start, 
                        int end, char *args[]) {
    int i, redin = FALSE, redout = FALSE, cnt = 0;
    struct Token *t;

    /* Build command */
    for (i = start; i < end; i++) {
        t = dynarray_get(oTokens, i);

        if (t->token_type == TOKEN_WORD) {
            if (redin == TRUE) {
                redin_handler(t->token_value);
                redin = FALSE;
            }
            else if (redout == TRUE) {
                redout_handler(t->token_value);
                redout = FALSE;
            }
            else {
                args[cnt++] = t->token_value;
            }
        }
        else if (t->token_type == TOKEN_REDIN)
            redin = TRUE;
        else if (t->token_type == TOKEN_REDOUT)
            redout = TRUE;
    }

    if (cnt >= MAX_ARGS_CNT) 
        fprintf(stderr, "[BUG] args overflow! cnt=%d\n", cnt);

    args[cnt] = NULL;

#ifdef DEBUG
    for (i = 0; i < cnt; i++) {
        if (args[i] == NULL)
            printf("CMD: NULL\n");
        else
            printf("CMD: %s\n", args[i]);
    }
    printf("END\n");
#endif
}
/*--------------------------------------------------------------------*/
void build_command(DynArray_T oTokens, char *args[]) {
    build_command_partial(oTokens, 0, 
                        dynarray_get_length(oTokens), 
                        args);
}
/*--------------------------------------------------------------------*/
int execute_builtin_partial(DynArray_T toks, int start, int end,
                            enum BuiltinType btype, int in_child) {
    
    int argc = end - start;
    struct Token *t1;
    int ret;
    char *dir;

    switch (btype) {
    case B_EXIT:
        if (in_child) return 0;
        
        if (argc == 1) {
            dynarray_map(toks, free_token, NULL);
            dynarray_free(toks);
            exit(EXIT_SUCCESS);
        }
        else {
            error_print("exit does not take any parameters", FPRINTF);
            return -1;
        }

    case B_CD: {
        if (argc == 1) {
            dir = getenv("HOME");
            if (!dir) {
                error_print("cd: HOME variable not set", FPRINTF);
                return -1;
            }
        } 
        else if (argc == 2) {
            t1 = dynarray_get(toks, start + 1);
            if (t1 && t1->token_type == TOKEN_WORD) 
                dir = t1->token_value;
        } 
        else {
            error_print("cd: Too many parameters", FPRINTF);
            return -1;
        }

        ret = chdir(dir);
        if (ret < 0) {
            error_print(NULL, PERROR);
            return -1;
        }
        return 0;
    }

    default:
        error_print("Bug found in execute_builtin_partial", FPRINTF);
        return -1;
    }
}
/*--------------------------------------------------------------------*/
int execute_builtin(DynArray_T oTokens, enum BuiltinType btype) {
    return execute_builtin_partial(oTokens, 0, 
                                dynarray_get_length(oTokens), btype, FALSE);
}
/*--------------------------------------------------------------------*/
/* 
 * You need to finish implementing job related APIs. (find_job_by_jid(),
 * remove_pid_from_job(), delete_job()) in job.c to handle the job.
 * Feel free to modify the format of the job API according to your design.
 */
void wait_fg(int job_id) {
    pid_t pid;
    int status;

     // Find the job structure by job ID
    struct job *job = find_job_by_jid(job_id);
    if (!job) {
        fprintf(stderr, "Job: %d not found\n", job_id);
        return;
    }

    while (1) {
        pid = waitpid(-job->pgid, &status, 0);

        if (pid > 0) {
            // Remove the finished process from the job's pid list
            if (!remove_pid_from_job(job, pid)) {
                fprintf(stderr, "Pid %d not found in the job: %d list\n", 
                    pid, job->job_id);
            }

            if (job->remaining_processes == 0) break;
        }

        if (pid == 0) continue;

        if (pid < 0) {
            if (errno == EINTR) continue;
            if (errno == ECHILD) break;
            error_print("Unknown error waitpid() in wait_fg()", PERROR);
        }
    }

    // Clean up job table entry if all processes are done
    if (job->remaining_processes == 0)
        delete_job(job->job_id);
}
/*--------------------------------------------------------------------*/
void print_job(int job_id, pid_t pgid) {
    fprintf(stdout, 
        "[%d] Process group: %d running in the background\n", job_id, pgid);
}
/*--------------------------------------------------------------------*/
int fork_exec(DynArray_T oTokens, int is_background) {
    /*
     * TODO: Implement fork_exec() in execute.c
     * To run a newly forked process in the foreground, call wait_fg() 
     * to wait for the process to finish.  
     * To run it in the background, call print_job() to print job id and
     * process group id.  
     * All terminated processes must be handled by sigchld_handler() in * snush.c. 
     */
    char *args[dynarray_get_length(oTokens)];
    struct job *job = add_job();
    pid_t shell_pid = getpid();
    pid_t pid = fork();

    if(pid == 0) {
        setpgid(0, 0);
        build_command(oTokens, args);
        execvp(args[0], args);
        _exit(1);
    }
    job->pgid = pid;
    add_pid_to_job(job, pid);
    setpgid(pid, pid);
    if(!is_background) {
        job ->jobstate = FOREGROUND;
        tcsetpgrp(STDIN_FILENO, pid);
        wait_fg(job->job_id);
        tcsetpgrp(STDIN_FILENO, shell_pid);
    } else {
        job->jobstate = BACKGROUND;
        print_job(job->job_id, pid);
    }
    return job->job_id;
}
/*--------------------------------------------------------------------*/
int iter_pipe_fork_exec(int n_pipe, DynArray_T oTokens, int is_background) {
    /*
     * TODO: Implement iter_pipe_fork_exec() in execute.c
     * To run a newly forked process in the foreground, call wait_fg() 
     * to wait for the process to finish.  
     * To run it in the background, call print_job() to print job id and
     * process group id.  
     * All terminated processes must be handled by sigchld_handler() in * snush.c. 
     */

    struct index {
        int start_index;
        int end_index;
    };

    struct job *job = add_job();
    pid_t shell_pid = getpid();
    pid_t childpid;

    //Decomposing the token array, and creating indexes to each command in the pipeline
    int i;
    int start = 0;
    int arrayindex = 0;
    struct index *indices = calloc(n_pipe+1, sizeof(struct index));

    for(i = 0; i < dynarray_get_length(oTokens); i++) {
        struct Token *t_curr = dynarray_get(oTokens, i);
        if(t_curr->token_type != TOKEN_PIPE) {
            continue;
        } else if (t_curr->token_type == TOKEN_PIPE) {
            indices[arrayindex].start_index = start;
            indices[arrayindex].end_index = i;
            start = i+1;
            arrayindex++;
        }
    }
    indices[arrayindex].start_index = start;
    indices[arrayindex].end_index = dynarray_get_length(oTokens);

    //Setting up pipes
    int pipefds[2 * n_pipe];
    for(i = 0; i < n_pipe; i++) {
        pipe(pipefds + i * 2);
    }

    //Creating child processes and make each execute 1 of the commands in the pipeline
    for(i = 0; i < n_pipe + 1; i++) {
        pid_t pid = fork();
        if(pid == 0) {
            //Set the FD's according to which command we are at
            if(i == 0) {
                dup2(pipefds[1], STDOUT_FILENO);
            } else if (i == n_pipe) {
                dup2(pipefds[(i - 1) * 2], STDIN_FILENO);
            } else {
                dup2(pipefds[(i - 1) * 2], STDIN_FILENO);
                dup2(pipefds[i * 2 + 1], STDOUT_FILENO);
            }
            for(int j = 0; j < 2 * n_pipe; j++){
                close(pipefds[j]);
            }
            //Check if the command is a builtin one, if so, execute it
            enum BuiltinType bt = check_builtin(dynarray_get(oTokens, indices[i].start_index));
            if (bt != NORMAL) {
                execute_builtin_partial(oTokens, indices[i].start_index,
                    indices[i].end_index, bt, FALSE);
                _exit(0);
            }
            //The command is external and therefor calls execvp
            char *args[dynarray_get_length(oTokens)];
            build_command_partial(oTokens, indices[i].start_index, 
                indices[i].end_index, args);
            execvp(args[0], args);
            perror("execvp failed");
            _exit(1);
        }
        if (i == 0) {
            childpid = pid;
            job->pgid = childpid;
        }
        add_pid_to_job(job, pid);
        setpgid(pid, childpid);
    }
    for(i = 0; i < 2 * n_pipe; i++){
                close(pipefds[i]);
    }
    if (!is_background) {
        job->jobstate = FOREGROUND;
        tcsetpgrp(STDIN_FILENO, job->pgid);
        wait_fg(job->job_id);
        tcsetpgrp(STDIN_FILENO, shell_pid);
    } else {
        job->jobstate = BACKGROUND;
        print_job(job->job_id, job->pgid);
    }

    return childpid;
}
/*--------------------------------------------------------------------*/
