# mbzm - eMBedded ZModem or something...

A small, simple, modern(!) and incomplete Zmodem implementation, for use in resource-constrained 
environments, or wherever you can find a use for it really.

## Zmodem? Really??

Yes, I am aware that it is the twenty-first century, and Zmodem is hardly the state of the art.
I wrote this for two reasons:

* I wanted a fun, period-appropriate protocol I could use to load 
programs into my retro computer (https://github.com/roscopeco/rosco_m68k) without
having to pull the ROM chips for each iteration. 

* I haven't implemented Zmodem before and wanted to explore how it worked.

As far as possible, I avoided referring to other Zmodem implementations and just tried to
implement this using the spec (http://pauillac.inria.fr/~doligez/zmodem/zmodem.txt).
The spec is a bit vague on certain things so I spent quite a bit of time reading old
usenet and forum posts, experimenting, and occasionally peeking at the original `zm.c`
implementation to see how things were supposed to work.

## Features/Limitations

Right now, this is very limited. For one thing, it can only receive. Expanding it to support
sending probably wouldn't be all that much work, but I don't need it right now so I haven't
done it. Other notable things:

* It doesn't do any dynamic memory allocation, so it can be used where malloc is unavailable.
* It's sort-of optimised for use in 16/32-bit environments (I wrote it with M68010 as the primary target) 
* It doesn't support the (optional) ZSINIT frame and will just ignore it
* It doesn't support **any** of the advanced features of the protocol (compression etc).
* It doesn't support resume
* It doesn't work with XON/XOFF flow control
* It has a ton of other limitations I'm too lazy to list right now...
* ... but it does work for the simple case of receiving data with error correction.

## Usage

### Tests

There are some unit tests included. They don't cover everything, but they're a start, and
a definite improvement over the other 30-year-old code out there.

`make test`

### Sample application

A sample application is included that will receive a file. You can use `sz` or `minicom` or
something to send a file, probably using `socat` or similar to set up a virtual link.

E.g. start socat:

`socat -d -d pty,raw,echo=0 pty,raw,echo=0`

You'll probably need to change the source to open the correct device.

To build the application, just do:

`make`

and then run it:

`./rz`


### Use as a library

To use, you'll need to implement two functions in your code:

```c
uint16_t recv();
uint16_t send(uint8_t chr);
```

These should return one of the codes used in the rest of the library
to indicate an error if one occurs. 

If there isn't a problem, `recv` should return the next byte from the serial
link. `send` should return `OK`.

#### Cross-compiling

If using this as part of a larger project, you'll probably want to just pull
the source into your existing build. If, for some reason, you just want to
build the objects for m68k, and you have a cross-compiler, you can do:

`make -f Makefile.m68k`

I use this mostly for testing that changes will still build using my cross
toolchain - it probably isn't actually useful for anything... 

## Shoutouts

The CRC calculation code comes from the excellent **libcrc** library
(https://github.com/lammertb/libcrc), because that code works well and
I have zero interest in calculating CRCs myself.
