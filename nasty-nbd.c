/* NAStySAN NBD adapter
 *
 * This creates a NAStySAN block device using /dev/ndb
 * The device is meant to be fault tolerant and reliable.
 *
 * We use the new netlink driver which needs Kernel 3.17 and above.
 * WHY IS THERE NO DOCUMENTATION ON THIS SUBJECT WHATSOEVER?
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include <unistd.h>

#include <linux/nbd-netlink.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

#define	N	struct nastysan_conf *n
struct nastysan_conf
  {
    struct nl_sock	*nl;	/* netlink socket	*/
    int			nl_id;	/* netlink driver ID	*/
    int			nbd_nr;	/* NBD number /dev/nbdX	*/
    int			kernel_fd;	/* kernel side of socket	*/
    int			user_fd;	/* userspace side of socket	*/
  };

enum
  {
  FORMAT_NULL = 0,

  FORMAT_BASE,
  FORMAT_WIDTH,
  FORMAT_FILL,
  FORMAT_SIGN,
  FORMAT_PLUS,

  FORMAT_ARGS,

  FORMAT_I8,
  FORMAT_I16,
  FORMAT_I32,
  FORMAT_I64,
  FORMAT_CHAR,
  FORMAT_INT,
  FORMAT_LONG,
  FORMAT_LL,

  FORMAT_U8,
  FORMAT_U16,
  FORMAT_U32,
  FORMAT_U64,
  FORMAT_UCHAR,
  FORMAT_UINT,
  FORMAT_ULONG,
  FORMAT_ULL,
  };

#define	fARGS(s,v)	(void *)FORMAT_ARGS, (const char *)s, &v
#define	fINT(X)		(void *)FORMAT_INT, (int)(X)

static size_t
FORMATnr(char *buf, size_t len, unsigned long long n, int base)
{
  const char	*b;

  if (base<0)
    {
      base	= -base;
      if (base<-64)
        b	= "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!#$%&()*+-;<=>?@^_`{|}~";	/* RFC1924	*/
      else if (base<-62)
        b	= "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";				/* B64url	*/
      else
        b	= "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";				/* HEX	*/
    }
  else
    {
      if (base>64)
        b	= "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.-:+=^!/*?&<>()[]{}@%$#";	/* Z85	*/
      else if (base>62)
        b	= "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";				/* B64	*/
      else
        b	= "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";				/* hex	*/
    }

#if 0
  buf[--len]	= 0;
#endif
  if (!n)
    {
      buf[--len]	= b[0];
      return len;
    }

  if (base<0)	base	= -base;
  if (base<2)	base	= 2;
  if (base>85)	base	= 85;

  do
    {
      buf[--len]	= b[n%base];
    } while (n /= base);

  return len;
}

static void
FORMATfill(char fill, int width, void (*cb)(void *user, const void *, size_t len), void *user, char *buf, size_t len)
{
  if (width < 0)
    return;
  memset(buf, fill, len);
  while ((width -= len)>0)
    cb(user, buf, len);
  cb(user, buf, len+width);
}

/* No floating point for now
 */
