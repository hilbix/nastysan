/* NetLink NBD binding
 *
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
  };

/* Receive the NBD device number ->nbd
 */
static int
nasty_nl_get_nbd_index(struct nl_msg *msg, void *user)
{
  struct nasty_nl_nbd	*nl = user;
  struct genlmsghdr	*gnmh;
  struct nlattr		*att, *attr[NBD_ATTR_MAX + 1];

  DP("index");

  gnmh	= nlmsg_data(nlmsg_hdr(msg));									/* WTF?	*/
  if (nla_parse(attr, NBD_ATTR_MAX, genlmsg_attrdata(gnmh, 0), genlmsg_attrlen(gnmh, 0), NULL))		/* WTF?	*/
    OOPS("invalid kernel response");

  att	= attr[NBD_ATTR_INDEX];
  if (!att)
    OOPS("kernel did not send index");

  nl->nbd	= nla_get_u32(att);

  return NL_OK;
}

static struct nasty_nl_nbd *
nasty_nl_init(struct nasty_nl_nbd *nl)
{
  memset(nl, 0, sizeof *nl);

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

#define	NLA_NEST_START(M,T)	nasty_nla_nest_start(M,T,#T)

static struct nlattr *
nasty_nla_nest_start(struct nl_msg *msg, int typ, const char *what)
{
  struct nlattr		*a;

  a	= nla_nest_start(msg, typ);
  if (!a)
    OOPS("netlink 'nest' failed for ", what);
  return a;
}

/* Populate ->nl, ->id and ->nbd
 */
static struct nasty_nl_nbd *
nasty_nl_start(struct nasty_nl_nbd *nl, int nbd, int socks, int *fds, uint64_t nbdsize)
{
  struct nl_msg		*msg;
  struct nlattr		*attr, *sock;
  int			i, flags, blocksize;
  void			*v;

  nl_socket_modify_cb(nl->sock, NL_CB_VALID, NL_CB_CUSTOM, nasty_nl_get_nbd_index, nl);

  msg	= nlmsg_alloc();
  if (!msg)
    OOPS("cannot allocate netlink message");

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

  v	= genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, nl->id, 0, 0, NBD_CMD_CONNECT, 0);
  if (!v)
    OOPS("failed to add generic netlink headers");

  /* Open the sockets to use
   */
  if (!socks) OOPS("missing sockets");
  attr	= NLA_NEST_START(msg, NBD_ATTR_SOCKETS);
  for (i=0; i<socks; i++)
    {
      DP("nl sock %d", fds[i]);
      sock	= NLA_NEST_START(msg, NBD_SOCK_ITEM);
      NLA_PUT_U32(msg, NBD_SOCK_FD, fds[i]);
      nla_nest_end(msg, sock);
    }
  nla_nest_end(msg, attr);

  /* Why do sockets fail to be detected if done after these options?
   */
  if (nbd >= 0)		NLA_PUT_U32(msg,	NBD_ATTR_INDEX,			nbd);
//  if (timeout)	NLA_PUT_U64(msg,	NBD_ATTR_TIMEOUT,		timeout);

  if (flags)		NLA_PUT_U64(msg,	NBD_ATTR_SERVER_FLAGS,		flags);
  if (blocksize)	NLA_PUT_U64(msg,	NBD_ATTR_BLOCK_SIZE_BYTES,	blocksize);
  if (nbdsize)		NLA_PUT_U64(msg,	NBD_ATTR_SIZE_BYTES,		nbdsize);

  if (nl_send_sync(nl->sock, msg) < 0)
    OOPS("netlink setup failed, see 'dmesg'");
  return nl;

nla_put_failure:	/* WTF? Those macros need a goto?	*/
  OOPS("netlink message put failure");
}

static struct nasty_nl_nbd *
nasty_nl_stop(struct nasty_nl_nbd *nl, int nbd)
{
  OOPS("nl_stop() not yet implemented");
}

