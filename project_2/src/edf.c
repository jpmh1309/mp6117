#include<stddef.h>
#include<stdio.h>
#include<stdlib.h>
#include "rm.c"

int createEarliestDeadlineFirst(struct Task * tasks, size_t num_tasks, struct ScheduleTimeUnit * o_schedule){
    struct Task priorityTasks[num_tasks];
    size_t run_period;
    //Check that all tasks_id are unique.
    for(int i = 0; i < num_tasks -1; i++){
        for(int j = i+1; j < num_tasks; j++){
            if(tasks[i].task_id == tasks[j].task_id ){
                printf("Error: Input task ID for two tasks is the same\n");
                return -1;
            }
        }
    }

    //Check that for all tasks p >= c
    for(int i = 0; i < num_tasks; i++){
        if(tasks[i].p < tasks[i].c){
            printf("Error: Task period is lower than the computation time\n");
            return -1;
        }
        else if (tasks[i].task_id == 0){
            printf("Error: task_id 0 is reserved for no task running\n");
            return -1; 
        }
    }

    //Copy the tasks into another array and reset the progress and mark them as waiting (all are ready to run)
    for(int i = 0; i < num_tasks; i++){ 
        priorityTasks[i] = tasks[i];
        priorityTasks[i].progress = 0;
        priorityTasks[i].status = WAITING;
    }
    //Sort the input tasks based on their priority (shortest period first)
    qsort(priorityTasks,num_tasks, sizeof(struct Task), taskPriorityCompare);

    
    //TODO:Compute the tests from [1] and [2]

    //Calculate the lcm of the periods to know for how long the simulation should run.
    run_period = priorityTasks[0].p;
    for(int i =1; i < num_tasks; i++){
        run_period = lcm(run_period, priorityTasks[i].p);
    }

    printf("The run will be a max of: %d time units as it is the lcm. As long as it is schedulable\n", (int)run_period);

    //Create the schedule struct(run_period +1 is because the last period requires arrows)
    struct ScheduleTimeUnit schedule[run_period + 1];
    //Setup where the arrows will be set in the graph
    for(int i =0; i< run_period + 1; i++){
        schedule[i].num_task_arrow = 0;
        for(int j =0; j< num_tasks; j++){
            if(i % priorityTasks[j].p == 0) {
                schedule[i].arrow_task_ids[schedule[i].num_task_arrow] = priorityTasks[j].task_id;
                schedule[i].num_task_arrow++;
            }
        }
    }
    //Deadlines
    size_t num_deadlines = 0;
    for(int i = 0; i < num_tasks; i++){
        num_deadlines += run_period / priorityTasks[i].p;
    }
    printf("num_deadlines: %d\n", (int) num_deadlines);
    struct Deadlines deadlines[num_deadlines];
    num_deadlines = 0;
    for(int i =1; i< run_period + 1; i++){
        for(int j =0; j< num_tasks; j++){
            if(i % priorityTasks[j].p == 0) {
                deadlines[num_deadlines].deadline = i;
                deadlines[num_deadlines].task_id = priorityTasks[j].task_id;
                num_deadlines++;
            }
        }
    }
    printf("num_deadlines: %d\n", (int) num_deadlines);

    //Tasks are arranged in meaning that the shortest deadline is first. 
    //The running_task variable refers to the index of the array and not the task id.
    //Schedule the first task out of the loop
    //The first deadline also correlates with deadlines[0]
    unsigned int running_task = 0;
    //currentDeadline is implemented to avoid changing the context in case two tasks have the same deadline
    struct Deadlines currentDeadline = deadlines[0];
    priorityTasks[running_task].status = RUNNING;
    priorityTasks[running_task].progress += 1;
    schedule[0].task_id = priorityTasks[running_task].task_id;

    if(priorityTasks[running_task].c == priorityTasks[running_task].progress){
        priorityTasks[running_task].progress = 0;
        priorityTasks[running_task].status = COMPLETED;
        //Aparently just moveing the running_task to the max will make it go to the lower priority in the next loop
        running_task = num_tasks;
    }

    for(int i =1; i< run_period +1; i++){
        int deadlineMissed = 0;
         
        //Check if any deadline was missed.
        for(int j =0; j < num_deadlines ; j++){
            if(deadlines[j].deadline == i){
                for(int k = 0; k< num_tasks; k++){
                    if(priorityTasks[k].task_id == deadlines[j].task_id){
                        if(priorityTasks[k].status == WAITING 
                        || priorityTasks[k].status == RUNNING){
                            run_period = i-1;
                            deadlineMissed = 1;
                            printf("Deadline was missed in time: %d for task_id: %d \n", i, deadlines[j].task_id);
                        }
                    }
                    else{
                        priorityTasks[k].status = WAITING;
                    }
                }

            }
        }
        if(deadlineMissed) break;
        
        //Iterate throught the deadline to find the earliest deadline.
        //Deadlines are ordered on time. So first 
        for(int j =0; j < num_deadlines; j++){
            int tasks_to_run = 1;
            //Found the earliest deadline
            if(deadlines[j].deadline > i){
                //Check if the proposed deadline corresponds to a COMPLETED task.
                for(int k = 0; k< num_tasks; k++ ){
                    if(deadlines[j].task_id == priorityTasks[k].task_id){
                        if(priorityTasks[k].status == COMPLETED){
                            tasks_to_run = 0;
                            break;
                        }
                    }
                }
                //if all tasks are completed, it will advance the loop by one without executing tasks.
                if(!tasks_to_run) continue;
                
                //If the deadline is the same continue executing the same task.
                //If not change the task to execute.
                if(currentDeadline.deadline != deadlines[j].deadline)
                    currentDeadline = deadlines[j];
                
                //Store the running task and put it to run
                for(int k =0; k< num_tasks; k++){
                    if(currentDeadline.task_id == priorityTasks[k].task_id){
                        running_task = k;
                        priorityTasks[running_task].status = RUNNING;
                    }
                }
                break;
            }    
        }
        

        //Run the task, update the schedule and the running task.
        if(priorityTasks[running_task].status == RUNNING){
            schedule[i].task_id = priorityTasks[running_task].task_id;
            priorityTasks[running_task].progress++;

            //Check if the task finished running:
            if(priorityTasks[running_task].c == priorityTasks[running_task].progress){
                priorityTasks[running_task].progress = 0;
                priorityTasks[running_task].status = COMPLETED;
                //Aparently just moveing the running_task to the max will make it go to the lower priority in the next loop
                running_task = num_tasks;
            }
        }
        //If no task is running.
        else{
            schedule[i].task_id = 0;
        }
        printf("t: %d. task: %d\n", i, schedule[i].task_id);
    
    }
}

int main(){
    struct Task tasks[3];
    struct ScheduleTimeUnit schedule[255];
    int schedulePeriod = 0;
    //Scheludable scenario
    
    tasks[0].task_id = 1;
    tasks[0].c = 1;
    tasks[0].p = 5;

    tasks[1].task_id = 2;
    tasks[1].c = 2;
    tasks[1].p = 8;

    tasks[2].task_id = 3;
    tasks[2].c = 6;
    tasks[2].p = 19;
    

    //Unschedulable scenario.
    /*
    tasks[0].task_id = 1;
    tasks[0].c = 3;
    tasks[0].p = 6;

    tasks[1].task_id = 2;
    tasks[1].c = 4;
    tasks[1].p = 9;
    */
    schedulePeriod = createEarliestDeadlineFirst(tasks, 3, schedule);
    printf("Reading from the retrurned schedule\n");
    for(int i =0; i< schedulePeriod; i++){
        printf("t: %d. task: %d\n", i, schedule[i].task_id);
        if(schedule[i].num_task_arrow != 0){
            for(int j = 0; j< schedule[i].num_task_arrow; j++){
                printf("Arrow at t: %d for task_id:%d\n", i, schedule[i].arrow_task_ids[j]);
            }
        }
    }
    
    return 0;

}