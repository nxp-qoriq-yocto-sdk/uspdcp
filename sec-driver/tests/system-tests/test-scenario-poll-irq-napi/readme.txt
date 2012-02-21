Test application for SEC PDCP user-space driver that exercises the following:
- SEC driver working mode: POLL, IRQ, NAPI
- use 2 job rings
- use 2 threads: one thread is producer for a job ring and consumer for the
  other. Reverse for the second thread
- random number of PDCP contexts, random number of packets per context.
- the packets used are generated offline using reference code from standard
  specifications, by randomizing the parameters (keys, input, etc)
- supports the following algorithms:
    - PDCP data plane:
        - SNOW F8 encryption
        - AES CTR encryption
        - NULL encryption
    - PDCP control plane:
        - SNOW F8 encryption + SNOW F9 integrity
        - AES CTR encryption + AES CMAC
        - SNOW F8 encryption + NULL integrity
        - AES CTR encryption + NULL integrity
        - NULL encryption + SNOW F9 integrity
        - NULL encryption + AES CMAC integrity
        - NULL encryption + NULL integrity
- each algorithm is exercised in both UL and DL scenarios, as well as in
  encapsulation and decapsulation scenarios
- delete contexts with packets in flight
