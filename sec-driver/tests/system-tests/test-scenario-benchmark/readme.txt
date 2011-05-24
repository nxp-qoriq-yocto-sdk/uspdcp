Benchmark application for SEC PDCP user-space driver that exercises the following:
- use 2 job rings
- use 2 threads: one thread is producer for a job ring and consumer for the
  other. Reverse for the second thread
- generate packets in sw, i.e hardcode the packets
- test individually the following algos: SNOW F8, SNOW F9, AES CMAC, AES CTR.
- configurable number of PDCP contexts, packet size, number of packets.
- gives core cycles/packet
