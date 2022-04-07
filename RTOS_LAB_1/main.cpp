#include <cstdlib>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <cstdio>
#include <sys/types.h>
#include <pthread.h>
#include <fcntl.h>
#include <vector>
#include <time.h>

#define max_file_size 5000
#define separator "-------------------------------------------------------------------------"
#define start_separator "___________________________start_programm_________________________________"
#define end_separator "____________________________end_programm__________________________________"

struct cmdArgs
{
    size_t a;
    size_t c;
    size_t m;
    size_t seed;
    char* inputFilePath;
    char* outputFilePath;
};

struct lkgGenParam{
    size_t a;
    size_t c;
    size_t m;
    size_t seed; // x0 argument - start
    size_t sizeKey; // input file size
};

struct worker
{
    char* msg; // buf with input.txt content
    char* random_subsequence;
    char* outputText;
    size_t size;
    // Fragments of notepad text
    size_t downIndex;
    size_t topIndex;
    pthread_barrier_t* barrier;
};

void* lkg(void* params)
{
    lkgGenParam *parametrs = reinterpret_cast<lkgGenParam *>(params);
    size_t a = parametrs->a;
    size_t m = parametrs->m;
    size_t c = parametrs->c;
    size_t sizeKey = parametrs->sizeKey;

    int* buff = new int[sizeKey / sizeof(int) + 1];
    buff[0] = parametrs->seed;

    for(size_t i = 1; i < sizeKey / sizeof(int) + 1; i++)
    {
        buff[i]= (a * buff[i-1] + c) % m;
    }

    return reinterpret_cast<char *>(buff);
}

void* crypt(void * cryptParametrs)
{
    worker* param = reinterpret_cast<worker*>(cryptParametrs);
    size_t topIndex = param->topIndex;
    size_t downIndex = param->downIndex;
    // std:: cout << "downIndex = " << downIndex << "\ttopIndex = " << topIndex << std:: endl;
    
    while(downIndex < topIndex)
    { 
        param->outputText[downIndex] = param->random_subsequence[downIndex] ^ param->msg[downIndex];
        // std:: cout << param->msg[downIndex];
        downIndex++;
    }

    int status = pthread_barrier_wait(param->barrier);
    if (status != 0 && status != PTHREAD_BARRIER_SERIAL_THREAD) {
        exit(status);
    }

    return nullptr;
}

void clear_memory(char* outputText, char* msg)
{
    delete[] outputText;
    delete[] msg;
}

