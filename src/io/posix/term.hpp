#pragma once

// termios port

#include "../../linux/sys/ioctl.hpp"

#include "../../types.hpp"

namespace micron
{
namespace posix
{
using tcflag_t = unsigned int;
using cc_t = unsigned char;
using speed_t = unsigned int;

struct termios_t {
  tcflag_t c_iflag; /* input mode flags */
  tcflag_t c_oflag; /* output mode flags */
  tcflag_t c_cflag; /* control mode flags */
  tcflag_t c_lflag; /* local mode flags */
  cc_t c_line;      /* line discipline */
  cc_t c_cc[32];    /* control characters */
  speed_t c_ispeed; /* input speed */
  speed_t c_ospeed; /* output speed */
};

constexpr static const u64 TCSANOW = 0;
constexpr static const u64 TCSADRAIN = 1;
constexpr static const u64 TCSAFLUSH = 2;

/* c_cc characters */
constexpr static const u64 VINTR = 0;
constexpr static const u64 VQUIT = 1;
constexpr static const u64 VERASE = 2;
constexpr static const u64 VKILL = 3;
constexpr static const u64 VEOF = 4;
constexpr static const u64 VTIME = 5;
constexpr static const u64 VMIN = 6;
constexpr static const u64 VSWTC = 7;
constexpr static const u64 VSTART = 8;
constexpr static const u64 VSTOP = 9;
constexpr static const u64 VSUSP = 10;
constexpr static const u64 VEOL = 11;
constexpr static const u64 VREPRINT = 12;
constexpr static const u64 VDISCARD = 13;
constexpr static const u64 VWERASE = 14;
constexpr static const u64 VLNEXT = 15;
constexpr static const u64 VEOL2 = 16;
/* c_oflag bits */
constexpr static const u64 OPOST = 0000001; /* Post-process output.  */
constexpr static const u64 OLCUC = 0000002;
constexpr static const u64 ONLCR = 0000004;  /* Map NL to CR-NL on output.  */
constexpr static const u64 OCRNL = 0000010;  /* Map CR to NL on output.  */
constexpr static const u64 ONOCR = 0000020;  /* No CR output at column 0.  */
constexpr static const u64 ONLRET = 0000040; /* NL performs CR function.  */
constexpr static const u64 OFILL = 0000100;  /* Use fill characters for delay.  */
constexpr static const u64 OFDEL = 0000200;  /* Fill is DEL.  */
constexpr static const u64 NLDLY = 0000400;  /* Select newline delays:  */
constexpr static const u64 NL0 = 0000000;    /* Newline type 0.  */
constexpr static const u64 NL1 = 0000400;    /* Newline type 1.  */
constexpr static const u64 CRDLY = 0003000;  /* Select carriage-return delays:  */
constexpr static const u64 CR0 = 0000000;    /* Carriage-return delay type 0.  */
constexpr static const u64 CR1 = 0001000;    /* Carriage-return delay type 1.  */
constexpr static const u64 CR2 = 0002000;    /* Carriage-return delay type 2.  */
constexpr static const u64 CR3 = 0003000;    /* Carriage-return delay type 3.  */
constexpr static const u64 TABDLY = 0014000; /* Select horizontal-tab delays:  */
constexpr static const u64 TAB0 = 0000000;   /* Horizontal-tab delay type 0.  */
constexpr static const u64 TAB1 = 0004000;   /* Horizontal-tab delay type 1.  */
constexpr static const u64 TAB2 = 0010000;   /* Horizontal-tab delay type 2.  */
constexpr static const u64 TAB3 = 0014000;   /* Expand tabs to spaces.  */
constexpr static const u64 BSDLY = 0020000;  /* Select backspace delays:  */
constexpr static const u64 BS0 = 0000000;    /* Backspace-delay type 0.  */
constexpr static const u64 BS1 = 0020000;    /* Backspace-delay type 1.  */
constexpr static const u64 FFDLY = 0100000;  /* Select form-feed delays:  */
constexpr static const u64 FF0 = 0000000;    /* Form-feed delay type 0.  */
constexpr static const u64 FF1 = 0100000;    /* Form-feed delay type 1.  */

constexpr static const u64 VTDLY = 0040000; /* Select vertical-tab delays:  */
constexpr static const u64 VT0 = 0000000;   /* Vertical-tab delay type 0.  */
constexpr static const u64 VT1 = 0040000;   /* Vertical-tab delay type 1.  */

constexpr static const u64 XTABS = 0014000;

constexpr static const u64 ISIG = 0000001;   /* Enable signals.  */
constexpr static const u64 ICANON = 0000002; /* Canonical input (erase and kill processing).  */
constexpr static const u64 XCASE = 0000004;
constexpr static const u64 ECHO = 0000010; /* Enable echo.  */
constexpr static const u64 ECHOE = 0000020;
constexpr static const u64 ECHOK = 0000040;  /* Echo KILL.  */
constexpr static const u64 ECHONL = 0000100; /* Echo NL.  */
constexpr static const u64 NOFLSH = 0000200; /* Disable flush after interrupt or quit.  */
constexpr static const u64 TOSTOP = 0000400; /* Send SIGTTOU for background output.  */
constexpr static const u64 ECHOCTL = 0001000;
constexpr static const u64 ECHOPRT = 0002000;
constexpr static const u64 ECHOKE = 0004000;
constexpr static const u64 FLUSHO = 0010000;
constexpr static const u64 PENDIN = 0040000;
constexpr static const u64 IEXTEN = 0100000;
constexpr static const u64 EXTPROC = 0200000;

constexpr static const u64 IGNBRK = 0000001; /* Ignore break condition.  */
constexpr static const u64 BRKINT = 0000002; /* Signal interrupt on break.  */
constexpr static const u64 IGNPAR = 0000004; /* Ignore characters with parity errors.  */
constexpr static const u64 PARMRK = 0000010; /* Mark parity and framing errors.  */
constexpr static const u64 INPCK = 0000020;  /* Enable input parity check.  */
constexpr static const u64 ISTRIP = 0000040; /* Strip 8th bit off characters.  */
constexpr static const u64 INLCR = 0000100;  /* Map NL to CR on input.  */
constexpr static const u64 IGNCR = 0000200;  /* Ignore CR.  */
constexpr static const u64 ICRNL = 0000400;  /* Map CR to NL on input.  */
constexpr static const u64 IUCLC = 0001000;
constexpr static const u64 IXON = 0002000;  /* Enable start/stop output control.  */
constexpr static const u64 IXANY = 0004000; /* Enable any character to restart output.  */
constexpr static const u64 IXOFF = 0010000; /* Enable start/stop input control.  */
constexpr static const u64 IMAXBEL = 0020000;
constexpr static const u64 IUTF8 = 0040000; /* Input is UTF8 (not in POSIX).  */

constexpr static const u64 CSIZE = 0000060;
constexpr static const u64 CS5 = 0000000;
constexpr static const u64 CS6 = 0000020;
constexpr static const u64 CS7 = 0000040;
constexpr static const u64 CS8 = 0000060;
constexpr static const u64 CSTOPB = 0000100;
constexpr static const u64 CREAD = 0000200;
constexpr static const u64 PARENB = 0000400;
constexpr static const u64 PARODD = 0001000;
constexpr static const u64 HUPCL = 0002000;
constexpr static const u64 CLOCAL = 0004000;

constexpr static const u64 B0 = 0000000;
constexpr static const u64 B50 = 0000001;
constexpr static const u64 B75 = 0000002;
constexpr static const u64 B110 = 0000003;
constexpr static const u64 B134 = 0000004;
constexpr static const u64 B150 = 0000005;
constexpr static const u64 B200 = 0000006;
constexpr static const u64 B300 = 0000007;
constexpr static const u64 B600 = 0000010;
constexpr static const u64 B1200 = 0000011;
constexpr static const u64 B1800 = 0000012;
constexpr static const u64 B2400 = 0000013;
constexpr static const u64 B4800 = 0000014;
constexpr static const u64 B9600 = 0000015;
constexpr static const u64 B19200 = 0000016;
constexpr static const u64 B38400 = 0000017;

template <int Fd = 1>     // stdin
int
terminal_setattr(const termios_t &buf)
{
  // TODO: implement proper ioctl lookup fn
  if ( micron::ioctl(Fd, micron::tcsets, &buf) < 0 )
    return -1;
  return 0;
}

template <int Fd = 1>     // stdin
int
terminal_getattr(termios_t &buf)
{
  // TODO: implement proper ioctl lookup fn
  if ( micron::ioctl(Fd, micron::tcgets, &buf) < 0 )
    return -1;
  return 0;
}
};     // namespace posix

};     // namespace micron
