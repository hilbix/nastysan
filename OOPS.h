/* Common things
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include <unistd.h>

#define	xDP(X...)	do {} while (0)
#if 0
#define	DP(X...)	do {} while (0)
#else
#define	DP(X...)	do { debugprintf(__FILE__, __LINE__, __FUNCTION__, X); } while (0)
static void debugprintf(const char *file, int line, const char *fn, const char *s, ...)
{
  va_list	list;

  fprintf(stderr, "[[%s:%d %s", file, line, fn);
  va_start(list, s);
  vfprintf(stderr, s, list);
  va_end(list);
  fprintf(stderr, "]]\n");
  fflush(stderr);
}
#endif

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
  int	base=10, width=0, fill=0, sign=0;

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

      base=10, width=0, fill=0, sign=0;
    }
}

#define	FORMAT(X...)	_FORMAT(X, NULL)
static void
_FORMAT(void (*cb)(void *user, const void *, size_t len), void *user, const char *s, ...)
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

#define	STDERR(X...)	_STDERR(X, NULL)
static void
_STDERR(const char *s, ...)
{
  va_list	list;

  va_start(list, s);
  vFORMAT(OUT, stderr, s, list);
  va_end(list);
}

#define	STDOUT(X...)	_STDOUT(X, NULL)
static void
_STDOUT(const char *s, ...)
{
  va_list	list;

  va_start(list, s);
  vFORMAT(OUT, stdout, s, list);
  va_end(list);
}

#define	STDOUTf(X...)	_STDOUTf(X, NULL)
static void
_STDOUTf(const char *s, ...)
{
  va_list	list;

  va_start(list, s);
  vFORMAT(OUT, stdout, s, list);
  va_end(list);
  fflush(stdout);
}

#define	OOPS(X...)	_OOPS(X, NULL)
__attribute__((noreturn))
static void
_OOPS(const char *s, ...)
{
  int		e = errno;
  va_list	list;

  va_start(list, s);
  FORMAT(OUT, stderr, "OOPS: ", fARGS(s, list), ": ", strerror(e), "\n", NULL);
  va_end(list);
  exit(23);
  abort();
  for(;;) pause();
}

#define	LOG(X...)	_LOG(X, NULL)
static void
_LOG(const char *s, ...)
{
  va_list	list;

  va_start(list, s);
  STDOUTf(fARGS(s, list), "\n", NULL);
  va_end(list);
}

