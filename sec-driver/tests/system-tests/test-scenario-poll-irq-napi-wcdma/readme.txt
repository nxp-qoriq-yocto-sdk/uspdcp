Test application for SEC WCDMA user-space driver that exercises the following:
- SEC driver working mode: POLL, IRQ, NAPI
- use 2 job rings
- use 2 threads: one thread is producer for a job ring and consumer for the
  other. Reverse for the second thread
- random number of WCDMA contexts, random number of packets per context.
- the packets used are generated offline using reference code from standard
  specifications, by randomizing the parameters (keys, input, etc)
- supports the following algorithms:
    - RLC NULL
    - RLC KASUMI f8 encryption
    - RLC SNOW f8 encryption
- each algorithm is exercised in both UL and DL scenarios, as well as in
  encapsulation and decapsulation scenarios
- delete contexts with packets in flight