int main (int argc, char **argv) 
{
    std:: cout << start_separator << std:: endl;
    std:: cout << separator << std:: endl;
    int c;
    cmdArgs args_cmd;
    while ((c = getopt(argc, argv, "i:o:a:c:x:m:")) != -1) 
    {
        switch (c) 
        {
            case 'i':
                {
                    std:: cout << "option i with value\t" << optarg << std:: endl;
                    args_cmd.inputFilePath = optarg;
                    break;
                }
            case 'o':
                {
                    std:: cout << "option o with value\t" << optarg << std:: endl;
                    args_cmd.outputFilePath = optarg;
                    break;
                }
            case 'a':
                {
                    std:: cout << "option a with value\t" << optarg << std::endl;
                    args_cmd.a = atoi(optarg);
                    break;
                }
            case 'c':
                {
                    std:: cout << "option c with value\t" << optarg << std:: endl;
                    args_cmd.c = atoi(optarg);
                    break;
                }
            case 'm':
                {  
                    std:: cout << "option m with value\t" << optarg << std:: endl;
                    args_cmd.m = atoi(optarg);
                    break;
                }
            case 'x':
                {
                    std:: cout << "option x with value\t" << optarg << std:: endl;
                    args_cmd.seed = atoi(optarg);
                    break;
                }
            case '?':
                break;
            default:
                std:: cout << "?? getopt returned character code 0 ??\t" << c << std:: endl;
        }
    }

    if (optind < argc) {
        std:: cout << "Unrecognized command line elements: " << std:: endl;
        while (optind < argc)
            std:: cout << argv[optind++] << '\t';
        std:: cout << std:: endl;
    }

    std:: cout << separator << std:: endl; 

    int inputFile = open(args_cmd.inputFilePath, O_RDONLY);
    if (inputFile == -1)
    {
        std::cerr << "Unable to open " << args_cmd.inputFilePath << " file\n";
        exit(-1);
    }

    int inputSize = lseek(inputFile, 0, SEEK_END);
    std::cout << "input file size = " << inputSize << std::endl;
    if(inputSize == -1)
    {
        std::cerr << "error with " << args_cmd.inputFilePath << "\nUnable to get file size\n";
        exit(-1);
    }

    // check file is empty
    if (inputSize == 0)
    {
        std:: cerr << "File is empty" << std::endl;
        exit(-1);
    }

    // check max_file_size
    if (inputSize > 5000)
    {
        std:: cerr << "file size is too large\n max_file_size  = " << max_file_size << "\tcurrent_size_file = " << inputSize << std:: endl;
        exit(-1);
    }
    
    std:: cout << separator << std:: endl;

    char* random_subsequence;
    char* outputText = new char[inputSize];
    char* msg = new char[inputSize]; // text buffer

    // Return cursor to the beginning of the file, from previous end position
    lseek(inputFile, 0, SEEK_SET);

    // Read in buffer
    if(read(inputFile, msg, inputSize) == -1)
    {
        std::cerr << "Can't read to buffer";
        clear_memory(outputText, msg);
        exit(-1);
    }

    // The number of processors currently online
    int count_worker_thread = sysconf(_SC_NPROCESSORS_ONLN);
    std:: cout << "Currently available processes:\t" <<  count_worker_thread << std:: endl;

    lkgGenParam lkgParam;
    lkgParam.sizeKey = inputSize ;
    lkgParam.a=args_cmd.a;
    lkgParam.c=args_cmd.c;
    lkgParam.m=args_cmd.m;
    lkgParam.seed=args_cmd.seed;

    pthread_t keyGenThread;
    pthread_t cryptThread[count_worker_thread];

    if (pthread_create(&keyGenThread, NULL, lkg, &lkgParam) != 0)
    {
        std::cerr << "Unable to create a new thread";
        clear_memory(outputText, msg);
        exit(-1);
    }

    size_t random_subsequence_thread_status = pthread_join(keyGenThread, (void**)&random_subsequence);
    if(random_subsequence_thread_status != 0)
    {
        std::cerr << "Unable to join random_subsequence thread. Error code: " << random_subsequence_thread_status;
        clear_memory(outputText, msg);
        exit(-1);
    }

    pthread_barrier_t barrier;

    pthread_barrier_init(&barrier, NULL, count_worker_thread + 1);
    std::vector <worker*> workers;

    for(int i = 0; i < count_worker_thread; i++)
    {
        worker* workerParams = new worker;

        workerParams->random_subsequence = random_subsequence;
        workerParams->size = inputSize;
        workerParams->outputText = outputText;
        workerParams->msg = msg;
        workerParams->barrier = &barrier;

        size_t current_len = inputSize / count_worker_thread;
        
        workerParams->downIndex = i * current_len;
        workerParams->topIndex = i * current_len + current_len;

        if (i == count_worker_thread - 1)
        {
            workerParams->topIndex = inputSize;
        }

        workers.push_back(workerParams);
        pthread_create(&cryptThread[i], NULL, crypt, workerParams);
    }
    // block main thread

    int status = pthread_barrier_wait(&barrier);

    if (status != 0 && status != PTHREAD_BARRIER_SERIAL_THREAD) 
    {
        clear_memory(outputText, msg);
        for (auto & _worker : workers) 
        {
            delete _worker;
        }
        exit(status);
    }

    int output;
    if ((output=open(args_cmd.outputFilePath, O_WRONLY)) == -1) 
    {
        std::cerr << "Cannot open file\t" << args_cmd.outputFilePath << std::endl;
    }
    else
    {
        if(write(output, outputText, inputSize) != inputSize)
            std:: cerr << "Write Error" << std:: endl;

        close(output);
    }

    pthread_barrier_destroy(&barrier);


    delete[] random_subsequence;
    delete[] outputText;
    delete[] msg;

    for (auto & _worker : workers) 
    {
        delete _worker;
    }
    std:: cout << separator << std:: endl;
    std:: cout << end_separator << std::endl << std:: endl;
    return 0;
}
