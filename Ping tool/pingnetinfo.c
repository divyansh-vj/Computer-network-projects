#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <sys/time.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <poll.h>
#include<limits.h>
#include<float.h>
#include <netinet/tcp.h>

#define MAX_TTL 25
#define MAX_PROBES_TRIES 7

// // NEEEEEEEEEEEEEEEED TO CHANGEEEEEEEEEEEEEEEEEE COPIEDDDDDDDDD FOR NOWWWWWWWWWWWWWWWWWWWWWW
// double timespec_diff(struct timespec *start, struct timespec *stop) {
//     double timediff_ms = 0;

//     if ((stop->tv_nsec - start->tv_nsec) < 0) {
//         timediff_ms += (stop->tv_sec - start->tv_sec - 1) * 1000;
//         timediff_ms += (stop->tv_nsec - start->tv_nsec + 1000000000) / (double)(1000000);
//     } else {
//         timediff_ms += (stop->tv_sec - start->tv_sec) * 1000;
//         timediff_ms += (stop->tv_nsec - start->tv_nsec) / (double)(1000000);
//     }

//     return timediff_ms;
// }

unsigned short checksum(const unsigned short *data, unsigned int size) {
    unsigned int sum = 0;

    for (unsigned int i = 0; i < size; ++i) {
        unsigned int value = data[i];
        sum += value;
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (unsigned short)(~sum);
}

double timespec_diff(struct timespec *start, struct timespec *stop) {
    double timediff_ms = 0;
    timediff_ms += (stop->tv_sec - start->tv_sec) * 1000 + (stop->tv_nsec - start->tv_nsec) / (double)(1000000);
    return timediff_ms;
}



void print_header(char *buf){
    struct iphdr* ip = (struct iphdr*)buf;
    if(ip->protocol == IPPROTO_ICMP){
        struct icmphdr* icmp = (struct icmphdr*)(buf + (ip->ihl * 4));
        // printf("ICMP: type = %d, code = %d, checksum = %d, id = %d, seq = %d\n", icmp->type, icmp->code, icmp->checksum, icmp->un.echo.id, icmp->un.echo.sequence);
        printf("ICMP Header: \ntype = %d \ncode = %d \nchecksum = %d\n", icmp->type, icmp->code, icmp->checksum);
        if((icmp->type == 0) || (icmp->type == 8) || (icmp->type == 13) || (icmp->type == 14) || (icmp->type == 17) || (icmp->type == 18)){
            printf("id = %d \nseq = %d\n\n", icmp->un.echo.id, icmp->un.echo.sequence);
        }
        else if(icmp->type == 5){
            printf("Gateway IP = %s\n\n", inet_ntoa(*(struct in_addr*)&ip->saddr));
        }
        else if(icmp->type == 12){
            printf("Pointer = %d \nUnused = 0\n\n", icmp->un.frag.mtu);
        }
        else{
            printf("unused = 0\n\n");
        }
        if((icmp->type == 3) || (icmp->type == 4) || (icmp->type == 5) || (icmp->type == 12)){
            struct iphdr* ip_reply = (struct iphdr*)(buf + (ip->ihl * 4) + 8);
            printf("IP Header: \nversion = %d \nheader length = %d \ntype of service = %d \ntotal length = %d \nidentification = %d \nfragment offset = %d \nttl = %d \nprotocol = %d \nchecksum = %d \nsource IP = %s \ndestination IP = %s\n\n", ip_reply->version, ip_reply->ihl, ip_reply->tos, ip_reply->tot_len, ip_reply->id, ip_reply->frag_off, ip_reply->ttl, ip_reply->protocol, ip_reply->check, inet_ntoa(*(struct in_addr*)&ip_reply->saddr), inet_ntoa(*(struct in_addr*)&ip_reply->daddr));
            if(ip_reply->protocol == IPPROTO_TCP){
                struct tcphdr* tcp = (struct tcphdr*)(buf + (ip->ihl * 4) + 8 + (ip_reply->ihl * 4));
                printf("TCP Header: \nsource port = %d \ndestination port = %d \nsequence number = %d \nacknowledgement number = %d \nheader length = %d \nflags = %d \nwindow size = %d \nchecksum = %d \nurgent pointer = %d\n\n", tcp->source, tcp->dest, tcp->seq, tcp->ack_seq, tcp->doff, tcp->fin, tcp->window, tcp->check, tcp->urg_ptr);
            }
            else if(ip_reply->protocol == IPPROTO_UDP){
                struct udphdr* udp = (struct udphdr*)(buf + (ip->ihl * 4) + 8 + (ip_reply->ihl * 4));
                printf("UDP Header: \nsource port = %d \ndestination port = %d \nlength = %d \nchecksum = %d\n\n", udp->source, udp->dest, udp->len, udp->check);
            }
            else{
                printf("Unknown higher level protocol\n\n");
            }
        }
    }
}



// int checksum(char *buff, int len){
//     char *temp_buf = (char *)malloc(len);
//     memcpy(temp_buf, buff, len);
//     int sum = 0,temp=0;
//     int a,b;
//     for(int i=0;i<(len);i+=2){
//         a = temp_buf[i];
//         a &= 0xff;
//         a = a << 8;
//         b = temp_buf[i+1];
//         b &= 0xff;
//         // temp = temp_buf[i] << 8 | temp_buf[i+1];
//         temp = a | b;
//         sum += temp;
//         // printf("sum is %d and temp is %d\n", sum, temp);
//         temp = sum;
//         sum += (temp >> 16);
//         sum = sum & 0xffff;
//     }
//     return ((~sum) & 0xffff);
// }




typedef struct {
    int route_len;
    struct in_addr route[MAX_TTL];
    int valid[MAX_TTL];
    double latency[MAX_TTL];
    double bandwidth[MAX_TTL];
    double RTT[MAX_TTL];
} route_t;




int main(int argc, char const *argv[]){
    struct iphdr* ip;
    // struct iphdr* ip_reply;
    struct icmphdr* icmp;
    struct in_addr *dest_ip_address, *self_ip_address;
    struct hostent *host;
    struct sockaddr_in src, dest;
    struct timespec start, ending;
    char buf[4096], recv_buf[4096];
    if(argc != 4){
        printf("Please enter command correctly, need 3 inputs\n");
        exit(0);
    }
    route_t route_table;
    route_table.route_len = 0;

    dest_ip_address = (struct in_addr *)malloc(sizeof(struct in_addr));
    self_ip_address = (struct in_addr *)malloc(sizeof(struct in_addr));

    int num_probes = atoi(argv[2]);
    int time_diff = atoi(argv[3]);

    // check if ip or hostname
    if (inet_pton(AF_INET, argv[1], dest_ip_address) == 1) {
    }
    // if(check(argv[1])){
        
    //     //if normal ip what do help divyansh
    //     inet_aton(argv[1], dest_ip_address);
    // }
    else{
        host = gethostbyname(argv[1]);
        if(host == NULL || ((struct in_addr **) host->h_addr_list) == NULL){
            printf("Error in getting host IP address given name\n");
        }
        struct in_addr ** addr_list = (struct in_addr **)host->h_addr_list;
        dest_ip_address = addr_list[0];
    }

    
    struct ifaddrs *ifaddr;
    if(getifaddrs(&ifaddr) < 0) {
        perror("Failed to get network interfaces.\n");
        exit(0);
    }
    
    for (struct ifaddrs *it = ifaddr; it != NULL; it = it->ifa_next) {
        if(it->ifa_addr != NULL && it->ifa_addr->sa_family == AF_INET && (strcmp(it->ifa_name, "lo") != 0) && (it->ifa_flags & IFF_UP)) {
            // self_ip_address = (struct in_addr *)malloc(sizeof(struct in_addr));
            *self_ip_address = ((struct sockaddr_in *)(it->ifa_addr))->sin_addr;
            break;
        }
    }
    
    dest.sin_family = AF_INET;
    dest.sin_port = htons(40000);
    dest.sin_addr = *dest_ip_address;
    src.sin_family = AF_INET;
    src.sin_port = htons(20000);
    src.sin_addr.s_addr = (self_ip_address == NULL) ? INADDR_ANY : self_ip_address->s_addr;

    int sock;
    sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(sock < 0){
        printf("Error in making socket\n");
        exit(0);
    }
    
    int one = 1;
    const int *val = &one;
    if (setsockopt (sock, IPPROTO_IP, IP_HDRINCL, val, sizeof (one)) < 0){
        printf ("ERROR setting\n");
    }

    //IP Header
    for(int ttl = 1; ttl <= MAX_TTL; ttl++){
        ip = (struct iphdr*)buf;
        icmp = (struct icmphdr*)(buf + sizeof(struct iphdr));
        ip->ihl = 5;
        ip->version = 4;
        ip->tos = 0;
        ip->tot_len = sizeof(struct iphdr) + sizeof(struct icmphdr);
        ip->id = htons(0);
        ip->frag_off = 0;
        ip->ttl = ttl;
        ip->protocol = IPPROTO_ICMP;
        ip->saddr = src.sin_addr.s_addr;
        ip->daddr = dest.sin_addr.s_addr;
        ip->check = 0;
        ip->check = checksum((unsigned short *)ip, sizeof(struct iphdr));
        // ip->check = (write checksum function)

        int num_node_changes = 0;
        int curr_node_counter = 0;
        struct in_addr curr_node;
        struct iphdr *recv_iphdr, *failed_iphdr;
        struct icmphdr *recv_icmphdr;
        int route_complete = 0;
        int node_added = 0;
        struct timespec RTT_start, RTT_end;
        int ret=0;
        for (int i=0; i< MAX_PROBES_TRIES; i++){

            icmp->type = ICMP_ECHO;
            icmp->code = 0;
            icmp->un.echo.id = ttl;
            icmp->un.echo.sequence = i;
            icmp->checksum = 0;
            icmp->checksum = checksum((unsigned short *)icmp, sizeof(struct icmphdr));
            //icmp->checksum = in_cksum((unsigned short *)icmp, sizeof(struct icmphdr));
            if(ret!=0){
                if((1000-timespec_diff(&start, &ending)) > 0)
                    usleep((1000-timespec_diff(&start, &ending))*1000);
                // usleep((1000-time_diff)*1000);
            }
            if(sendto(sock, buf, sizeof(struct iphdr) + sizeof(struct icmphdr), 0, (const struct sockaddr *)&dest, sizeof(dest)) < 0) {
                perror("Error in sending");
                close(sock);
                exit(0);
            }
            print_header(buf);
            clock_gettime(CLOCK_MONOTONIC, &start);
            ending = start;
            // struct pollfd pfd;
            // pfd.fd = sock;
            // pfd.events = POLLIN;
            // int poll_ret = poll(&pfd, 1, 1000);
            // struct timeval timeout;

            // timeout.tv_sec = 10;  // 10 seconds
            // timeout.tv_usec = 0;
            // if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            //     perror("setsockopt");
            // }
            do{
                struct pollfd fdset[1];      // add sockfd in pollfd struct
                fdset[0].fd = sock;
                fdset[0].events = POLLIN;
                // double min_latency_time = DBL_MAX, min_prev;
                // int route_success = 0, flag_prev;
                struct sockaddr_in recv_addr;
                socklen_t rlen = sizeof(recv_addr);
                ret = poll(fdset, 1, 1000 - timespec_diff(&start, &ending));
                if(ret < 0){
                    printf("Error in polling\n");
                    close(sock);
                    exit(0);
                }
                if(ret == 0){
                    break;
                }
                int recv_len = recvfrom(sock, recv_buf, 4096, 0, (struct sockaddr *)&recv_addr, &rlen);
                if(recv_len < 0){
                    printf("Error in receiving\n");
                    close(sock);
                    exit(0);
                }
                print_header(recv_buf);
                clock_gettime(CLOCK_MONOTONIC, &ending);
                // double latency = timespec_diff(&start, &ending);
                // if(latency < min_latency_time){
                //     min_latency_time = latency;
                // }
                recv_iphdr = (struct iphdr *)recv_buf;
                int hlen1 = (recv_iphdr->ihl) << 2;
                recv_icmphdr = (struct icmphdr *)(recv_buf + hlen1);
                failed_iphdr =  (struct iphdr *)(recv_buf + hlen1 + 8);
                // struct in_addr src_addr;
                // src_addr.s_addr = recv_iphdr->saddr;
                // printf("Hop_Count(%d) %s \n", ttl, inet_ntoa(src_addr));
                // int hlen2 = (failed_iphdr->ihl) << 2;
                if(recv_iphdr->protocol == IPPROTO_ICMP) {
                    // struct in_addr src_addr;
                    // struct in_addr src_addr;
                    // src_addr.s_addr = recv_iphdr->saddr;
                    // printf("Hop_Count(%d) %s \n", ttl, inet_ntoa(src_addr));
                    int route_success = 0;
                    // src_addr.s_addr = recv_iphdr->saddr;
                    if(recv_icmphdr->type == ICMP_TIME_EXCEEDED) {
                        // printf("Hop_Count(%d) %s \n", ttl, inet_ntoa(src_addr));
                        route_success = 1;
                        // if(recv_iphdr->saddr == dest.sin_addr.s_addr){
                        //     route_complete = 1;
                        // }
                        // break;
                    }
                    if(recv_iphdr->saddr == dest.sin_addr.s_addr) {
                        // printf("Hop_Count(%d) %s \n", ttl, inet_ntoa(src_addr));
                        route_success = 1;
                        route_complete = 1;
                        //done = 1;
                        // break;
                    }
                    if(route_success){
                        if(curr_node.s_addr == recv_iphdr->saddr){
                            curr_node_counter++;
                            if(curr_node_counter > 5){
                                printf("\n\nNext hop is %s\n", inet_ntoa(curr_node));
                                // printf("Route_len = %d\n", route_table.route_len);
                                route_table.route[route_table.route_len].s_addr = curr_node.s_addr;
                                // printf("Route_table = %s\n", inet_ntoa(route_table.route[route_table.route_len]));
                                route_table.valid[route_table.route_len] = 1;
                                route_table.route_len++;
                                node_added = 1;
                                break;
                            }
                        }
                        else{
                            curr_node.s_addr = recv_iphdr->saddr;
                            curr_node_counter = 1;
                            num_node_changes++;
                            if(num_node_changes > 1){
                                printf("Network unstable. Next hop is not constant\n");
                                close(sock);
                                exit(0);
                                // break;
                            }
                        }
                        break;
                    }
                }
            }while(1);
            if(node_added){
                break;
            }
            // while(1){
            // }
            // if(route_success == 1){
            //     min_prev = min_latency_time;
            //     break;
            // }
        }


        if(!node_added){
            route_table.valid[route_table.route_len] = 0;
            route_table.latency[route_table.route_len] = -1;
            route_table.bandwidth[route_table.route_len] = -1;
            route_table.route_len++;
            printf("* * * intermediate node not responding\n");
            printf("Latency and Bandwidth not available\n\n");
        }


        else{
            
            
            int poll_tim = time_diff*1000;
            int data_sizes[2] = {0, 512};
            printf("RTT calculation for node %s\n", inet_ntoa(route_table.route[route_table.route_len-1]));
            for(int i=0;i<2;i++){
                printf("\nData size = %d\n", data_sizes[i]);
                ip->daddr = route_table.route[route_table.route_len-1].s_addr;
                ip->tot_len = (sizeof(struct iphdr) + sizeof(struct icmphdr) + data_sizes[i]);
                ip->ttl = ttl+1;
                ip->check = 0;
                ip->check = checksum((unsigned short *)ip, sizeof(struct iphdr));

                struct sockaddr_in inter_node;
                inter_node.sin_family = AF_INET;
                inter_node.sin_addr.s_addr = route_table.route[route_table.route_len-1].s_addr;
                inter_node.sin_port = htons(40000);

                double min_RTT = FLT_MAX;
                int num_packet_loss = 0;
                int ret=0;
                for(int j=0;j<num_probes;j++){
                    
                    
                    
                    icmp->type = ICMP_ECHO;
                    icmp->code = 0;
                    icmp->un.echo.id = ttl+MAX_TTL;
                    icmp->un.echo.sequence = j;
                    icmp->checksum = 0;
                    icmp->checksum = checksum((unsigned short *)icmp, sizeof(struct icmphdr)+data_sizes[i]);
                    //icmp->checksum = in_cksum((unsigned short *)icmp, sizeof(struct icmphdr));
                    if(ret!=0){
                        if(((time_diff*1000)-timespec_diff(&RTT_start, &RTT_end)) > 0)
                            usleep(((time_diff*1000)-timespec_diff(&RTT_start, &RTT_end))*1000);
                        // usleep((1000-time_diff)*1000);
                    }
                    if(sendto(sock, buf, (sizeof(struct iphdr) + sizeof(struct icmphdr) + data_sizes[i]), 0, (const struct sockaddr *)&inter_node, sizeof(inter_node)) < 0) {
                        perror("Error in sending");
                        close(sock);
                        exit(0);
                    }
                    print_header(buf);
                    clock_gettime(CLOCK_MONOTONIC, &RTT_start);
                    RTT_end = RTT_start;
                    
                    do{
                        struct pollfd fdset[1];      // add sockfd in pollfd struct
                        fdset[0].fd = sock;
                        fdset[0].events = POLLIN;
                        struct sockaddr_in recv_addr;
                        socklen_t rlen = sizeof(recv_addr);
                        ret = poll(fdset, 1, (poll_tim-(timespec_diff(&RTT_start, &RTT_end))));
                        if(ret < 0){
                            printf("Error in polling\n");
                            close(sock);
                            exit(0);
                        }
                        if(ret == 0){
                            num_packet_loss++;
                            printf("Timeout, ICMP seq no=%d\n", j);
                            break;
                        }
                        int recv_len = recvfrom(sock, recv_buf, 4096, 0, (struct sockaddr *)&recv_addr, &rlen);
                        if(recv_len < 0){
                            printf("Error in receiving\n");
                            close(sock);
                            exit(0);
                        }
                        print_header(recv_buf);
                        clock_gettime(CLOCK_MONOTONIC, &RTT_end);

                        recv_iphdr = (struct iphdr *)recv_buf;
                        int hlen1 = (recv_iphdr->ihl) << 2;
                        recv_icmphdr = (struct icmphdr *)(recv_buf + hlen1);
                        if(recv_iphdr->protocol == IPPROTO_ICMP && recv_icmphdr->type == ICMP_ECHOREPLY && recv_icmphdr->un.echo.id == icmp->un.echo.id && recv_icmphdr->un.echo.sequence == icmp->un.echo.sequence){
                            double RTT = timespec_diff(&RTT_start, &RTT_end);
                            if(RTT < min_RTT){
                                min_RTT = RTT;
                            }
                            printf("%d bytes from %s: ICMP seq no=%d, RTT = %lf ms\n", recv_len, inet_ntoa(recv_addr.sin_addr), j, RTT);
                            break;
                        }
                        
                    }
                    while(1); 
                }
                
                printf("---%s pingnetinfo stats---\n", inet_ntoa(route_table.route[route_table.route_len-1]));
                printf("%d packets transmitted, %d packets received, %d%% packet loss\n", num_probes, num_probes-num_packet_loss, (num_packet_loss*100)/num_probes);
                route_table.RTT[route_table.route_len-1] = min_RTT;
                if(i==0){
                    if(ttl==1){
                        route_table.latency[route_table.route_len-1] = min_RTT/2;
                        printf("Latency for link %s -> ", inet_ntoa(*self_ip_address));
                        printf("%s = %lf ms\n", inet_ntoa(route_table.route[route_table.route_len-1]), route_table.latency[route_table.route_len-1]);
                    }
                    else if(route_table.valid[route_table.route_len-2]==0){
                        route_table.latency[route_table.route_len-1] = -1;
                        printf("Latency cannot be calculated as previous hop is not responding\n\n");
                        continue;
                    }
                    else{
                        route_table.latency[route_table.route_len-1] = (min_RTT - route_table.RTT[route_table.route_len-2])/2;
                        printf("Latency for link %s -> ", inet_ntoa(route_table.route[route_table.route_len-2]));
                        printf("%s = %lf ms\n", inet_ntoa(route_table.route[route_table.route_len-1]), route_table.latency[route_table.route_len-1]);
                    }
                    // if(route_table.latency[route_table.route_len-1] > 0){
                    // }
                }
                else{
                    //bandwidth ka function likhna hai
                    double time;
                    if(ttl == 1){
                        time = min_RTT - 2*route_table.latency[route_table.route_len-1];
                        time /= 2;
                        route_table.bandwidth[route_table.route_len-1] = (data_sizes[i]*8*1000)/(min_RTT*1024*1024);
                        printf("Bandwidth for link %s -> ", inet_ntoa(*self_ip_address));
                        printf("%s = %lf Mbps\n", inet_ntoa(route_table.route[route_table.route_len-1]), route_table.bandwidth[route_table.route_len-1]);
                    }
                    else if(route_table.valid[route_table.route_len-2]==0){
                        route_table.bandwidth[route_table.route_len-1] = -1;
                        printf("Bandwidth cannot be calculated as previous hop is not responding\n\n");
                        continue;
                    }
                    else{
                        time = min_RTT - route_table.RTT[route_table.route_len-2] - 2*route_table.latency[route_table.route_len-1];
                        time /= 2;
                        route_table.bandwidth[route_table.route_len-1] = (data_sizes[i]*8*1000)/(time*1024*1024);
                        printf("Bandwidth for link %s -> ", inet_ntoa(route_table.route[route_table.route_len-2]));
                        printf("%s = %lf Mbps\n", inet_ntoa(route_table.route[route_table.route_len-1]), route_table.bandwidth[route_table.route_len-1]);
                    }
                    // printf("Bandwidth for %s = %lf Mbps\n", (data_sizes[i]*8*1000)/(min_RTT*1024), inet_ntoa(route_table.route[route_table.route_len-1]));
                    // route_table.bandwidth[route_table.route_len-1] = (data_sizes[i]*8*1000)/(min_RTT*1024);
                }
            }

            
        }


        if(route_complete){
            printf("Route complete\n");
            break;
            // close(sock);
            // exit(0);
        }
        if(ttl == MAX_TTL){
            printf("Route not found. Too long\n");
            close(sock);
            exit(0);
        }








    }
    close(sock);
    return 0;
}

                    //458 - 550
                    // struct pollfd pfd;
                    // pfd.fd = sock;
                    // pfd.events = POLLIN;
                    // int poll_ret = poll(&pfd, 1, 1000);
                    // struct timeval timeout;

                    // timeout.tv_sec = 10;  // 10 seconds
                    // timeout.tv_usec = 0;
                    // if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
                    //     perror("setsockopt");
                    // }
                    // struct pollfd fdset[1];      // add sockfd in pollfd struct
                    // fdset[0].fd = sock;
                    // fdset[0].events = POLLIN;
                    // // double min_latency_time = DBL_MAX, min_prev;
                    // // int route_success = 0, flag_prev;
                    // struct sockaddr_in recv_addr;
                    // socklen_t rlen = sizeof(recv_addr);
                    // ret = poll(fdset, 1, time_diff*1000);
                    // if(ret < 0){
                    //     printf("Error in polling\n");
                    //     close(sock);
                    //     exit(0);
                    //     // break;
                    // }
                    // if(ret == 0){
                    //     num_packet_loss++;
                    //     printf("Timeout, ICMP seq no=%d\n", j);
                    //     // close(sock);
                    //     // exit(0);
                    //     // break;
                    //     continue;
                    // }
                    // int recv_len = recvfrom(sock, recv_buf, 4096, 0, (struct sockaddr *)&recv_addr, &rlen);
                    // if(recv_len < 0){
                    //     printf("Error in receiving\n");
                    //     close(sock);
                    //     exit(0);
                    // }
                    // print_header(recv_buf);
                    // clock_gettime(CLOCK_MONOTONIC, &RTT_end);
                    // // double latency = timespec_diff(&start, &ending);
                    // // if(latency < min_latency_time){
                    // //     min_latency_time = latency;
                    // // }
                    // recv_iphdr = (struct iphdr *)recv_buf;
                    // int hlen1 = (recv_iphdr->ihl) << 2;
                    // recv_icmphdr = (struct icmphdr *)(recv_buf + hlen1);
                    // int ret2;
                    // while(1){

                    //     if(recv_iphdr->protocol == IPPROTO_ICMP && recv_icmphdr->type == ICMP_ECHOREPLY && recv_icmphdr->un.echo.id == icmp->un.echo.id && recv_icmphdr->un.echo.sequence == icmp->un.echo.sequence){
                    //         double RTT = timespec_diff(&RTT_start, &RTT_end);
                    //         if(RTT < min_RTT){
                    //             min_RTT = RTT;
                    //         }
                    //         printf("%d bytes from %s: ICMP seq no=%d, RTT = %lf ms\n", recv_len, inet_ntoa(recv_addr.sin_addr), j, RTT);
                    //         break;
                    //     }
                    //     else{



                    //         ret = 0;
                    //         ret2 = poll(fdset, 1, poll_tim-(timespec_diff(&RTT_start, &RTT_end)));
                    //         if(ret2 < 0){
                    //             printf("Error in polling\n");
                    //             close(sock);
                    //             exit(0);
                    //             // break;
                    //         }
                    //         if(ret2 == 0){
                    //             num_packet_loss++;
                    //             printf("Timeout, ICMP seq no=%d\n", j);
                    //             break;
                    //         }
                    //         int recv_len = recvfrom(sock, recv_buf, 4096, 0, (struct sockaddr *)&recv_addr, &rlen);
                    //         if(recv_len < 0){
                    //             printf("Error in receiving\n");
                    //             close(sock);
                    //             exit(0);
                    //         }
                    //         print_header(recv_buf);
                    //         clock_gettime(CLOCK_MONOTONIC, &RTT_end);
                    //         recv_iphdr = (struct iphdr *)recv_buf;
                    //         int hlen1 = (recv_iphdr->ihl) << 2;
                    //         recv_icmphdr = (struct icmphdr *)(recv_buf + hlen1);



                    //     }
                    // }