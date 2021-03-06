/*
 * This is the main module of the CNTLM
 *
 * CNTLM is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * CNTLM is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
 * St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Copyright (c) 2007 David Kubicek
 *
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <ctype.h>
#include <pwd.h>
#include <fcntl.h>
#include <syslog.h>
#include <termios.h>
#include <fnmatch.h>

/*
 * Some helping routines like linked list manipulation substr(), memory
 * allocation, NTLM authentication routines, etc.
 */
#include "config/config.h"
#include "socket.h"
#include "utils.h"
#include "ntlm.h"
#include "swap.h"
#include "config.h"
#include "acl.h"
#include "auth.h"
#include "http.h"
#include "globals.h"
#include "pages.h"
#include "forward.h"				/* code serving via parent proxy */
#include "direct.h"				/* code serving directly without proxy */

#define STACK_SIZE	sizeof(void *)*8*1024

/*
 * Global "read-only" data initialized in main(). Comments list funcs. which use
 * them. Having these global avoids the need to pass them to each thread and
 * from there again a few times to inner calls.
 */
int debug = 1;					/* all debug printf's and possibly external modules */

struct auth_s *g_creds = NULL;			/* throughout the whole module */

int quit = 0;					/* sighandler() */
int ntlmbasic = 0;				/* forward_request() */
int serialize = 0;
int scanner_plugin = 0;
long scanner_plugin_maxsize = 0;

/*
 * List of finished threads. Each forward_request() thread adds itself to it when
 * finished. Main regularly joins and removes all tid's in there.
 */
plist_t threads_list = NULL;
pthread_mutex_t threads_mtx = PTHREAD_MUTEX_INITIALIZER;

/*
 * List of cached connections. Accessed by each thread forward_request().
 */
plist_t connection_list = NULL;
pthread_mutex_t connection_mtx = PTHREAD_MUTEX_INITIALIZER;

/*
 * List of available proxies and current proxy id for proxy_connect().
 */
int parent_count = 0;
plist_t parent_list = NULL;

/*
 * List of custom header substitutions, SOCKS5 proxy users and 
 * UserAgents for the scanner plugin.
 */
hlist_t header_list = NULL;			/* forward_request() */
hlist_t users_list = NULL;			/* socks5_thread() */
plist_t scanner_agent_list = NULL;		/* scanner_hook() */
plist_t noproxy_list = NULL;			/* proxy_thread() */

void stop() {
    syslog(LOG_INFO, "Stop request recieved.\n");
    quit ++ ;
}

/*
 * Parse proxy parameter and add it to the global list.
 */
int parent_add(char *parent, int port) {
	int len, i;
	char *proxy;
	proxy_t *aux;

	/*
	 * Check format and parse it.
	 */
	proxy = strdup(parent);
	len = strlen(proxy);
	i = strcspn(proxy, ": ");
	if (i != len) {
		proxy[i++] = 0;
		while (i < len && (proxy[i] == ' ' || proxy[i] == '\t'))
			i++;

		if (i >= len) {
			free(proxy);
			return 0;
		}

		port = atoi(proxy+i);
	}

	/*
	 * No port argument and not parsed from proxy?
	 */
	if (!port) {
		syslog(LOG_ERR, "Invalid proxy specification %s.\n", parent);
		free(proxy);
		myexit(1);
        return 0;
	}

	/*
	 * Try to resolve proxy address
	 *
	if (debug)
		syslog(LOG_INFO, "Resolving proxy %s...\n", proxy);
	if (!so_resolv(&host, proxy)) {
		syslog(LOG_ERR, "Cannot resolve proxy %s, discarding.\n", parent);
		free(proxy);
		return 0;
	}
	*/

	aux = (proxy_t *)new(sizeof(proxy_t));
	strlcpy(aux->hostname, proxy, sizeof(aux->hostname));
	aux->port = port;
	aux->resolved = 0;

	parent_list = plist_add(parent_list, ++parent_count, (char *)aux);

	free(proxy);
	return 1;
}

/*
 * Register and bind new proxy service port.
 */
void listen_add(const char *service, plist_t *list, char *spec, int gateway) {
	struct in_addr source;
	int i, p, len, port;
	char *tmp;

	len = strlen(spec);
	p = strcspn(spec, ":");
	if (p < len-1) {
		tmp = substr(spec, 0, p);
		if (!so_resolv(&source, tmp)) {
			syslog(LOG_ERR, "Cannot resolve listen address %s\n", tmp);
			myexit(1);
            return;
		}
		free(tmp);
		port = atoi(tmp = spec+p+1);
	} else {
		source.s_addr = htonl(gateway ? INADDR_ANY : INADDR_LOOPBACK);
		port = atoi(tmp = spec);
	}

	if (!port) {
		syslog(LOG_ERR, "Invalid listen port %s.\n", tmp);
		myexit(1);
        return;
	}

	i = so_listen(port, source);
	if (i > 0) {
		*list = plist_add(*list, i, NULL);
		syslog(LOG_INFO, "%s listening on %s:%d\n", service, inet_ntoa(source), port);
	}
}

