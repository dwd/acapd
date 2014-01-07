acapd
=====

Infotrope ACAP Server

(This README written 7th Jan, 2014)

This is work that I did about a decade ago. As such, it's littered with an
interesting combination of archeology (it's got its own shared/weak pointer
implementation, because Boost's either wasn't up to it at the time, or else
I didn't know it was), and poor design (spawning threads for every command??
what was I *thinking*?).

Possibly the worst part of it is that the bootstrap process is so awful. I'd
never consider running without an external configuration file now, but at the
time, I thought that a configuration server ought to store its own
configuration.

An ACAP server, by the way, is a configuration server. We'd probably call it
a NoSQL cloud-enabled mobile-centric database now, but back in 1997, it was
a configuration server. Email clients - because there were no other clients
worth speaking of - would connect to them to fetch their configuration.

ACAP died a death of a thousand people who could't be bothered implementing it
for a number of reasons. Firstly, it's murderously hard to implement. Just
reading the source code while I made it compile on a recent Linux made my eyes
water. I was clearly insane for deciding to do this. Just the code surrounding
virtual datasets makes me reach for a stiff drink.

Secondly, the email community, even a decade ago, was largely gone. Introducing
radical new ideas about how email clients would be self-configuring, and
defining what configuration would be standardized, was a tough sell - and more
than likely, most implementors weren't ever aware of ACAP.

But for all this, I'm rather proud of this code - warts and all. Back when I
wrote it, in my late 20s, it was by far the largest thing I'd written from
scratch, and although I don't doubt I could do a much better job now, the fact
it runs, and is (as far as I can recall) conformant to the spec, is a pretty
good achievement. And ACAP remains pretty handy, for the most part.

So while I look at parts and wince slightly, and wonder why I embarked on this
at all, another voice in my head says "Oh, just change those Infotrope::Utils::magic_ptr's to a nice std::shared_ptr while you're here, and maybe back the data onto sqlite instead".

And maybe, one day...
