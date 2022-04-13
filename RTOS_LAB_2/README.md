## Task 2 Single-threaded resource manager (RM) "Pseudorandom number generator BBS”
### Theoretical information

Previously, a linear congruent generator was used to generate a pseudorandom sequence . This generator has good performance, but a short sequence repetition period and other anomalies, which makes it unsuitable for use in cryptographic tasks.
[The BBS(Blum-Blum-Shub) algorithm](https://en.wikipedia.org/wiki/Blum_Blum_Shub), on the contrary, allows to obtain a highly cryptographically stable sequence, but is characterized by slow operation. The calculation is performed using a recurrent formula:

<img src="https://latex.codecogs.com/svg.image?X_{n&plus;1}\equiv&space;X_{n}^2\mod{M}" title="X_{n+1}\equiv X_{n}^2\mod{M}" />,

where <img src="https://latex.codecogs.com/svg.image?M&space;=&space;pq" title="M = pq" />, <img src="https://latex.codecogs.com/svg.image?p" title="p" /> and <img src="https://latex.codecogs.com/svg.image?q" title="q" /> are prime numbers such that <img src="https://latex.codecogs.com/svg.image?M" title="M" /> is a [Blum number](https://oeis.org/A016105).

The output of the generator  <img src="https://latex.codecogs.com/svg.image?Y_{n&plus;1}" title="Y_{n+1}" />  will be the parity bit of the element <img src="https://latex.codecogs.com/svg.image?X_{n&plus;1}" title="X_{n+1}" /> :

<img src="https://latex.codecogs.com/svg.image?Y_{n&plus;1}&space;=&space;paritybit(X_{n&plus;1})" title="Y_{n+1} = paritybit(X_{n+1})" />

The generator parameters, thus, will be <img src="https://latex.codecogs.com/svg.image?X_{0}" title="X_{0}" /> (seed, initial state), as well as the values of <img src="https://latex.codecogs.com/svg.image?p" title="p" /> and <img src="https://latex.codecogs.com/svg.image?q" title="q" />.

### Description of the "Client-RM" interaction protocol

- The protocol should be described in the shared header file bbs.h.
- The command codes are defined using the macros [___DIOF and_ __DION_](https://www.qnx.com/developers/docs/6.5.0SP1.update/com.qnx.doc.neutrino_lib_ref/d/devctl.html).
- The "Client-RM" interaction is carried out via the POSIX interface using the [_open_](http://www.qnx.com/developers/docs/6.5.0/index.jsp?topic=%2Fcom.qnx.doc.neutrino_lib_ref%2Fo%2Fopen.html), [_devctl_](https://www.qnx.com/developers/docs/6.5.0SP1.update/com.qnx.doc.neutrino_lib_ref/d/devctl.html), [_close_](http://www.qnx.com/developers/docs/6.5.0/index.jsp?topic=%2Fcom.qnx.doc.neutrino_lib_ref%2Fo%2Fopen.html) functions.
- The requests described below are passed through the devctl function using a valid descriptor obtained by successfully calling the _open_ function:

  | Request 1  | Setting the generator parameters |
  | ------ | ------ |
  | Request Code | 1 |
  | Input data | structure containing generator parameters: `struct BBSParams {uint32_t seed;  uint32_t p; uint32_t q;} `|
  | Output data | No |

  | Request 2  | Getting the pseudorandom sequence element |
  | ------ | ------ |
  | Request Code | 2 |
  | Input data | No|
  | Output data | `uint32_t` |


### Client development
Realize the cryptobbs-client client program for the QNX Neutrino 6.6 (x86) platform. The primacy of client development can be understood in the context of the TDD (test-driven-development) concept, when top-level code using a certain interface is developed first, before implementing the interface itself. This makes it possible to think better in advance about solving a number of problems, including the specifics of interface usage scenarios and error handling.

#### Requirements:
1.	Open a connection with the RM using the `open("/dev/cryptobbs")` function.
2.	Set the parameters of the BBS generator (<img src="https://latex.codecogs.com/svg.image?p&space;=&space;3,&space;q&space;=&space;263,&space;seed&space;=&space;866" title="p = 3, q = 263, seed = 866" />) using the[`devctl()`](https://www.qnx.com/developers/docs/6.5.0SP1.update/com.qnx.doc.neutrino_lib_ref/d/devctl.html) function.
3.	In the loop, send requests to the RM using the [`devctl()`](https://www.qnx.com/developers/docs/6.5.0SP1.update/com.qnx.doc.neutrino_lib_ref/d/devctl.html) function with the requirement to receive the next element of the pseudorandom sequence.
4.  Save the next element to a fixed-length 1024 vector, working on the principle of a ring buffer.
5.	The cycle ends when the user presses Ctrl + C (the _SIGINT_ process receives).
6.	The contents of the vector are output to `stdout`.
7.  The connection to the RM is closed using the [`close()`](http://www.qnx.com/developers/docs/6.5.0/index.jsp?topic=%2Fcom.qnx.doc.neutrino_lib_ref%2Fo%2Fopen.html) function.


### Resource Manager Development

Realize a cryptobbs resource manager for the _QNX Neutrino 6.6 (x86)_ platform that meets the following requirements,
#### Requirements
- uses the [skeleton of a single-threaded RM](http://www.qnx.com/developers/docs/7.0.0/#com.qnx.doc.neutrino.resmgr/topic/skeleton_SIMPLE_ST_EG.html) as a basis;
- [registered](http://www.qnx.com/developers/docs/7.0.0/#com.qnx.doc.neutrino.lib_ref/topic/r/resmgr_attach.html) and functions using the _QNX Neutrino OS resmgr_library_ , the mount point is _"/dev/cryptobbs”_;
- realizes the processing of the client request using the [`io_devctl()`](https://it.wikireading.ru/2461) function, passed to the _io_funcs_ library through the corresponding pointer structure in the skeleton;
- it works according to the "Client-RM” interaction protocol , uses the bbs.h file;
- extracts the data structure from the message (for Protocol request 1), converts the `void*` type to the BBSParams structure type using `reinterpret_cast`;
- validates the data, in case of an error, passes it to the client;	
- upon request 2 of the Protocol, it generates another element of the pseudorandom sequence using the BBS algorithm according to the parameters passed in request 1 and sends it to the client.

### Test
- RM launch.
- client launch.
- Client completion by SIGINT.
- Checking the client's output.