/*
 * Register a new tunnel definition, bind service port.
 */
void tunnel_add(plist_t *list, char *spec, int gateway) {
	struct in_addr source;
	int i, len, count, pos, port;
	char *field[4];
	char *tmp;

	spec = strdup(spec);
	len = strlen(spec);
	field[0] = spec;
	for (count = 1, i = 0; count < 4 && i < len; ++i)
		if (spec[i] == ':') {
			spec[i] = 0;
			field[count++] = spec+i+1;
		}

	pos = 0;
	if (count == 4) {
		if (!so_resolv(&source, field[pos])) {
                        syslog(LOG_ERR, "Cannot resolve tunel bind address: %s\n", field[pos]);
                        myexit(1);
                        return;
                }
		pos++;
	} else
		source.s_addr = htonl(gateway ? INADDR_ANY : INADDR_LOOPBACK);

	if (count-pos == 3) {
		port = atoi(field[pos]);
		if (port == 0) {
			syslog(LOG_ERR, "Invalid tunnel local port: %s\n", field[pos]);
			myexit(1);
            return;
		}

		if (!strlen(field[pos+1]) || !strlen(field[pos+2])) {
			syslog(LOG_ERR, "Invalid tunnel target: %s:%s\n", field[pos+1], field[pos+2]);
			myexit(1);
            return;
		}

		tmp = new(strlen(field[pos+1]) + strlen(field[pos+2]) + 2 + 1);
		strcpy(tmp, field[pos+1]);
		strcat(tmp, ":");
		strcat(tmp, field[pos+2]);

		i = so_listen(port, source);
		if (i > 0) {
			*list = plist_add(*list, i, tmp);
			syslog(LOG_INFO, "New tunnel from %s:%d to %s\n", inet_ntoa(source), port, tmp);
		} else
			free(tmp);
	} else {
		printf("Tunnel specification incorrect ([laddress:]lport:rserver:rport).\n");
		myexit(1);
        return;
	}

	free(spec);
}

/*
 * Add no-proxy hostname/IP
 */
plist_t noproxy_add(plist_t list, char *spec) {
	char *tok, *save;

	tok = strtok_r(spec, ", ", &save);
	while ( tok != NULL ) {
		if (debug)
			printf("Adding no-proxy for: '%s'\n", tok);
		list = plist_add(list, 0, strdup(tok));
		tok = strtok_r(NULL, ", ", &save);
	}

	return list;
}

int noproxy_match(const char *addr) {
	plist_t list;

	list = noproxy_list;
	while (list) {
		if (list->aux && strlen(list->aux)
				&& fnmatch(list->aux, addr, 0) == 0) {
			if (debug)
				printf("MATCH: %s (%s)\n", addr, (char *)list->aux);
			return 1;
		} else if (debug)
			printf("   NO: %s (%s)\n", addr, (char *)list->aux);

		list = list->next;
	}

	return 0;
}

/*
 * Proxy thread - decide between direct and forward based on NoProxy
 */
void *proxy_thread(void *thread_data) {
	rr_data_t request, ret;
	int keep_alive;				/* Proxy-Connection */

	int cd = ((struct thread_arg_s *)thread_data)->fd;

	do {
		ret = NULL;
		keep_alive = 0;

		if (debug) {
			printf("\n******* Round 1 C: %d *******\n", cd);
			printf("Reading headers (%d)...\n", cd);
		}

		request = new_rr_data();
		if (!headers_recv(cd, request)) {
			free_rr_data(request);
			break;
		}

		do {
			/*
			 * Are we being returned a request by forward_request or direct_request?
			 */
			if (ret) {
				free_rr_data(request);
				request = ret;
			}

			keep_alive = hlist_subcmp(request->headers, "Proxy-Connection", "keep-alive");

			if (noproxy_match(request->hostname))
				ret = direct_request(thread_data, request);
			else
				ret = forward_request(thread_data, request);

			if (debug)
				printf("proxy_thread: request rc = %p\n", (void *)ret);
		} while (ret != NULL && ret != (void *)-1);

		free_rr_data(request);
	/*
	 * If client asked for proxy keep-alive, loop unless the last server response
	 * requested (Proxy-)Connection: close.
	 */
	} while (keep_alive && ret != (void *)-1 && !serialize);

	/*
	 * Add ourselves to the "threads to join" list.
	 */
	if (!serialize) {
		pthread_mutex_lock(&threads_mtx);
		threads_list = plist_add(threads_list, (unsigned long)pthread_self(), NULL);
		pthread_mutex_unlock(&threads_mtx);
	}

	free(thread_data);
	close(cd);

	return NULL;
}

/*
 * Tunnel/port forward thread - this method is obviously better solution than using extra
 * tools like "corkscrew" which after all require us for authentication and tunneling
 * their HTTP CONNECT in the first place.
 */
