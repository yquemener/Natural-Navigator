from cflib.crazyflie import Crazyflie
import cflib.crtp
from time import sleep, time
import traceback, sys
from subprocess import Popen, PIPE
from threading  import Thread


try:
  from Queue import Queue, Empty
except ImportError:
  from queue import Queue, Empty  # python 3.x
    
ON_POSIX = 'posix' in sys.builtin_module_names

def enqueue_output(out, queue):
  for line in iter(out.readline, b''):
    queue.put(line)
  out.close()
    
    
cflib.crtp.init_drivers()
available = cflib.crtp.scan_interfaces()

cr = Crazyflie()
cr.open_link("radio://0/10/250K")
pr = None
try:
  pr = Popen(["/home/data/Projects/active/quadcopter/crazyflie/naturalnavigator/qt-build/NaturalNavigator"], stdout=PIPE, bufsize=1, close_fds=ON_POSIX)
  q = Queue()
  t = Thread(target=enqueue_output, args=(pr.stdout, q)   )
  t.daemon = True # thread dies with the program
  t.start()    
  tinit = time()
  start_fly=False
  exec(open("dynpid.py","r"))
  dynpid = Controller(cr.commander)
    
  while pr.poll()==None:
    try:  
      s = q.get_nowait()
      if s!="":
	s=s.rstrip("\n").rstrip("\r").rstrip("\n")
	if s=="START":
	  exec(open("dynpid.py","r"))
	  try:
	    exec(open("dynpid.py","r"))
	  except:
	    print "Error"
	    exc_type, exc_value, exc_traceback = sys.exc_info()
	    print exc_type
	    print exc_value
	    traceback.print_tb(exc_traceback)
	    raise Exception("dynpid failed to load")
	    
	  dynpid = Controller(cr.commander)
	  dynpid.start()
	elif s=="STOP":
	  dynpid.stop()
	else:
	  start_fly=False
	  if "\t" in s:
	    (x,y,z,sx,sy,sz) = s.split("\t")
	    z = float(z)
	    y = float(y)
	    x = float(x)
	    sx = float(sx)
	    sy = float(sy)
	    sz = float(sz)
	    dynpid.update_position(x, y, z, sx, sy, sz)
	  
    except Empty:
      dynpid.update()
  cr.close_link()

except:
  print "Error"
  exc_type, exc_value, exc_traceback = sys.exc_info()
  print exc_type
  print exc_value
  traceback.print_tb(exc_traceback)
  cr.commander.send_setpoint(0, 0, 0, 0)
  sleep(2)
  cr.close_link()
  pr.kill()
  pr.wait()
