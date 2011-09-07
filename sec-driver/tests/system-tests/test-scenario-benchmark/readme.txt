Benchmark application for SEC PDCP user-space driver that exercises the following:
- use 2 job rings
- use 2 threads: one thread is producer for a job ring and consumer for the
  other. Reverse for the second thread
- generate packets in sw, i.e hardcode the packets
- use PDCP data-plane with the following algos: SNOW, AES
- retrieves processed packets using polling method.
- configurable number of PDCP contexts, packet size, number of packets.
- gives core cycles/packet and execution time.
- can be used to run the test in infinite loop, aiming to obtain CPU load with
  Linux's <top> tool.
- retrieves processed packets using polling method.
- Benchmark results (core cycles, execution times and CPU load) were
  retrieved for:
  --> number of PDCP contexts = 512 (=256 DRBs) or 2(=1 DRB)
  --> packet size = 1000 bytes
  --> Downlink (DL) packets per second = 19000
  --> Uplink (UL) packets per second = 10000
