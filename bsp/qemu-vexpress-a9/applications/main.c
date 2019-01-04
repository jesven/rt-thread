#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    printf("hello rt-thread\n");
    printf("utest_run kernel.smp.smp_signal\n");

    return 0;
}


#include <rtthread.h>
extern void dns_setserver(unsigned char numdns, const unsigned int *dnsserver);

static int set_dns(int argc, char** argv)
{
    unsigned int dns;

    if (argc > 1)
        dns = atoi(argv[1]);
    else
        dns = 0x08080808;

    dns_setserver(0, &dns);
    dns_setserver(1, &dns);

    return 0;
}
MSH_CMD_EXPORT(set_dns, set dns server);
