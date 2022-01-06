#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "libenc.h"
#include <pthread.h>
#include <unistd.h>
#define MAGIC_CONST 0x5391

#define COLOR_RED "\x1b[31m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_BLUE "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN "\x1b[36m"
#define COLOR_RESET "\x1b[0m"
#define COLOR_WHITE "\x1b[0m"
#define THREAD_NUMBER 20
unsigned long threshold;
size_t correct_start_time = 0;
size_t correct_end_time = 0;
size_t correct_time = 0;

uint64_t timestamp();
uint64_t cache_hit();
uint64_t cache_miss();
void maccess(void *p);
void memory_flush(void *p);
void *attacker_thread_01(void *arg);
void *attacker_thread_02(void *arg);
uint64_t flush_reload_start();
uint64_t flush_reload_end();
uint64_t flush_reload();
uint64_t get_cpu_time_start();
uint64_t get_cpu_time_end();

//=================================================flush_load&reload =======================================================================

uint64_t flush_reload_start()
{

    size_t start, end, time = 0, delta = 0;  //initialize variables for f+r at cpu start
    int i = 0, j = 0;

    int *pointer01 = (int *)get_cpu_time_start + 27;  //found from core dump

    asm volatile("mfence"); //reload
    start = timestamp();
    asm volatile("mfence");

    maccess(pointer01);

    asm volatile("mfence");
    end = timestamp();
    asm volatile("mfence");

    memory_flush(pointer01); //flush
    delta = end - start;

    if (delta < threshold)
    {
        correct_start_time = start;
        printf("cpu_start delta : %ld ", delta);
    }

    return delta;
}

uint64_t flush_reload_end()
{

    size_t start, end, time = 0, delta = 0;    // //initialize variables for f+r at cpu end
    int i = 0, j = 0;

    int *pointer01 = (int *)get_cpu_time_end + 24;   // found from core dump

    asm volatile("mfence"); //reload
    start = timestamp();
    asm volatile("mfence");

    maccess(pointer01);

    asm volatile("mfence");
    end = timestamp();
    asm volatile("mfence");

    memory_flush(pointer01); //flush
    delta = end - start;

    if (delta < threshold)
    {
        correct_end_time = start;
        printf("cpu_end delta : %ld \n", delta);
    }

    return delta;
}

//==================================================cache hits and misses =================================================================

uint64_t cache_hit()
{

    size_t start = 0, end = 0, time = 0, delta = 0;
    int i = 0, j = 0;
    int *pointer01 = (int *)get_cpu_time_start + 27;
    maccess(pointer01);
    for (j = 0; j < 100; ++j)
    {
        asm volatile("mfence");
        start = timestamp();
        asm volatile("mfence");

        maccess(pointer01);

        asm volatile("mfence");
        end = timestamp();
        asm volatile("mfence");
        delta = end - start;
        time = time + end - start;
    }
    return time / 100;
}

uint64_t cache_miss()
{

    size_t start = 0, end = 0, time = 0, delta = 0;
    int i = 0, j = 0;

    for (j = 0; j < 100; ++j)
    {
        int *pointer01 = (int *)get_cpu_time_start + 27;
        memory_flush(pointer01);
        asm volatile("mfence");
        start = timestamp();
        asm volatile("mfence");

        maccess(pointer01);

        asm volatile("mfence");
        end = timestamp();
        asm volatile("mfence");
        delta = end - start;
        time = time + end - start;
    }
    return time / 100;
}

//========================================================== assembly_functions  =========================================================================
uint64_t timestamp()
{
    uint64_t a = 0, d = 0;
    asm volatile("mfence");
    asm volatile("rdtsc"
                 : "=a"(a), "=d"(d));
    return ((((d << 32) | a)) / 1);
}

void memory_flush(void *p)
{

    asm volatile("clflush 0(%0)\n"
                 :
                 : "r"(p)
                 : "rax");
    // asm volatile("mfence");
}
void maccess(void *p)
{
    asm volatile("movq (%0), %%rax\n"
                 :
                 : "c"(p)
                 : "rax");
}

void hexprint(char *in, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        printf("%02x ", (unsigned char)(in[i]));
    }
}

