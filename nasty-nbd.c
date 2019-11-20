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
  n->kernel_fd	= fd[0];
  n->user_fd	= fd[1];

  nasty_nl_open(&n->nl);
}


static void
nbd_read(N, void *ptr, size_t len)
{
  while (len)
    {
      int	got;

      got	= read(n->user_fd, ptr, len);
      if (got<=0)
        {
          if (!got)
            OOPS("EOF");
          if (errno == EAGAIN || errno == EINTR)
            continue;
          OOPS("read error");
        }
      ptr	= ((char *)ptr)+got;
      len	-= got;
    }
}

static void
dump(const char *what, const void *ptr, size_t len)
{
  const unsigned char	*p = ptr;

  while (len)
    {
      int	i;

      STDOUT(what, ":\t");
      for (i=0; i<16 && i<len; i++)
        STDOUT(fBASE(16), fWIDTH(2), fFILL('0'), fU8(p[i]), " ");
      while (++i<=16)
        STDOUT("   ");
      STDOUT(" ! ");
      for (i=0; i<16 && i<len; i++)
        if (p[i]>=32 && p[i]!=128)
          STDOUT(fCHAR(p[i]));
        else
          STDOUT(" ");
      STDOUT("\n");
      len	-= i;
      p		+= i;
    }
}

#ifndef ntohll
/* stolen at https://github.com/NetworkBlockDevice/nbd/blob/29f1c78da941179aba783ea29316d52c6fa712f5/cliserv.c#L89-L103 */
#ifdef WORDS_BIGENDEAN
static uint64t ntohll(uint64_t x) { return x; }
#else
static uint64_t
ntohll(uint64_t a)
{
  uint32_t lo = a & 0xffffffff;
  uint32_t hi = a >> 32U;
  lo = ntohl(lo);
  hi = ntohl(hi);
  return ((uint64_t) lo) << 32U | hi;
}
#endif
#endif

void
loop(N)
{
  uint32_t	magic	= ntohl(NBD_REQUEST_MAGIC);
  uint32_t	type, len;
  uint64_t	from;

/*
        __be32 magic;
        __be32 type;    == READ || == WRITE
        char handle[8];
        __be64 from;
        __be32 len;

        case NBD_CMD_READ:
        case NBD_CMD_WRITE:
        case NBD_CMD_DISC:
        case NBD_CMD_FLUSH:
        case NBD_CMD_TRIM:
        case NBD_CMD_WRITE_ZEROES:
*/

  struct nbd_request	r;

  nbd_read(n, &r, sizeof r);
  dump("got", &r, sizeof r);
  if (r.magic != magic)
    OOPS("protocol error: wrong magic");

  from	= ntohll(r.from);
  type	= ntohl(r.type);
  len	= ntohl(r.len);
  LOG("cmd=", fU32(type), " from=", fU64(from), " len=", fU32(len));

  sleep(1);
}

static int
run(N, int dev)
{
  /* get the device paramters	*/

  n->devsize	= 1024llu * 1024llu * 1024llu;		/* 1 GB fits into memory	*/

  /* now open /dev/nbdX	*/
  nasty_nl_start(&n->nl, dev, n->devsize, 1, &n->kernel_fd);
  LOG("got /dev/nbd", fINT(n->nl.nbd));

  /* close the unused server socket at our side	*/
  if (close(n->kernel_fd))
    OOPS("cannot close kernel FD side: ", fINT(n->kernel_fd));
  n->kernel_fd	= -1;

  /* now run the device	.. forever? */
  for (;;)
    loop(n);

  return 0;
}

static int
attach(N, char **args)
{
  while (*args)
    {
      int	nr;

      if (sscanf(*args, "/dev/nbd%d", &nr) != 1 || nr<0)
        OOPS("not /dev/nbdX");
      LOG("attach /dev/nbd", fINT(nr));
      run(n, nr);
    }
  nasty_nl_close(&n->nl);
  return 0;
}

static int
detach(N, char **args)
{
  while (*args)
    {
      int	nr;

      if (sscanf(*args, "/dev/nbd%d", &nr) != 1 || nr<0)
        OOPS("not /dev/nbdX");
      LOG("detach /dev/nbd", fINT(nr));
      nasty_nl_stop(&n->nl, nr);
    }
  nasty_nl_close(&n->nl);
  return 0;
}

static int
cmd(N, char **args)
{
  if (!strcmp(args[0], "detach"))	return detach(n, args+1);
  if (!strcmp(args[0], "attach"))	return attach(n, args+1);
  OOPS("unknown command");
}

int
main(int argc, char **argv)
{
  static struct nasty_conf _N;
  N = &_N;

  nasty_init(n);

  if (argc>1)
     return cmd(n, argv+1);

  return run(n, NASTY_NL_NBD_ANY);
}

