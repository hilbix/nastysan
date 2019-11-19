/* NetLink NBD binding
 *
 * This mostly was reverse engineered from the nbd-client sources
 * plus some sweat, blood and tears (read: documentation sucks).
 */
#include <linux/nbd.h>
#include <linux/nbd-netlink.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

#define	NASTY_NL_NBD_ANY	-1

struct nasty_nl_nbd
  {
    int			nbd;		/* NBD number /dev/nbdX	*/
    struct nl_sock	*sock;		/* netlink socket	*/
    int			id;		/* netlink driver ID	*/
    int			version;	/* interface version	*/
  };

struct nasty_nl_msg
  {
    struct nasty_nl_nbd	*nl;		/* netlink connection	*/
    struct nl_msg	*msg;		/* message		*/
    struct nasty_nl_nest *nest;		/* nested things	*/
  };

struct nasty_nl_nest
  {
    struct nasty_nl_nest *next;		/* parent structure	*/
    struct nlattr	*attr;		/* attr to work with	*/
  };

/*
 * Helpers
 */

/* Nesting begin with automatic bookkeeping
 */
#define	nasty_nl_msg_beg(M,T)	_nasty_nl_msg_beg(M,T,#T)
static struct nasty_nl_msg *
_nasty_nl_msg_beg(struct nasty_nl_msg *m, int typ, const char *hint)
{
  struct nlattr		*a;
  struct nasty_nl_nest	*nest;

  a	= nla_nest_start(m->msg, typ);
  if (!a)
    OOPS("netlink 'nest' failed for ", hint);
  nest	= alloc(sizeof *nest);
  nest->next	= m->nest;
  nest->attr	= a;
  m->nest	= nest;
  return m;
}

/* Nesting end with automatic bookkeeping
 */
static struct nasty_nl_msg *
nasty_nl_msg_end(struct nasty_nl_msg *m)
{
  struct nasty_nl_nest	*nest;

  nest	= m->nest;
  if (!nest)
    OOPS("netfilter nesting internal error, too many ends");
  m->nest	= nest->next;
  if (nla_nest_end(m->msg, nest->attr))
    OOPS("netfilter nesting end fails");
  free(nest);
  return m;
}

/* Allocate a message
 */
static struct nasty_nl_msg *
nasty_nl_msg(struct nasty_nl_nbd *nl)
{
  struct nasty_nl_msg	*r;
  struct nl_msg		*msg;

  msg	= nlmsg_alloc();
  if (!msg)
    OOPS("cannot allocate netlink message");
  r		= alloc(sizeof *r);
  r->nl		= nl;
  r->msg	= msg;
  r->nest	= 0;
  return r;
}

static struct nasty_nl_nbd *
nasty_nl_msg_free(struct nasty_nl_msg *m)
{
  struct nasty_nl_nbd *nl;

  nlmsg_free(m->msg);
  nl	= m->nl;
  free(m);
  return nl;
}

static struct nasty_nl_nbd *
nasty_nl_msg_send_free(struct nasty_nl_msg *m)
{
  struct nasty_nl_nbd	*nl;
  struct nl_msg		*msg;

  if (m->nest)
    OOPS("netlink internal error: nesting not finished before send");

  nl	= m->nl;
  msg	= m->msg;
  free(m);
  if (nl_send_sync(nl->sock, msg) < 0)
    OOPS("netlink setup failed, see 'dmesg'");
  return nl;
}

/* put standard headers (command) into message
 */
static struct nasty_nl_msg *
nasty_nl_msg_put(struct nasty_nl_msg *m, uint8_t cmd)
{
  void	*usrhdr;	/* here: 0 bytes	*/

  usrhdr	= genlmsg_put(m->msg, NL_AUTO_PORT, NL_AUTO_SEQ, m->nl->id, /*usrhdrlen=*/0, /*flags=*/0, NBD_CMD_CONNECT, m->nl->version);
  if (!usrhdr)
    OOPS("failed to add generic netlink headers");
  return m;
}

static struct nasty_nl_msg *
nasty_nl_msg_u32(struct nasty_nl_msg *m, int attr, uint32_t u)
{
  if (nla_put_u32(m->msg, attr, u) < 0)
    OOPS("netlink failed to put 32 bit into message");
  return m;
}

static struct nasty_nl_msg *
nasty_nl_msg_u64(struct nasty_nl_msg *m, int attr, uint64_t u)
{
  if (nla_put_u64(m->msg, attr, u) < 0)
    OOPS("netlink failed to put 64 bit into message");
  return m;
}


/*
 * open/close netlink
 */

static struct nasty_nl_nbd *
nasty_nl_open(struct nasty_nl_nbd *nl)
{
  memset(nl, 0, sizeof *nl);	/* ->version=0 etc.	*/

  nl->nbd	= -1;
  nl->sock	= nl_socket_alloc();
  if (!nl->sock)
    OOPS("cannot allocate netlink socket");
  if (genl_connect(nl->sock))
    OOPS("cannot connect to generic netlink socket");
  nl->id	= genl_ctrl_resolve(nl->sock, NBD_GENL_FAMILY_NAME);
  if (nl->id < 0)
    OOPS("could not get netlink driver (Kernel>=3.17 and module loaded?): ", NBD_GENL_FAMILY_NAME);
  return nl;
}