//=================================================================***main***========================================================================
void main()
{
    unsigned char token[64], plain_token[64], key[16];
    size_t time_to_miss, time_to_hit, seed = 0, j = 0;
    int sp, ep,iteration = 0;
    pthread_t threads[THREAD_NUMBER];
    pthread_t thread2[THREAD_NUMBER];

    printf(COLOR_CYAN);
    printf("_________                      _____            ________     ______              \n");
    printf("__  ____/___________  ___________  /______      ___  __/________  /_____________ \n");
    printf("_  /    __  ___/_  / / /__  __ \\  __/  __ \\     __  /  _  __ \\_  //_/  _ \\_  __ \\\n");
    printf("/ /___  _  /   _  /_/ /__  /_/ / /_ / /_/ /     _  /   / /_/ /  ,<  /  __/  / / /\n");
    printf("\\____/  /_/    _\\__, / _  .___/\\__/ \\____/      /_/    \\____//_/|_| \\___//_/ /_/ \n");
    printf("               /____/  /_/                                                       \n\n");
    printf(COLOR_RESET);
    
    printf("[" COLOR_RED "*" COLOR_RESET "] Generating encrypted token...\n");


    //-----------DETERMINE THRESHOLD----------
    time_to_hit = cache_hit();
    time_to_miss = cache_miss();
    printf("\n Average Time to miss= %zu ,Average Time to hit = %zu", time_to_miss, time_to_hit);
    threshold = (time_to_hit + time_to_miss) / 2;

    printf("\n Threshold = %zu \n", threshold);


    //---------INTIALIZE PTHREAD--------------
    int *pointer01 = (int *)get_cpu_time_start + 27;
    int *pointer02 = (int *)get_cpu_time_end + 24;
    memory_flush(pointer01);
    memory_flush(pointer02);

    sp = pthread_create(&threads[j], NULL, attacker_thread_01, NULL);
    ep = pthread_create(&thread2[j], NULL, attacker_thread_02, NULL);

    usleep(1);

   //-------------CALCULATE TOKEN-------------------
    libenc_get_token(token);
    printf("[" COLOR_YELLOW "*" COLOR_RESET "] Encrypted token: " COLOR_YELLOW);
    hexprint(token, 64);
    printf(COLOR_RESET "\n");

    strcpy(plain_token, "<TODO>");
    // TODO: find plaintext token
    //printf("ts1 = %zu", correct_start_time);
    //printf("ts2 = %zu", correct_end_time);
    correct_time = correct_end_time - correct_start_time + MAGIC_CONST;
    // printf("seed_start = %zu",correct_time);

    //--------------iterations 50000 offset------------
    for (size_t itr = correct_time - 50000; itr < correct_time + 50000; itr++)
    {
        iteration++;
        ;
        srand(itr);
        for (int i = 0; i < 16; i++)
        {
            key[i] = rand() % 256;
        }

        libenc_decrypt(token, 64, plain_token, key);

        if (plain_token[0] == 'S' && plain_token[1] == 'C' && plain_token[2] == 'A' && plain_token[3] == 'D')
        {
            //printf("iteration to obtain the seed %d\n", iteration - 50000);
            for (int n = 0; n < 64; n++)
            {
             //printf("%c", plain_token[n]);
           }
           break;
        }
    }

    printf("[" COLOR_GREEN "*" COLOR_RESET "] Key: " COLOR_GREEN);
    hexprint(key, 16);
    printf(COLOR_RESET "\n");
    printf("[" COLOR_GREEN "*" COLOR_RESET "] Plain token: " COLOR_GREEN "%s" COLOR_RESET "\n", plain_token);
}

//=============================================== attacker threads ==================================================================
void *attacker_thread_01(void *arg)   // monitors start function
{

    unsigned long start_time = 0, before_flush, before_sleep;
    int *pointer01 = (int *)get_cpu_time_start + 27;
    memory_flush(pointer01);

    for (int i = 0; i < 8040000; i++) 
    {

        usleep(1);

        start_time = flush_reload_start();           

        if (start_time < threshold)
        {
            sched_yield();
            break;
        }
    }
    pthread_exit(NULL);
    return NULL;
}

void *attacker_thread_02(void *arg)  // monitors end function
{

    unsigned long end_time = 0, before_flush, before_sleep;
    int *pointer02 = (int *)get_cpu_time_end + 24;
    memory_flush(pointer02);

    for (int i = 0; i < 8040000; i++)
    {

        usleep(1);

        end_time = flush_reload_end();

        if (end_time < threshold)
        {
            sched_yield();
            break;
        }
    }
    pthread_exit(NULL);
    return NULL;
}