void *tunnel_thread(void *thread_data) {
	char *hostname, *pos;
	char *thost = ((struct thread_arg_s *)thread_data)->target;

	hostname = strdup(thost);
	if ((pos = strchr(hostname, ':')) != NULL)
		*pos = 0;

	if (noproxy_match(hostname))
		direct_tunnel(thread_data);
	else
		forward_tunnel(thread_data);

	free(hostname);
	free(thread_data);

	/*
	 * Add ourself to the "threads to join" list.
	 */
	pthread_mutex_lock(&threads_mtx);
	threads_list = plist_add(threads_list, (unsigned long)pthread_self(), NULL);
	pthread_mutex_unlock(&threads_mtx);

	return NULL;
}

/*
 * SOCKS5 thread
 */
void *socks5_thread(void *thread_data) {
	char *tmp, *thost, *tport, *uname, *upass;
	unsigned short port;
	int ver, r, c, i, w;

	struct auth_s *tcreds = NULL;
	unsigned char *bs = NULL, *auths = NULL, *addr = NULL;
	int found = -1;
	int sd = -1;
	int open = !hlist_count(users_list);

	int cd = ((struct thread_arg_s *)thread_data)->fd;
	struct sockaddr_in caddr = ((struct thread_arg_s *)thread_data)->addr;
	free(thread_data);

	/*
	 * Check client's version, possibly fuck'em
	 */
	bs = (unsigned char *)new(10);
	thost = new(MINIBUF_SIZE);
	tport = new(MINIBUF_SIZE);
	r = read(cd, bs, 2);
	if (r != 2 || bs[0] != 5)
		goto bailout;

	/*
	 * Read offered auth schemes
	 */
	c = bs[1];
	auths = (unsigned char *)new(c+1);
	r = read(cd, auths, c);
	if (r != c)
		goto bailout;

	/*
	 * Are we wide open and client is OK with no auth?
	 */
	if (open) {
		for (i = 0; i < c && (auths[i] || (found = 0)); ++i);
	}

	/*
	 * If not, accept plain auth if offered
	 */
	if (found < 0) {
		for (i = 0; i < c && (auths[i] != 2 || !(found = 2)); ++i);
	}

	/*
	 * If not open and no auth offered or open and auth requested, fuck'em
	 * and complete the handshake
	 */
	if (found < 0) {
		bs[0] = 5;
		bs[1] = 0xFF;
		w = write(cd, bs, 2);
		goto bailout;
	} else {
		bs[0] = 5;
		bs[1] = found;
		w = write(cd, bs, 2);
	}

	/*
	 * Plain auth negotiated?
	 */
	if (found != 0) {
		/*
		 * Check ver and read username len
		 */
		r = read(cd, bs, 2);
		if (r != 2) {
			bs[0] = 1;
			bs[1] = 0xFF;		/* Unsuccessful (not supported) */
			w = write(cd, bs, 2);
			goto bailout;
		}
		c = bs[1];

		/*
		 * Read username and pass len
		 */
		uname = new(c+1);
		r = read(cd, uname, c+1);
		if (r != c+1) {
			free(uname);
			goto bailout;
		}
		i = uname[c];
		uname[c] = 0;
		c = i;

		/*
		 * Read pass
		 */
		upass = new(c+1);
		r = read(cd, upass, c);
		if (r != c) {
			free(upass);
			free(uname);
			goto bailout;
		}
		upass[c] = 0;

		/*
		 * Check credentials against the list
		 */
		tmp = hlist_get(users_list, uname);
		if (!hlist_count(users_list) || (tmp && !strcmp(tmp, upass))) {
			bs[0] = 1;
			bs[1] = 0;		/* Success */
		} else {
			bs[0] = 1;
			bs[1] = 0xFF;		/* Failed */
		}

		/*
		 * Send response
		 */
		w = write(cd, bs, 2);
		free(upass);
		free(uname);

		/*
		 * Fuck'em if auth failed
		 */
		if (bs[1])
			goto bailout;
	}

	/*
	 * Read request type
	 */
	r = read(cd, bs, 4);
	if (r != 4)
		goto bailout;

	/*
	 * Is it connect for supported address type (IPv4 or DNS)? If not, fuck'em
	 */
	if (bs[1] != 1 || (bs[3] != 1 && bs[3] != 3)) {
		bs[0] = 5;
		bs[1] = 2;			/* Not allowed */
		bs[2] = 0;
		bs[3] = 1;			/* Dummy IPv4 */
		memset(bs+4, 0, 6);
		w = write(cd, bs, 10);
		goto bailout;
	}

	/*
	 * Ok, it's connect to a domain or IP
	 * Let's read dest address
	 */
	if (bs[3] == 1) {
		ver = 1;			/* IPv4, we know the length */
		c = 4;
	} else if (bs[3] == 3) {
		ver = 2;			/* FQDN, get string length */
		r = read(cd, &c, 1);
		if (r != 1)
			goto bailout;
	} else
		goto bailout;

	addr = (unsigned char *)new(c+10 + 1);
	r = read(cd, addr, c);
	if (r != c)
		goto bailout;
	addr[c] = 0;

	/*
	 * Convert the address to character string
	 */
	if (ver == 1) {
		sprintf(thost, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);	/* It's in network byte order */
	} else {
		strlcpy(thost, (char *)addr, MINIBUF_SIZE);
	}

	/*
	 * Read port number and convert to host byte order int
	 */
	r = read(cd, &port, 2);
	if (r != 2)
		goto bailout;

	i = 0;
	if (noproxy_match(thost)) {
		sd = host_connect(thost, ntohs(port));
		i = (sd >= 0);
	} else {
		sprintf(tport, "%d", ntohs(port));
		strlcat(thost, ":", MINIBUF_SIZE);
		strlcat(thost, tport, MINIBUF_SIZE);

		tcreds = new_auth();
		sd = proxy_connect(tcreds);
		if (sd >= 0)
			i = prepare_http_connect(sd, tcreds, thost);
	}

	/*
	 * Direct or proxy connect?
	 */
	if (!i) {
		/*
		 * Connect/tunnel failed, report
		 */
		bs[0] = 5;
		bs[1] = 1;			/* General failure */
		bs[2] = 0;
		bs[3] = 1;			/* Dummy IPv4 */
		memset(bs+4, 0, 6);
		w = write(cd, bs, 10);
		goto bailout;
	} else {
		/*
		 * All right
		 */
		bs[0] = 5;
		bs[1] = 0;			/* Success */
		bs[2] = 0;
		bs[3] = 1;			/* Dummy IPv4 */
		memset(bs+4, 0, 6);
		w = write(cd, bs, 10);
	}

	syslog(LOG_DEBUG, "%s SOCKS %s", inet_ntoa(caddr.sin_addr), thost);

	/*
	 * Let's give them bi-directional connection they asked for
	 */
	tunnel(cd, sd);

bailout:
	if (addr)
		free(addr);
	if (auths)
		free(auths);
	if (thost)
		free(thost);
	if (tport)
		free(tport);
	if (bs)
		free(bs);
	if (tcreds)
		free(tcreds);
	if (sd)
		close(sd);
	close(cd);

	return NULL;
}

