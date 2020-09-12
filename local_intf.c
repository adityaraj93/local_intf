#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <sys/types.h>
#include <ifaddrs.h>
#include <netdb.h>

#define OPLIST(UC, arg, val, comment) \
	__OPL(UC##V4,	arg "4",		0x00000001, comment "print only IPv4, can't be used with \"6\"") \
	__OPL(UC##V6,	arg "6",		0x00000002, comment "print only IPv6, can't be used with \"4\"") \
	__OPL(UC##IP,	arg "ip",		0x00000004, comment "print only IP address") \
	__OPL(UC##HELP,	arg "help",		0x00000008, comment "print usage") \
	/* end-of-list */

#define IPV_OPS		(OP_V4 | OP_V6)
#define IPV_OP_SET()	(options & IPV_OPS)

#define __OPL(UC, arg, val, comment) UC = val,
enum option_val {
	OPLIST(OP_,,,)
};
#undef __OPL
static int options;

static void usage(char *progname) {
	printf("Usage:\n");
	printf("\t%s [OPTIONS]\n", progname);
	printf("OPTIONS:\n");
#define __OPL(UC, arg, val, comment)	printf("%s\t\t%s\n", arg, comment);
	OPLIST(,,,)
#undef __OPL
}

static int check_one(const char *op_in, const char *op, int flag) {
	if (strcmp(op_in, op) == 0) {
		options |= flag;
		return 0;
	}
	return 1;
}

static int parse_op(const char **argv, int argc) {
	for (int i = 1; i < argc; i++) {
		do {
#define __OPL(_UC, _arg, _val, _comment) if (check_one(argv[i], _arg, _UC) == 0) break;
			OPLIST(OP_,,,)
#undef __OPL
			printf("Invalid argument: %s\n", argv[i]);
			return !0;
		} while (0);
	}
	if (options & OP_HELP) {
		// help will be printed by the caller
		return !0;
	}
	if ((options & IPV_OPS) == IPV_OPS) {
		// user can't set both simultaneously
		printf("Can't use 4 & 6 together\n");
		return !0;
	}
	if (!IPV_OP_SET())
		options |= IPV_OPS;
	return 0;
}

int main(int argc, char **argv) {
		struct ifaddrs *intf, *ifa;
		char hostbuf[255], servbuf[255];
		int ret = 0, hdr_done = 0;

		if (0 != parse_op((const char **)argv, argc)) {
			usage(argv[0]);
			exit(EXIT_FAILURE);
		}
		if (0 != getifaddrs(&intf)) {
			perror("getifaddrs");
			exit(EXIT_FAILURE);
		}
		assert(IPV_OP_SET());
		for (ifa = intf; ifa != NULL; ifa = ifa->ifa_next) {
			if (ifa->ifa_addr == NULL)
				continue;
			switch (ifa->ifa_addr->sa_family) {
				case AF_INET:
					if (options & OP_V4) {
						ret = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
								  hostbuf, sizeof(hostbuf),
								  servbuf, sizeof(servbuf),
								  NI_NUMERICHOST);
					}
					else
						continue;
					break;
				case AF_INET6:
					if (options & OP_V6) {
						ret = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in6),
								  hostbuf, sizeof(hostbuf),
								  servbuf, sizeof(servbuf),
								  NI_NUMERICHOST);
					}
					else
						continue;
					break;
				default:
					continue;
			}
			if (ret != 0) {
				printf("getnameinfo() failed: %s\n", gai_strerror(ret));
				return EXIT_FAILURE;
			}
			else {
				if (options & OP_IP) { // just print the IP addresses
					printf("%s", hostbuf);
				}
				else {
					if (!hdr_done) {
						printf("%-15s\t%-50s\t%s\n", "Name", "Address", "Service");
						hdr_done = !0;
					}
					printf("%-15s\t%-50s\t%s",
						ifa->ifa_name,
						hostbuf,
						servbuf);
				}
			}
			printf("\n");
		}
		freeifaddrs(intf);
		return EXIT_SUCCESS;
}
