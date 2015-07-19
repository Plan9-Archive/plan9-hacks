#include <u.h>
#include <libc.h>
#include <bio.h>
#include <thread.h>
#include "irc.h"

static int
parsemsg(IrcMsg *msg, char *buf)
{
	char *p, *p2;

	snprint(msg->buf, sizeof(msg->buf), "%s", buf);

	p = msg->buf;

	if(*p == ':'){
		p2 = strchr(p, ' ');
		if(p2 == nil){
			werrstr("short line");
			goto bad;
		}
		*p2++ = 0;
		msg->prefix = p+1;
		p = p2;
	}

	p2 = strchr(p, ':');
	if(p2 != nil){
		*p2++ = 0;
		msg->trailer = p2;
	}

	p2 = strchr(p, ' ');
	if(p2 != nil){
		*p2++ = 0;
		msg->nparams = getfields(p2, msg->params, 16, 1, " ");
	}

	msg->cmd = p;

	//fprint(2, "prefix '%s' cmd '%s' nparams %d trailer '%s'\n",
	//	msg->prefix, msg->cmd, msg->nparams, msg->trailer);

	return 1;

bad:
	return -1;
}

static void
connproc(void *v)
{
	Irc *conn;
	IrcMsg *msg;
	int n;
	char *p;

	conn = v;
	while((p = Brdstr(&conn->bin, '\n', 1)) != nil){
		n = Blinelen(&conn->bin);
		p[n-1] = 0;
		//fprint(2, "irc message '%s'\n", p);
		msg = mallocz(sizeof(*msg), 1);
		if(parsemsg(msg, p) < 0)
			sysfatal("protocol botch: %r");

		sendp(conn->in, msg);
	}
}

Irc*
ircconn(char *addr, char *nick, char *user, char *real)
{
	Irc *conn;

	conn = mallocz(sizeof(*conn), 1);
	if(conn == nil)
		return nil;

	conn->io = ioproc();
	if(conn->io == nil)
		return nil;

	conn->fd = iodial(conn->io, addr, nil, nil, nil);

	if(conn->fd < 0){
		closeioproc(conn->io);
		free(conn);
		return nil;
	}

	conn->in = chancreate(sizeof(IrcMsg*), 0);
	conn->out = chancreate(sizeof(char*), 0);

	Binit(&conn->bin, conn->fd, OREAD);
	proccreate(connproc, conn, 8192);

	ircmsg(conn, "USER %s * * :%s", user, real);
	ircmsg(conn, "NICK %s", nick);

	return conn;
}

static int
ircsend(Irc *conn, char *buf, long n)
{
	return iowrite(conn->io, conn->fd, buf, n);
}

int ircmsg(Irc *conn, char *fmt, ...)
{
	int n;
	va_list args;
	char buf[513];
	va_start(args, fmt);
	n = vsnprint(buf, sizeof(buf), fmt, args);
	fprint(2, "SEND '%s'\n", buf);
	if(n < 510){
		buf[n++] = '\r';
		buf[n++] = '\n';
		buf[n] = 0;
	}

	return ircsend(conn, buf, n);
}

Channel *privmsg;

void
privmsgproc(void *v)
{
	Channel *c;
	IrcMsg *m;
	char *bang;

	c = v;
	for(;;){
		m = recvp(c);
		if((bang = strchr(m->prefix, '!')) != nil)
			*bang = 0;
		
		print("<%s> %s\n", m->prefix, m->trailer);
		free(m);
	}
}

void
threadmain(int argc, char *argv[])
{
	char *user;
	Irc *conn;
	IrcMsg *msg;
	IrcCmdHook *hook;

	user = getenv("user");

	conn = ircconn("tcp!chat.freenode.net!6667", "glentest", user, user);

	privmsg = chancreate(sizeof(IrcMsg*), 0);
	threadcreate(privmsgproc, privmsg, 8192);

	irchook(conn, "PRIVMSG", privmsg);

	Alt a[] = {
		{conn->in, &msg, CHANRCV},
		{nil, nil, CHANEND},
	};

	for(;;){
loop:
		switch(alt(a)){
		case 0:
			print("%s %s %s\n", msg->cmd, msg->prefix, msg->trailer);

			for(hook = conn->hooks; hook != nil; hook = hook->next){
				if(cistrcmp(hook->cmd, msg->cmd) == 0){
					sendp(hook->out, msg);
					goto loop;
				}
			}

			if(cistrcmp(msg->cmd, "ERROR") == 0){
				sysfatal("irc error: %s", msg->trailer);
			} else if(cistrcmp(msg->cmd, "PING") == 0){
				ircmsg(conn, "PONG :%s", msg->trailer);
			}
			free(msg);
			break;
		}

	}
	threadexitsall(nil);
}

void
irchook(Irc *conn, char *cmd, Channel *c)
{
	IrcCmdHook *hook;

	hook = mallocz(sizeof(*hook), 1);
	snprint(hook->cmd, sizeof(hook->cmd), "%s", cmd);
	hook->out = c;

	hook->next = conn->hooks;
	conn->hooks = hook;
}

