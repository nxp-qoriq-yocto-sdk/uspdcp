Test application for SEC PDCP user-space driver that exercises the following:
- SEC driver working mode: POLL, IRQ, NAPI
- use 2 job rings
- use 2 threads: one thread is producer for a job ring and consumer for the
  other. Reverse for the second thread
- generate packets in software, i.e hardcode the packets
- test individually the following:
    - PDCP data plane - with algorithms: SNOW F8, AES CTR, NULL
    - PDCP control plane - with algorithms:
            - SNOW F8 + SNOW F9
            - AES CTR + AES CMAC
            - SNOW F8 + AES CMAC
            - AES CTR + SNOW F9
            - NULL + SNOW F9
            - NULL + AES CMAC
            - NULL + NULL
  Validate correctness against reference data, where available.
- delete contexts with packets in flight
