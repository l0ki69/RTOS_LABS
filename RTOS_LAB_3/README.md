## Task 3 Development of a multithreaded QNX Neutrino resource manager
### Problem
Develop a multithreaded version of the BBS PRNG resource Administrator cryptobbs v2.0 that supports correct parallel work with multiple clients.
### Requirements
- [The QNX Neutrino thread pool](http://www.qnx.com/developers/docs/6.5.0/index.jsp?topic=%2Fcom.qnx.doc.neutrino_resmgr%2Fmultithread.html) should be used as a tool for organizing a multithreaded scheme;
- the protocol of interaction "cryptobbs-client" v 2.0 should support working with sessions, each client should be able to generate “his" pseudorandom sequence with individual parameters;
- the sender ID of the incoming message is used as the client ID;
- to provide storage of interaction contexts with each client inside the RM, allocation of such a context (_io_open_) and destruction (_io_close_).