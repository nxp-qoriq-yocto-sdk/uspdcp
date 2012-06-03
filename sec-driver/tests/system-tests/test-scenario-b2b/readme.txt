Benchmark application for SEC PDCP user-space driver that exercises the following:
- use 2 job rings
- use 1 thread
- encapsulates a job through one JR, decapsulates through the other:
    sec_process_packet(pkt_to_encap,JR0) -------------------------> sec_poll(JR0)
       |-> notify_packet -> sec_process_packet(encap'ed_pkt,JR1) -> sec_poll(JR1)
                                |-> memcmp(decap'ed pkt, pkt_to_encap)
- packets generated randomly at startup
- the last byte in the input packet is incremented with the context ID
- use PDCP data/control-plane with all the combinations supported by the driver
- user-configurable number of PDCP contexts, packet size, number of packets
- retrieves processed packets using polling method.