static void
vFORMAT(void (*cb)(void *user, const void *, size_t len), void *user, const char *s, va_list list)
{
  int	base=0, width=0, fill=0, sign=0;

  for (;; s=va_arg(list, void *))
    {
      int			type;
      long long			ll;
      unsigned long long	ull;
      char			tmp[PATH_MAX];
      size_t			n;

      switch ((uintptr_t)s)
        {
        case FORMAT_ARGS:	s	= va_arg(list, const char *); vFORMAT(cb, user, s, *va_arg(list, va_list *)); continue;
        case FORMAT_NULL:	return;
        default:
          n	= strlen(s);
          FORMATfill(fill, width-n, cb, user, tmp, sizeof tmp);
          goto out;

        case FORMAT_BASE:	base	= va_arg(list, int);		continue;
        case FORMAT_WIDTH:	width	= va_arg(list, int);		continue;
        case FORMAT_FILL:	fill	= (char)va_arg(list, int);	continue;
        case FORMAT_SIGN:	sign	= fill;				continue;
        case FORMAT_PLUS:	sign	= '+';				continue;


        case FORMAT_I8:		ll	= (int8_t)va_arg(list, int);	type=1; break;
        case FORMAT_I16:	ll	= (int16_t)va_arg(list, int);	type=1; break;
        case FORMAT_I32:	ll	= va_arg(list, int32_t);	type=1; break;
        case FORMAT_I64:	ll	= va_arg(list, int64_t);	type=1; break;
        case FORMAT_CHAR:	ll	= (char)va_arg(list, int);	type=1; break;
        case FORMAT_INT:	ll	= va_arg(list, int);		type=1; break;
        case FORMAT_LONG:	ll	= va_arg(list, long);		type=1; break;
        case FORMAT_LL:		ll	= va_arg(list, long);		type=1; break;

        case FORMAT_U8:		ull	= (uint8_t)va_arg(list, int);	type=2; break;
        case FORMAT_U16:	ull	= (uint16_t)va_arg(list, int);	type=2; break;
        case FORMAT_U32:	ull	= va_arg(list, uint32_t);	type=2; break;
        case FORMAT_U64:	ull	= va_arg(list, uint64_t);	type=2; break;
        case FORMAT_UCHAR:	ull	= (unsigned char)va_arg(list, int);		type=2; break;
        case FORMAT_UINT:	ull	= va_arg(list, unsigned int);	type=2; break;
        case FORMAT_ULONG:	ull	= va_arg(list, unsigned long);	type=2; break;
        case FORMAT_ULL:	ull	= va_arg(list, unsigned long);	type=2; break;
        }
      if (type==1)
        {
          if (ll<0)
            {
              sign	= '-';
              ull	= -ll;
            }
          else
            ull	= ll;
        }

      /* If fill is too big, pre-fill as this uses tmp buffer
       */
      n	= sizeof tmp - 8*sizeof ull;
      if (width > n)
        {
          FORMATfill(fill, width - n, cb, user, tmp, sizeof tmp);
          width	= n;
        }

      /* get number	*/
      n	= FORMATnr(tmp, sizeof tmp, ull, base);
      if (sign)
        tmp[--n]	= sign;

      /* fill it on the left	*/
      while (n >= sizeof tmp - width && n)
        tmp[--n]	= fill;

      s	= tmp+n;
      n	= sizeof tmp - n;

out:
      /* output	*/
      cb(user, s, n);

      /* fill it on the right	*/
      FORMATfill(fill, -width-n, cb, user, tmp, sizeof tmp);
    }
}

static void
FORMAT(void (*cb)(void *user, const void *, size_t len), void *user, const char *s, ...)
{
  va_list	list;

  va_start(list, s);
  vFORMAT(cb, user, s, list);
  va_end(list);
}

static void
OUT(void *user, const void *ptr, size_t len)
{
  fwrite(ptr, len, (size_t)1, user);
}

static void
STDERR(const char *s, ...)
{
  va_list	list;

  va_start(list, s);
  vFORMAT(OUT, stderr, s, list);
  va_end(list);
}

static void
STDOUT(const char *s, ...)
{
  va_list	list;

  va_start(list, s);
  vFORMAT(OUT, stdout, s, list);
  va_end(list);
}

static void
OOPS(const char *s, ...)
{
  int		e = errno;
  va_list	list;

  va_start(list, s);
  FORMAT(OUT, stderr, "OOPS: ", fARGS(s, list), ": ", strerror(e), "\n", NULL);
  va_end(list);
  exit(23);
  for(;;) abort();
}

static void
LOG(const char *s, ...)
{
  va_list	list;

  va_start(list, s);
  STDOUT(fARGS(s, list), "\n", NULL);
  va_end(list);
}

/* Receive the NBD device number ->nbd
 */
