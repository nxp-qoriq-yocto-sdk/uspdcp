Benchmark application for SEC PDCP user-space driver that exercises the following:
- use 2 job rings
- use 1 thread
- generate packets in software randomly
- support PDCP Data Plane and PDCP Control Plane with all the algorithm combinations
- Scatter Gather support with a configurable number of fragments (see below)
- for DL, input consists of user-configurable number of Scatter-Gather fragments,
  and the output is a single continous buffer; reverse for UL.
- configurable number of PDCP contexts, packet size, number of packets
- gives core cycles/packet and execution time.
- can be used to run the test in infinite loop, aiming to obtain CPU load with
  Linux's <top> tool.
- retrieves processed packets using polling method.
- the following parameters are used to configure the app's behavior:
    -t Selects the test type to be used. It is used for selecting PDCP Control Plane or PDCP User Plane encapsulation/decapsulation
                Valid values:
                        o CONTROL or CPLANE
                        o DATA or UPLANE

    -l Selects the header length. It is valid only for PDCP Data Plane.
            Valid values:
                    o SHORT or 7 - Header Length is 1 byte and SN is assumed to be 7 bits
                    o LONG or 12 - Header Lengt is 2 bytes and SN is assumed to be 12 bits

    -d Selects the packet direction
            Valid values:
                    o UL or UPLINK - The packets will have the direction bit set to 0
                    o DL or DOWNLINK - The packets will have the direction bit set to 1

    -e Select the encryption algorithm to be used.
            Valid values are:
                    o SNOW
                    o AES
                    o NULL

    -a Select the integrity algorithm to be used.
            Valid for Control Plane only, see -t option
            Valid values are:
                    o SNOW
                    o AES
                    o NULL

    -f Select the number of Scatter-Gather fragments in which a packet is split
            NOTE: It cannot exceed 16

    -s Select the maximum payload size of the packets.
            NOTE: It does NOT include the header size.

    -n Number of iterations to be run.
            NOTE: Setting this to 0 will result in endless looping

