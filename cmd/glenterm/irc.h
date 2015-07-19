typedef struct IrcMsg IrcMsg;
typedef struct IrcCmdHook IrcCmdHook;
typedef struct Irc Irc;

struct IrcMsg
{
	char *prefix;
	char *cmd;
	int nparams;
	char *params[16];
	char *trailer;
	char buf[512];
};

struct IrcCmdHook
{
	char cmd[32]; /* PRIVMSG */
	Channel *out; /* IrcMsg* */
	IrcCmdHook *next;
};

struct Irc
{
	int fd;

	Biobuf bin;
	Ioproc *io;

	Channel *in;
	Channel *out;

	IrcCmdHook *hooks;
};

Irc *ircconn(char *addr, char *nick, char *real, char *user);
int ircmsg(Irc *conn, char *fmt, ...);
void irchook(Irc *conn, char *cmd, Channel *c);

