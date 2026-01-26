# Libpcap Common Functions (C)

1. Opening a Device
   char errbuf[PCAP_ERRBUF_SIZE];
   pcap_t *handle = pcap_open_live(device, BUFSIZ, 1, 1000, errbuf);
   // Args: device, snapshot_len, promiscuous, timeout_ms, error_buffer

2. Compiling Filters
   struct bpf_program fp;
   pcap_compile(handle, &fp, "port 80", 0, net);
   pcap_setfilter(handle, &fp);

3. The Loop
   void packet_handler(u_char *args, const struct pcap_pkthdr *header, const u_char *packet);
   pcap_loop(handle, 0, packet_handler, NULL);
   // Note: '0' means infinite loop.

4. Parsing Headers (Offsets)
   Ethernet Header: 14 bytes
   IP Header: 20 bytes (usually)
   TCP Header: 20 bytes (usually)
   Use `ntohs()` for ports and `ntohl()` for IP addresses!
