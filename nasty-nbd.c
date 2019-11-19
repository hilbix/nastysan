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

int
main(int argc, char **argv)
{
  static struct nasty_conf _N;
  N = &_N;

  nasty_init(n);
  nasty_nl_start(&n->nl, NASTY_NL_NBD_ANY, 1, &n->kernel_fd);
  LOG("got /dev/nbd", fINT(n->nl.nbd));

  if (close(n->kernel_fd))
    OOPS("cannot close kernel FD side: ", fINT(n->kernel_fd));
  n->kernel_fd	= -1;
}

