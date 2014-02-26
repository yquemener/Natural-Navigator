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


def main():
  roll    = 2.5
  pitch   = -3.0
  yawrate = 0
  thrust  = 0 # Thrust is an integer value ranging from 10001 (next to no power) to 60000 (full power).
  cr.commander.send_setpoint(roll, pitch, yawrate, thrust)
  pr = Popen(["/home/data/Projects/active/quadcopter/crazyflie/naturalnavigator/qt-build/NaturalNavigator"], stdout=PIPE, bufsize=1, close_fds=ON_POSIX)
  q = Queue()
  t = Thread(target=enqueue_output, args=(pr.stdout, q)   )
  t.daemon = True # thread dies with the program
  t.start()    
  tinit = time()
  start_fly=False
  
  roll_calib = -1.5
  pitch_calib = -3.0
  target_x = 413
  target_y = 215
  target_z = 200
  thrust_str = 43000
  amount = 0
  
  while pr.poll()==None:
    try:  
      s = q.get_nowait()
      if s!="":
	s=s.rstrip("\n").rstrip("\r").rstrip("\n")
	if s=="START":
	  start_fly=True
	  thrust = thrust_str
	  cr.commander.send_setpoint(roll, pitch, yawrate, thrust)
	elif s=="STOP":
	  start_fly=False
	  thrust = 0
	  cr.commander.send_setpoint(roll, pitch, yawrate, thrust)
	  #print "Stop NOW!"
	else:
	  start_fly=False
	  (x,y,z,sx,sy,sz) = s.split("\t")
	  z = float(z)
	  y = float(y)
	  x = float(x)
	  dx = min(abs(x - target_x)/1.0, amount)
	  dy = min(abs(y - target_y)/1.0, amount)
	  
	  if z<target_z:
	    thrust = int(0.99*thrust_str)
	  if z>target_z:
	    thrust = thrust_str
	  if y>target_y:
	    pitch = pitch_calib - dy
	  else:
	    pitch = pitch_calib + dy
	  if x>target_x:
	    roll = roll_calib + dx
	  else:
	    roll = roll_calib - dx
	  print roll, pitch, thrust, x-target_x, y-target_y, z-target_z
	  roll = int(roll)
	  pitch = int(pitch)
	  cr.commander.send_setpoint(roll, pitch, yawrate, thrust)
	  #print "Thrust set to:" + str(thrust)
    except Empty:
      cr.commander.send_setpoint(roll, pitch, yawrate, thrust)
  cr.close_link()

try:
  main()
except:
  print "Error"
  exc_type, exc_value, exc_traceback = sys.exc_info()
  print exc_type
  print exc_value
  traceback.print_tb(exc_traceback)
  cr.commander.send_setpoint(0, 0, 0, 0)
  sleep(2)
  cr.close_link()
    