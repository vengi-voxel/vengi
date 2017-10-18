# Savable

To collect sql update statements for changed player data, every handler should register itself
somewhere in a way that the handler (e.g. the CooldownMgr or AttribMgr) can answer the question
whether there is something to persist.
Now consider that there are 1000 players logged in and they all have there CooldownMgr instance.
The "collector-instance" would query all of them and asks them whether they are dirty or not. They
collect the data and build a mass update statement. This might happen every minute or something like
that.
