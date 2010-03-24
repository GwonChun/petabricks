#!/usr/bin/python

import subprocess, sys, re, os, signal

if len(sys.argv)!=2:
  print "USAGE: checkmaxima /usr/bin/maxima"
  sys.exit(1)

errorMsg = '''
ERROR: Problem with 'maxima' on this system:
         %s

       Try downgrading to maxima 5.13, which can be found here:
         - http://packages.debian.org/lenny/maxima
         - http://packages.debian.org/lenny/maxima-share
       for debian based systems.

       There is a bug in maxima 5.15 to 5.20.1 that prevent it from working
       with petabricks. These bugs have been fixed in maxima svn.
'''

def die(msg):
  sys.stderr.write(errorMsg % msg)
  sys.exit(1)


p=subprocess.Popen([sys.argv[1], "--version"], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
ver = p.communicate()[0]
ver = re.search("([0-9]+.[.0-9]+)", ver).group(1)

p=subprocess.Popen([sys.argv[1], "-q","--disable-readline"], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
signal.signal(signal.SIGALRM, lambda signum, frame: os.kill(p.pid, signal.SIGKILL))
signal.alarm(3)
o=p.communicate('''
  load(ineq)$

  assume(x<=0)$
  test1rslt = is(equal(x,0));

  assume(equal(x,i))$
  test2rslt = diff(x+1,x);
''')[0]
signal.alarm(0)

def findResult(name):
  r=re.compile("[^a-zA-Z]"+re.escape(name)+" *= *([a-zA-Z0-9]+)")
  m=r.search(o)
  if m:
    return m.group(1)
  else:
    die("no output produced")

if findResult("test1rslt") != "unknown":
  die("ineq assume()/is() broken")

if p.returncode!=0 or findResult("test2rslt") != "1":
  die("maxima crash bug (http://sourceforge.net/tracker/?func=detail&atid=104933&aid=2938078&group_id=4933)")

if "Could not find `ineq'" in o:
  die("maxima-share is not installed (load(ineq) failed)")


