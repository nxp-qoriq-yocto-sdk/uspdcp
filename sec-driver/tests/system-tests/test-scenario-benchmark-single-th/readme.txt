Benchmark application for SEC PDCP user-space driver that exercises the following:
- use 2 job rings
- use 1 threads: 
- generate packets in sw, i.e hardcode the packets
- test the following algos: SNOW F8 encapsulation
- configurable number of PDCP contexts, packet size, number of packets.
- gives core cycles/packet and execution time.
