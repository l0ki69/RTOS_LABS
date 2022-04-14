#include "bbs.h"
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <iostream>
#include <vector>

#define SEED 866
#define P  3
#define Q  263
#define VECTOR_SIZE 1024
//#define VECTOR_SIZE 10 // test size

bool stop_signal = false;

void signalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    stop_signal = true;
}

int main(int argc, char **argv) {
    signal(SIGINT, signalHandler);

    // open a connection to the server (fd == coid)
    int fd = open("/dev/cryptobbs", O_RDWR);
    if (fd < 0)
    {
        std::cerr << "E: unable to open server connection: " << strerror(errno) << std::endl;
        return EXIT_FAILURE;
    }

    bbs::BBSParams param;
    param.seed = SEED;
    param.p = P;
    param.q = Q;

    std::cout << "Set generator params" << std::endl;
    int error;
    if ((error = devctl(fd, SET_GEN_PARAMS, &param, sizeof(param), NULL)) != EOK)
    {
        fprintf(stderr, "Error setting RTS: %s\n", strerror(error));
        exit(EXIT_FAILURE);
    };

    std::vector <std::uint32_t> psp_vector(VECTOR_SIZE);
    std::uint32_t elem;
    int counter = -1;
    std::cout << "Get elements" << std::endl;
    while (!stop_signal)
    {
        if ((error = devctl(fd, GET_ELEMENT, &elem, sizeof(elem), NULL)) != EOK)
        {
            fprintf(stderr, "Error setting RTS: %s\n",
                    strerror(error));
            exit(EXIT_FAILURE);
        };

        counter++;

        if (counter == VECTOR_SIZE)
        	counter = 0;

		psp_vector.at(counter) = elem;
        sleep(5);
        // Для удобного тестирования лучше поставить sleep(5) секуд
    }

    std::cout << "Output vector" << std::endl;
    for (auto &_el: psp_vector)
        std::cout << _el << std::endl;

    close(fd);
    std::cout << "ok";

    return EXIT_SUCCESS;
}
