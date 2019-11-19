/* NetLink NBD binding
 */
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

/* Populate ->nl, ->id and ->nbd
 */
static struct nasty_nl_nbd *
nasty_nl_start(struct nasty_nl_nbd *nl, int nbd, int n, int *fds)
{
  struct nl_msg		*msg;
  struct nlattr		*attr, *sock;
  int			i;

  nl_socket_modify_cb(nl->sock, NL_CB_VALID, NL_CB_CUSTOM, nasty_nl_get_nbd_index, nl);

  msg	= nlmsg_alloc();
  if (!msg)
    OOPS("cannot allocate netlink message");

  if (nbd >= 0)		NLA_PUT_U32(msg,	NBD_ATTR_INDEX,			nbd);
//  if (1)		NLA_PUT_U64(msg,	NBD_ATTR_SIZE_BYTES,		size64);
//  if (1)		NLA_PUT_U64(msg,	NBD_ATTR_BLOCK_SIZE_BYTES,	blocksize);
//  if (1)		NLA_PUT_U64(msg,	NBD_ATTR_SERVER_FLAGS,		flags);
//  if (timeout)	NLA_PUT_U64(msg,	NBD_ATTR_TIMEOUT,		timeout);

  genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, nl->id, 0, 0, NBD_CMD_CONNECT, 0);

  attr	= nla_nest_start(msg, NBD_ATTR_SOCKETS);
  if (!attr)
    OOPS("netlink 'nest' failed for ", "ATTR");

  for (i=0; i<n; i++)
    {
      sock	= nla_nest_start(msg, NBD_SOCK_ITEM);
      if (!sock)
        OOPS("netlink 'nest' failed for ", "SOCK");
      NLA_PUT_U32(msg, NBD_SOCK_FD, fds[i]);
      nla_nest_end(msg, sock);
    }
  nla_nest_end(msg, attr);

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

