/* NAStySAN NBD adapter
 *
 * This creates a NAStySAN block device using /dev/ndb
 * The device is meant to be fault tolerant and reliable.
 *
 * We use the new netlink driver which needs Kernel 3.17 and above.
 * WHY IS THERE NO DOCUMENTATION ON THIS SUBJECT WHATSOEVER?
 *
 */
#include "OOPS.h"

#include <linux/nbd-netlink.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

#include "nasty-nl-nbd.h"

#define	N	struct nasty_conf *n
struct nasty_conf
  {
    struct nasty_nl_nbd	nl;
    uint64_t		devsize;	/* device size in bytes.  Device always has blocksize 4096	*/
    int			kernel_fd;	/* kernel side of socket	*/
    int			user_fd;	/* userspace side of socket	*/
  };

static void
nasty_init(N)
{
  int	fd[2];

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd))
    OOPS("cannot get socketpair()");

  nasty_nl_init(&n->nl);
  n->kernel_fd	= fd[0];
  n->user_fd	= fd[1];
}

void
loop(N)
{
  000;
  STDOUTf(".");
  sleep(1);
}

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int
main(int argc, char **argv)
{
  static struct nasty_conf _N;
  N = &_N;

  nasty_init(n);

  /* get the device paramters	*/

  n->devsize	= 1024llu * 1024llu * 1024llu;		/* 1 GB fits into memory	*/

#if 0
  struct sockaddr_in	sa;

  n->kernel_fd	= socket(AF_INET, SOCK_STREAM, 0);
  sa.sin_family		= AF_INET;
  sa.sin_port		= htons(9999);
  if (inet_pton(sa.sin_family, "127.0.0.1", &sa.sin_addr)<=0)
    OOPS("inet_pton(127.0.0.1)");
  if (connect(n->kernel_fd, (struct sockaddr *)&sa, sizeof sa))
    OOPS("connect failed");
#endif

  /* now open /dev/nbdX	*/
  nasty_nl_start(&n->nl, NASTY_NL_NBD_ANY, 1, &n->kernel_fd, n->devsize);
  LOG("got /dev/nbd", fINT(n->nl.nbd));

  /* close the unused server socket at our side	*/
  if (close(n->kernel_fd))
    OOPS("cannot close kernel FD side: ", fINT(n->kernel_fd));
  n->kernel_fd	= -1;

  /* now run the device	.. forever? */
  for (;;)
    loop(n);
}