static struct nasty_nl_nbd *
nasty_nl_close(struct nasty_nl_nbd *nl)
{
  nl_socket_free(nl->sock);
  nl->sock	= 0;
  nl->id	= 0;
  nl->nbd	= -1;
  return nl;
}


/*
 * start/stop NBD
 */

/* INTERNAL callback:  Receive the NBD device number ->nbd
 */
static int
nasty_nl_get_nbd_index(struct nl_msg *msg, void *user)
{
  struct nasty_nl_nbd	*nl = user;
  struct genlmsghdr	*gnmh;
  struct nlattr		*att, *attr[NBD_ATTR_MAX + 1];

  gnmh	= nlmsg_data(nlmsg_hdr(msg));									/* WTF?	*/
  if (nla_parse(attr, NBD_ATTR_MAX, genlmsg_attrdata(gnmh, 0), genlmsg_attrlen(gnmh, 0), NULL))		/* WTF?	*/
    OOPS("invalid kernel response");

  att	= attr[NBD_ATTR_INDEX];
  if (!att)
    OOPS("kernel did not send index");

  nl->nbd	= nla_get_u32(att);

  return NL_OK;
}

/* Start an NBD drive.  The drive talks to the given sockets.
 *
 * before:	nasty_nl_open()
 * after:	nasty_nl_close() is possible
 * later:	nasty_nl_stop() to stop the NBD again
 *
 * nbd		NASTY_NL_NBD_ANY to request a fresh unused NBD
 * nbdsize	size of device in bytes, must be multiple of the blocksize (which is 4096 here)
 * socks	number of sockets in array
 * fds		array of sockets
 */
static struct nasty_nl_nbd *
nasty_nl_start(struct nasty_nl_nbd *nl, int nbd, uint64_t nbdsize, int socks, int *fds)
{
  struct nasty_nl_msg	*m;
  int			i, flags, blocksize;

  /* receive the NBD device number from netlink	*/
  nl_socket_modify_cb(nl->sock, NL_CB_VALID, NL_CB_CUSTOM, nasty_nl_get_nbd_index, nl);

  /* /usr/include/linux/nbd.h:
   *
   * NBD_FLAG_HAS_FLAGS		(1 << 0)	nbd-server supports flags
   * NBD_FLAG_READ_ONLY		(1 << 1)	device is read-only
   * NBD_FLAG_SEND_FLUSH	(1 << 2)	can flush writeback cache
   * NBD_FLAG_SEND_FUA		(1 << 3)	send FUA (forced unit access)
   * NBD_FLAG_SEND_TRIM		(1 << 5)	send trim/discard
   * NBD_FLAG_CAN_MULTI_CONN	(1 << 8)	Server supports multiple connections per export.
   */
  flags	= NBD_FLAG_HAS_FLAGS|NBD_FLAG_SEND_FLUSH|NBD_FLAG_SEND_FUA|NBD_FLAG_SEND_TRIM;
  if (socks>1)
    flags	|= NBD_FLAG_CAN_MULTI_CONN;

  blocksize	= 4096;				/* everything else is bullshit	*/
  nbdsize	&= ~(uint64_t)(blocksize-1);

  m	= nasty_nl_msg(nl);
  nasty_nl_msg_put(m, NBD_CMD_CONNECT);

  /* Send all the sockets to use
   */
  if (!socks) OOPS("missing sockets");
  nasty_nl_msg_beg(m, NBD_ATTR_SOCKETS);
  for (i=0; i<socks; i++)
    {
      xDP("nl sock %d", fds[i]);
      nasty_nl_msg_beg(m, NBD_SOCK_ITEM);
      nasty_nl_msg_u32(m, NBD_SOCK_FD, fds[i]);
      nasty_nl_msg_end(m);
    }
  nasty_nl_msg_end(m);

  /* Why do sockets fail to be detected if done after these options?
   */
  if (nbd >= 0)		nasty_nl_msg_u32(m,	NBD_ATTR_INDEX,			nbd);
//if (timeout)		nasty_nl_msg_u64(m,	NBD_ATTR_TIMEOUT,		timeout);
  if (flags)		nasty_nl_msg_u64(m,	NBD_ATTR_SERVER_FLAGS,		flags);
  if (blocksize)	nasty_nl_msg_u64(m,	NBD_ATTR_BLOCK_SIZE_BYTES,	blocksize);
  if (nbdsize)		nasty_nl_msg_u64(m,	NBD_ATTR_SIZE_BYTES,		nbdsize);

  nasty_nl_msg_send_free(m);
  return nl;
}

/* Stop an NBD drive again.  The number must be given
 *
 * before:	nasty_nl_open()
 * after:	nasty_nl_close()
 */
static struct nasty_nl_nbd *
nasty_nl_stop(struct nasty_nl_nbd *nl, int nbd)
{
  struct nasty_nl_msg	*m;

  m	= nasty_nl_msg(nl);
  nasty_nl_msg_put(m, NBD_CMD_DISCONNECT);
  nasty_nl_msg_u32(m, NBD_ATTR_INDEX, nbd);
  nasty_nl_msg_send_free(m);
  return nl;
}

