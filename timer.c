
 
/* call back function - inform the user the time has expired */
void timeout_cb()
{
    printf("=== your time is up ===\n");
}
 
/* Go to sleep for a period of seconds */
static void* g_start_timer(void *args)
{
    /* function pointer */
    void (*function_pointer)();
 
    /* cast the seconds passed in to int and 
     * set this for the period to wait */
    int seconds = *((int*) args);
    printf("go to sleep for %d\n", seconds);
     
    /* assign the address of the cb to the function pointer */
    function_pointer = timeout_cb;
     
    sleep(seconds);
    /* call the cb to inform the user time is out */
    (*function_pointer)();
     
    pthread_exit(NULL);
}
 
int timer()
{
    pthread_t thread_id;
    int seconds;
    printf("Enter time in seconds\n");
    scanf("%d",&seconds);
    int rc;
 
    rc =  pthread_create(&thread_id, NULL, g_start_timer, (void *) &seconds);
    if(rc)
    printf("=== Failed to create thread\n");
 
    pthread_join(thread_id, NULL);
 
    printf("=== Timer expired ===\n");
 
    return 0;
}