static int
nb_get_nbd_index(struct nl_msg *msg, void *user)
{
  N = user;
  struct genlmsghdr	*gnmh;
  struct nlattr		*att, *attr[NBD_ATTR_MAX + 1];

  gnmh	= nlmsg_data(nlmsg_hdr(msg));									/* WTF?	*/
  if (nla_parse(attr, NBD_ATTR_MAX, genlmsg_attrdata(gnmh, 0), genlmsg_attrlen(gnmh, 0), NULL))		/* WTF?	*/
    OOPS("invalid kernel response", NULL);

  att	= attr[NBD_ATTR_INDEX];
  if (!att)
    OOPS("kernel did not send index", NULL);
  n->nbd_nr	= nla_get_u32(att);
  return NL_OK;
}

static void
nl_init(N)
{
  n->nl	= nl_socket_alloc();
  if (!n->nl)
    OOPS("cannot allocate netlink socket", NULL);
  if (genl_connect(n->nl))
    OOPS("cannot connect to generic netlink socket", NULL);
  n->nl_id	= genl_ctrl_resolve(n->nl, NBD_GENL_FAMILY_NAME);
  if (n->nl_id < 0)
    OOPS("could not get netlink driver (Kernel>=3.17 and module loaded?): ", NBD_GENL_FAMILY_NAME, NULL);
}

/* Populate ->nl, ->id and ->nbd
 */
static void
nl_open(N)
{
  struct nl_msg		*msg;
  struct nlattr		*attr, *sock;

  nl_init(n);

  nl_socket_modify_cb(n->nl, NL_CB_VALID, NL_CB_CUSTOM, nb_get_nbd_index, n);

  msg	= nlmsg_alloc();
  if (!msg)
    OOPS("cannot allocate netlink message", NULL);

  if (n->nbd_nr >= 0)	NLA_PUT_U32(msg,	NBD_ATTR_INDEX,			n->nbd_nr);
//  if (1)		NLA_PUT_U64(msg,	NBD_ATTR_SIZE_BYTES,		size64);
//  if (1)		NLA_PUT_U64(msg,	NBD_ATTR_BLOCK_SIZE_BYTES,	blocksize);
//  if (1)		NLA_PUT_U64(msg,	NBD_ATTR_SERVER_FLAGS,		flags);
//  if (timeout)	NLA_PUT_U64(msg,	NBD_ATTR_TIMEOUT,		timeout);

  genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, n->nl_id, 0, 0, NBD_CMD_CONNECT, 0);

  attr	= nla_nest_start(msg, NBD_ATTR_SOCKETS);
  if (!attr)
    OOPS("netlink 'nest' failed for ", "ATTR", NULL);

  sock	= nla_nest_start(msg, NBD_SOCK_ITEM);
  if (!sock)
    OOPS("netlink 'nest' failed for ", "SOCK", NULL);
  NLA_PUT_U32(msg, NBD_SOCK_FD, n->kernel_fd);
  nla_nest_end(msg, sock);

  nla_nest_end(msg, attr);

  if (nl_send_sync(n->nl, msg) < 0)
    OOPS("netlink setup failed, see 'dmesg'", NULL);
  return;

nla_put_failure:	/* WTF? Those macros need a goto?	*/
  OOPS("netlink message put failure", NULL);
}

static void
nl_stop(N)
{
  OOPS("nl_stop() not yet implemented", NULL);
}

static void
nastysan_init(N)
{
  int	fd[2];

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd))
    OOPS("cannot get socketpair()", NULL);

  n->kernel_fd	= fd[0];
  n->user_fd	= fd[1];
  n->nbd_nr	= -1;
}

int
main(int argc, char **argv)
{
  static struct nastysan_conf _n;
  N = &_n;

  nastysan_init(n);
  nl_open(n);
  LOG("got /dev/nbd", fINT(n->nbd_nr), NULL);

  if (close(n->kernel_fd))
    OOPS("cannot close kernel FD side: ", fINT(n->kernel_fd), NULL);
  n->kernel_fd	= -1;
}