int start(int argc, char **argv, char **errorMsg) {
    *errorMsg = NULL;
    int returnCode = 0;

	char *tmp, *head;
	char *cpassword, *cpassntlm2, *cpassnt, *cpasslm;
	char *cuser, *cdomain, *cworkstation, *cuid, *cpidfile, *cauth;
	struct passwd *pw;
	struct termios termold, termnew;
	pthread_attr_t pattr;
	pthread_t pthr;
	hlist_t list;
	int i, w;

	int cd = 0;
	int help = 0;
	int nuid = 0;
	int ngid = 0;
	int gateway = 0;
	int tc = 0;
	int tj = 0;
	int interactivepwd = 0;
	int interactivehash = 0;
	int tracefile = 0;
	int cflags = 0;
	int asdaemon = 1;
	plist_t tunneld_list = NULL;
	plist_t proxyd_list = NULL;
	plist_t socksd_list = NULL;
	plist_t rules = NULL;
	config_t cf = NULL;
	char *magic_detect = NULL;
    quit = 0;

	g_creds = new_auth();
	cuser = new(MINIBUF_SIZE);
	cdomain = new(MINIBUF_SIZE);
	cpassword = new(MINIBUF_SIZE);
	cpassntlm2 = new(MINIBUF_SIZE);
	cpassnt = new(MINIBUF_SIZE);
	cpasslm = new(MINIBUF_SIZE);
	cworkstation = new(MINIBUF_SIZE);
	cpidfile = new(MINIBUF_SIZE);
	cuid = new(MINIBUF_SIZE);
	cauth = new(MINIBUF_SIZE);

    openlog("cntlm", LOG_CONS, LOG_DAEMON);

#if config_endian == 0
	syslog(LOG_INFO, "Starting cntlm version " VERSION " for BIG endian\n");
#else
	syslog(LOG_INFO, "Starting cntlm version " VERSION " for LITTLE endian\n");
#endif

    optind = 1;
	while ((i = getopt(argc, argv, ":-:a:c:d:fghIl:p:r:su:vw:A:BD:F:G:HL:M:N:O:P:R:S:T:U:")) != -1) {
		switch (i) {
			case 'A':
			case 'D':
                if (!acl_add(&rules, optarg, (i == 'A' ? ACL_ALLOW : ACL_DENY))) {
					myexit(1);
                    goto bailout;
                }
				break;
			case 'a':
				strlcpy(cauth, optarg, MINIBUF_SIZE);
				break;
			case 'B':
				ntlmbasic = 1;
				break;
			case 'c':
				if (!(cf = config_open(optarg))) {
					syslog(LOG_ERR, "Cannot access specified config file: %s\n", optarg);
					myexit(1);
                    goto bailout;
				}
				break;
			case 'd':
				strlcpy(cdomain, optarg, MINIBUF_SIZE);
				break;
			case 'F':
				cflags = swap32(strtoul(optarg, &tmp, 0));
				break;
			case 'f':
				asdaemon = 0;
				break;
			case 'G':
				if (strlen(optarg)) {
					scanner_plugin = 1;
					if (!scanner_plugin_maxsize)
						scanner_plugin_maxsize = 1;
					i = strlen(optarg) + 3;
					tmp = new(i);
					snprintf(tmp, i, "*%s*", optarg);
					scanner_agent_list = plist_add(scanner_agent_list, 0, tmp);
				}
				break;
			case 'g':
				gateway = 1;
				break;
			case 'H':
				interactivehash = 1;
				break;
			case 'I':
				interactivepwd = 1;
				break;
			case 'L':
				/*
				 * Parse and validate the argument.
				 * Create a listening socket for tunneling.
				 */
				tunnel_add(&tunneld_list, optarg, gateway);
				break;
			case 'l':
				/*
				 * Create a listening socket for proxy function.
				 */
				listen_add("Proxy", &proxyd_list, optarg, gateway);
				break;
			case 'M':
				magic_detect = strdup(optarg);
				break;
			case 'N':
				noproxy_list = noproxy_add(noproxy_list, tmp=strdup(optarg));
				free(tmp);
				break;
			case 'O':
				listen_add("SOCKS5 proxy", &socksd_list, optarg, gateway);
				break;
			case 'P':
				strlcpy(cpidfile, optarg, MINIBUF_SIZE);
				break;
			case 'p':
				/*
				 * Overwrite the password parameter with '*'s to make it
				 * invisible in "ps", /proc, etc.
				 */
				strlcpy(cpassword, optarg, MINIBUF_SIZE);
                /*for (i = strlen(optarg)-1; i >= 0; --i)
                    optarg[i] = '*';*/
				break;
			case 'R':
				tmp = strdup(optarg);
				head = strchr(tmp, ':');
				if (!head) {
					fprintf(stderr, "Invalid username:password format for -R: %s\n", tmp);
				} else {
					head[0] = 0;
					users_list = hlist_add(users_list, tmp, head+1,
						HLIST_ALLOC, HLIST_ALLOC);
				}
				break;
			case 'r':
				if (is_http_header(optarg))
					header_list = hlist_add(header_list,
						get_http_header_name(optarg),
						get_http_header_value(optarg),
						HLIST_NOALLOC, HLIST_NOALLOC);
				break;
			case 'S':
				scanner_plugin = 1;
				scanner_plugin_maxsize = atol(optarg);
				break;
			case 's':
				/*
				 * Do not use threads - for debugging purposes only
				 */
				serialize = 1;
				break;
			case 'T':
				tracefile = open(optarg, O_CREAT | O_TRUNC | O_WRONLY, 0600);
				if (tracefile < 0) {
					fprintf(stderr, "Cannot create trace file.\n");
					myexit(1);
                    goto bailout;
				} else {
					printf("Redirecting all output to %s\n", optarg);
					dup2(tracefile, 1);
					dup2(tracefile, 2);
					printf("Cntlm debug trace, version " VERSION);
#ifdef __CYGWIN__
					printf(" windows/cygwin port");
#endif
					printf(".\nCommand line: ");
					for (i = 0; i < argc; ++i)
						printf("%s ", argv[i]);
					printf("\n");
				}
				break;
			case 'U':
				strlcpy(cuid, optarg, MINIBUF_SIZE);
				break;
			case 'u':
				i = strcspn(optarg, "@");
				if (i != strlen(optarg)) {
					strlcpy(cuser, optarg, MIN(MINIBUF_SIZE, i+1));
					strlcpy(cdomain, optarg+i+1, MINIBUF_SIZE);
				} else {
					strlcpy(cuser, optarg, MINIBUF_SIZE);
				}
				break;
			case 'v':
				debug = 1;
				asdaemon = 0;
				openlog("cntlm", LOG_CONS | LOG_PERROR, LOG_DAEMON);
				break;
			case 'w':
				strlcpy(cworkstation, optarg, MINIBUF_SIZE);
				break;
			case 'h':
			default:
				help = 1;
		}
	}

	/*
	 * More arguments on the command-line? Must be proxies.
	 */
	i = optind;
	while (i < argc) {
		tmp = strchr(argv[i], ':');
		parent_add(argv[i], !tmp && i+1 < argc ? atoi(argv[i+1]) : 0);
		i += (!tmp ? 2 : 1);
	}


    if (!interactivehash && !parent_list) {
        *errorMsg = "Parent proxy address missing.\n";
        returnCode = 1;
        croak(*errorMsg, interactivepwd || magic_detect);
        goto bailout;

    }

    if (!interactivehash && !magic_detect && !proxyd_list) {
        *errorMsg = "No proxy service ports were successfully opened.\n";
        returnCode = 1;
        croak(*errorMsg, interactivepwd);
        goto bailout;
    }
	/*
	 * Set default value for the workstation. Hostname if possible.
	 */
	if (!strlen(cworkstation)) {
#if config_gethostname == 1
		gethostname(cworkstation, MINIBUF_SIZE);
#endif
		if (!strlen(cworkstation))
			strlcpy(cworkstation, "cntlm", MINIBUF_SIZE);

		syslog(LOG_INFO, "Workstation name used: %s\n", cworkstation);
	}

	/*
	 * Parse selected NTLM hash combination.
	 */
	if (strlen(cauth)) {
		if (!strcasecmp("ntlm", cauth)) {
			g_creds->hashnt = 1;
			g_creds->hashlm = 1;
			g_creds->hashntlm2 = 0;
		} else if (!strcasecmp("nt", cauth)) {
			g_creds->hashnt = 1;
			g_creds->hashlm = 0;
			g_creds->hashntlm2 = 0;
		} else if (!strcasecmp("lm", cauth)) {
			g_creds->hashnt = 0;
			g_creds->hashlm = 1;
			g_creds->hashntlm2 = 0;
		} else if (!strcasecmp("ntlmv2", cauth)) {
			g_creds->hashnt = 0;
			g_creds->hashlm = 0;
			g_creds->hashntlm2 = 1;
		} else if (!strcasecmp("ntlm2sr", cauth)) {
			g_creds->hashnt = 2;
			g_creds->hashlm = 0;
			g_creds->hashntlm2 = 0;
		} else {
			syslog(LOG_ERR, "Unknown NTLM auth combination.\n");
			myexit(1);
            goto bailout;
		}
	}

	if (socksd_list && !users_list)
		syslog(LOG_WARNING, "SOCKS5 proxy will NOT require any authentication\n");

	if (!magic_detect)
		syslog(LOG_INFO, "Using following NTLM hashes: NTLMv2(%d) NT(%d) LM(%d)\n",
			g_creds->hashntlm2, g_creds->hashnt, g_creds->hashlm);

	if (cflags) {
		syslog(LOG_INFO, "Using manual NTLM flags: 0x%X\n", swap32(cflags));
		g_creds->flags = cflags;
	}

	/*
	 * Last chance to get password from the user
	 */
	if (interactivehash || magic_detect || (interactivepwd && !ntlmbasic)) {
		printf("Password: ");
		tcgetattr(0, &termold);
		termnew = termold;
		termnew.c_lflag &= ~(ISIG | ECHO);
		tcsetattr(0, TCSADRAIN, &termnew);
		tmp = fgets(cpassword, MINIBUF_SIZE, stdin);
		tcsetattr(0, TCSADRAIN, &termold);
		i = strlen(cpassword) - 1;
		if (cpassword[i] == '\n') {
			cpassword[i] = 0;
			if (cpassword[i - 1] == '\r')
				cpassword[i - 1] = 0;
		}
		printf("\n");
	}

	/*
	 * Convert optional PassNT, PassLM and PassNTLMv2 strings to hashes
	 * unless plaintext pass was used, which has higher priority.
	 *
	 * If plain password is present, calculate its NT and LM hashes 
	 * and remove it from the memory.
	 */
	if (!strlen(cpassword)) {
		if (strlen(cpassntlm2)) {
			tmp = scanmem(cpassntlm2, 8);
			if (!tmp) {
				syslog(LOG_ERR, "Invalid PassNTLMv2 hash, terminating\n");
                return 1;
			}
			auth_memcpy(g_creds, passntlm2, tmp, 16);
			free(tmp);
		}
		if (strlen(cpassnt)) {
			tmp = scanmem(cpassnt, 8);
			if (!tmp) {
				syslog(LOG_ERR, "Invalid PassNT hash, terminating\n");
                return 1;
			}
			auth_memcpy(g_creds, passnt, tmp, 16);
			free(tmp);
		}
		if (strlen(cpasslm)) {
			tmp = scanmem(cpasslm, 8);
			if (!tmp) {
				syslog(LOG_ERR, "Invalid PassLM hash, terminating\n");
                return 1;
			}
			auth_memcpy(g_creds, passlm, tmp, 16);
			free(tmp);
		}
	} else {
		if (g_creds->hashnt || magic_detect || interactivehash) {
			tmp = ntlm_hash_nt_password(cpassword);
			auth_memcpy(g_creds, passnt, tmp, 21);
			free(tmp);
		} if (g_creds->hashlm || magic_detect || interactivehash) {
			tmp = ntlm_hash_lm_password(cpassword);
			auth_memcpy(g_creds, passlm, tmp, 21);
			free(tmp);
		} if (g_creds->hashntlm2 || magic_detect || interactivehash) {
			tmp = ntlm2_hash_password(cuser, cdomain, cpassword);
			auth_memcpy(g_creds, passntlm2, tmp, 16);
			free(tmp);
		}
		memset(cpassword, 0, strlen(cpassword));
	}

	auth_strcpy(g_creds, user, cuser);
	auth_strcpy(g_creds, domain, cdomain);
	auth_strcpy(g_creds, workstation, cworkstation);

	free(cuser);
	free(cdomain);
	free(cworkstation);
	free(cpassword);
	free(cpassntlm2);
	free(cpassnt);
	free(cpasslm);
	free(cauth);

	/*
	 * Try known NTLM auth combinations and print which ones work.
	 * User can pick the best (most secure) one as his config.
	 */
	if (magic_detect) {
		magic_auth_detect(magic_detect);
		goto bailout;
	}

	if (interactivehash) {
		if (g_creds->passlm) {
			tmp = printmem(g_creds->passlm, 16, 8);
			printf("PassLM          %s\n", tmp);
			free(tmp);
		}

		if (g_creds->passnt) {
			tmp = printmem(g_creds->passnt, 16, 8);
			printf("PassNT          %s\n", tmp);
			free(tmp);
		}

		if (g_creds->passntlm2) {
			tmp = printmem(g_creds->passntlm2, 16, 8);
			printf("PassNTLMv2      %s    # Only for user '%s', domain '%s'\n",
				tmp, g_creds->user, g_creds->domain);
			free(tmp);
		}
		goto bailout;
	}

	/*
	 * If we're going to need a password, check we really have it.
	 */
	if (!ntlmbasic && (
			(g_creds->hashnt && !g_creds->passnt)
		     || (g_creds->hashlm && !g_creds->passlm)
		     || (g_creds->hashntlm2 && !g_creds->passntlm2))) {
		syslog(LOG_ERR, "Parent proxy account password (or required hashes) missing.\n");
		myexit(1);
        goto bailout;
	}

	/*
	 * Ok, we are ready to rock. If daemon mode was requested,
	 * fork and die. The child will not be group leader anymore
	 * and can thus create a new session for itself and detach
	 * from the controlling terminal.
	 */
	if (asdaemon) {
		if (debug)
			printf("Forking into background as requested.\n");

		i = fork();
		if (i == -1) {
			perror("Fork into background failed");		/* fork failed */
			myexit(1);
            goto bailout;
        } else if (i) {
			myexit(0);					/* parent */
            goto bailout;
        }
		setsid();
		umask(0);
		w = chdir("/");
		i = open("/dev/null", O_RDWR);
		if (i >= 0) {
			dup2(i, 0);
			dup2(i, 1);
			dup2(i, 2);
			if (i > 2)
				close(i);
		}
	}

	/*
	 * Reinit syslog logging to include our PID, after forking
	 * it is going to be OK
	 */
	if (asdaemon) {
		openlog("cntlm", LOG_CONS | LOG_PID, LOG_DAEMON);
		syslog(LOG_INFO, "Daemon ready");
	} else {
		openlog("cntlm", LOG_CONS | LOG_PID | LOG_PERROR, LOG_DAEMON);
		syslog(LOG_INFO, "Cntlm ready, staying in the foreground");
	}

	/*
	 * Check and change UID.
	 */
	if (strlen(cuid)) {
		if (getuid() && geteuid()) {
			syslog(LOG_WARNING, "No root privileges; keeping identity %d:%d\n", getuid(), getgid());
		} else {
			if (isdigit(cuid[0])) {
				nuid = atoi(cuid);
				ngid = nuid;
				if (nuid <= 0) {
					syslog(LOG_ERR, "Numerical uid parameter invalid\n");
					myexit(1);
                    goto bailout;
				}
			} else {
				pw = getpwnam(cuid);
				if (!pw || !pw->pw_uid) {
					syslog(LOG_ERR, "Username %s in -U is invalid\n", cuid);
					myexit(1);
                    goto bailout;
				}
				nuid = pw->pw_uid;
				ngid = pw->pw_gid;
			}
			setgid(ngid);
			i = setuid(nuid);
			syslog(LOG_INFO, "Changing uid:gid to %d:%d - %s\n", nuid, ngid, strerror(errno));
			if (i) {
				syslog(LOG_ERR, "Terminating\n");
				myexit(1);
                goto bailout;
			}
		}
	}

	/*
	 * PID file requested? Try to create one (it must not exist).
	 * If we fail, exit with error.
	 */
	if (strlen(cpidfile)) {
		umask(0);
		cd = open(cpidfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (cd < 0) {
			syslog(LOG_ERR, "Error creating a new PID file\n");
			myexit(1);
		}

		tmp = new(50);
		snprintf(tmp, 50, "%d\n", getpid());
		w = write(cd, tmp, strlen(tmp));
		free(tmp);
		close(cd);
	}

	/*
	 * Change the handler for signals recognized as clean shutdown.
	 * When the handler is called (termination request), it signals
	 * this news by adding 1 to the global quit variable.
	 */
//	signal(SIGPIPE, SIG_IGN);
//	signal(SIGINT, &sighandler);
//	signal(SIGTERM, &sighandler);
//	signal(SIGHUP, &sighandler);

	/*
	 * Initialize the random number generator
	 */
	srandom(time(NULL));

	/*
	 * This loop iterates over every connection request on any of
	 * the listening ports. We keep the number of created threads.
	 *
	 * We also check the "finished threads" list, threads_list, here and
	 * free the memory of all inactive threads. Then, we update the
	 * number of finished threads.
	 *
	 * The loop ends, when we were "killed" and all threads created
	 * are finished, OR if we were killed more than once. This way,
	 * we have a "clean" shutdown (wait for all connections to finish
	 * after the first kill) and a "forced" one (user insists and
	 * killed us twice).
	 */
	while (quit == 0 || (tc != tj && quit < 2)) {
		struct thread_arg_s *data;
		struct sockaddr_in caddr;
		struct timeval tv;
		socklen_t clen;
		fd_set set;
		plist_t t;
		int tid = 0;

		FD_ZERO(&set);

		/*
		 * Watch for proxy ports.
		 */
		t = proxyd_list;
		while (t) {
			FD_SET(t->key, &set);
			t = t->next;
		}

		/*
		 * Watch for SOCKS5 ports.
		 */
		t = socksd_list;
		while (t) {
			FD_SET(t->key, &set);
			t = t->next;
		}

		/*
		 * Watch for tunneled ports.
		 */
		t = tunneld_list;
		while (t) {
			FD_SET(t->key, &set);
			t = t->next;
		}

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		/*
		 * Wait here for data (connection request) on any of the listening 
		 * sockets. When ready, establish the connection. For the main
		 * port, a new proxy_thread() thread is spawned to service the HTTP
		 * request. For tunneled ports, tunnel_thread() thread is created
		 * and for SOCKS port, socks5_thread() is created.
		 *
		 * All threads are defined in forward.c, except for local proxy_thread()
		 * which routes the request as forwarded or direct, depending on the
		 * URL host name and NoProxy settings.
		 */
		cd = select(FD_SETSIZE, &set, NULL, NULL, &tv);
		if (cd > 0) {
			for (i = 0; i < FD_SETSIZE; ++i) {
				if (!FD_ISSET(i, &set))
					continue;

				clen = sizeof(caddr);
				cd = accept(i, (struct sockaddr *)&caddr, (socklen_t *)&clen);

				if (cd < 0) {
					syslog(LOG_ERR, "Serious error during accept: %s\n", strerror(errno));
					continue;
				}

				/*
				 * Check main access control list.
				 */
				if (acl_check(rules, caddr.sin_addr) != ACL_ALLOW) {
					syslog(LOG_WARNING, "Connection denied for %s:%d\n",
						inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));
					tmp = gen_denied_page(inet_ntoa(caddr.sin_addr));
					w = write(cd, tmp, strlen(tmp));
					free(tmp);
					close(cd);
					continue;
				}

				/*
				 * Log peer IP if it's not localhost
				 *
				 * if (debug || (gateway && caddr.sin_addr.s_addr != htonl(INADDR_LOOPBACK)))
				 * 	syslog(LOG_INFO, "Connection accepted from %s:%d\n",
				 * 	inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));
				 */

				pthread_attr_init(&pattr);
				pthread_attr_setstacksize(&pattr, STACK_SIZE);
#ifndef __CYGWIN__
				pthread_attr_setguardsize(&pattr, 256);
#endif

				if (plist_in(proxyd_list, i)) {
					data = (struct thread_arg_s *)new(sizeof(struct thread_arg_s));
					data->fd = cd;
					data->addr = caddr;
					if (!serialize)
						tid = pthread_create(&pthr, &pattr, proxy_thread, (void *)data);
					else
						proxy_thread((void *)data);
				} else if (plist_in(socksd_list, i)) {
					data = (struct thread_arg_s *)new(sizeof(struct thread_arg_s));
					data->fd = cd;
					data->addr = caddr;
					tid = pthread_create(&pthr, &pattr, socks5_thread, (void *)data);
				} else {
					data = (struct thread_arg_s *)new(sizeof(struct thread_arg_s));
					data->fd = cd;
					data->addr = caddr;
					data->target = plist_get(tunneld_list, i);
					tid = pthread_create(&pthr, &pattr, tunnel_thread, (void *)data);
				}

				pthread_attr_destroy(&pattr);

				if (tid)
					syslog(LOG_ERR, "Serious error during pthread_create: %d\n", tid);
				else
					tc++;
			}
		} else if (cd < 0 && !quit)
			syslog(LOG_ERR, "Serious error during select: %s\n", strerror(errno));

		if (threads_list) {
			pthread_mutex_lock(&threads_mtx);
			t = threads_list;
			while (t) {
				plist_t tmp = t->next;
				tid = pthread_join((pthread_t)t->key, (void *)&i);

				if (!tid) {
					tj++;
					if (debug)
						printf("Joining thread %lu; rc: %d\n", t->key, i);
				} else
					syslog(LOG_ERR, "Serious error during pthread_join: %d\n", tid);

				free(t);
				t = tmp;
			}
			threads_list = NULL;
			pthread_mutex_unlock(&threads_mtx);
		}
	}

bailout:
	if (strlen(cpidfile))
		unlink(cpidfile);

	syslog(LOG_INFO, "Terminating with %d active threads\n", tc - tj);
	pthread_mutex_lock(&connection_mtx);
    connection_list = plist_free(connection_list);
	pthread_mutex_unlock(&connection_mtx);

	hlist_free(header_list);
    scanner_agent_list = plist_free(scanner_agent_list);
    noproxy_list = plist_free(noproxy_list);
    plist_close_all(tunneld_list);
    tunneld_list = plist_free(tunneld_list);
    plist_close_all(proxyd_list);
    proxyd_list - plist_free(proxyd_list);
    plist_close_all(socksd_list);
    socksd_list = plist_free(socksd_list);
    rules = plist_free(rules);

	free(cuid);
	free(cpidfile);
	free(magic_detect);
	free(g_creds);

    parent_list = plist_free(parent_list);
    parent_count = 0;

    quit = 0;
    return returnCode;
}
