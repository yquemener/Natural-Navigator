""" This is a simple PID control. It is designed to be called dynamically by an "exec" statement
so that it is possible to modify it without restarting the whole program"""

# This is a basic one-dimension PID 
class PID:
  def __init__(self, pos, target, Kp, Ki, Kd, neutral=0, debug=False):
    self.target = target
    self.pos = pos
    self.speed = 0
    self.integ = 0
    self.lasterror = self.target - self.pos
    self.Ki = Ki
    self.Kd = Kd
    self.Kp = Kp
    self.neutral = neutral
    self.debug=debug
    
    
  def update(self, newpos ,dt):
    
    dt = float(dt)
    if dt < 0.01:
      dt = 0.01
    self.speed = (newpos - self.pos)/dt
    self.pos = newpos
    errorp = self.target - self.pos
    errori = self.integ + errorp*dt
    errord = (self.lasterror - errorp)/dt
    self.lasterror = errorp
    self.integ = errori
    
    self.output = self.Kp*errorp + self.Kd*errord + self.Ki*errori
    if self.debug:
      color=""
      if self.output>0:
	color = '\033[92m'
      else:
	color = '\033[94m'
      print color + str(self.output) , "=", self.Kp,"*", errorp,"+", self.Kd,"*", errord,"+", self.Ki,"*",errori,'\033[0m'
    
    return self.neutral + self.output

# This is a 3 dimensions PID controller with a start sequence. We are trying to find an appropriate thrust, roll and 
# pitch in order to reach the target and maintain a speed of zero in x, y and z

class Controller:
  def __init__(self, commander):
    self.target = [413, 215, 190]
    self.calib_roll = 1.5
    self.calib_pitch = -5.0
    self.calib_thrust = 38000
    
    # Speed at which we increase thrust (per sec) for take off
    self.start_ramp = 1000.0
    
    # Max expected interval (in seconds) between 2 kinect position updates
    self.expecttime = 5.0 
    
    self.starttime = time()
    self.lasttime = time()
    self.sequence = "off"
    
    #crazyflie controller
    self.commander = commander
    
    # commands
    self.cmd_yawrate = 0
    self.cmd_roll = self.calib_roll
    self.cmd_pitch = self.calib_pitch
    self.cmd_thrust = 0
    
    # PIDs
    self.pid_thrust = None
    self.pid_roll = None
    self.pid_pitch = None
    
    
    
  def start(self):
    self.starttime = time()
    self.lasttime = time()
    self.sequence = "start"
    self.cmd_roll = self.calib_roll
    self.cmd_pitch = self.calib_pitch 
    self.cmd_thrust = self.calib_thrust
    

  def stop(self):
    self.sequence = "off"
    self.cmd_yawrate = 0
    self.cmd_roll = self.calib_roll
    self.cmd_pitch = self.calib_pitch
    self.cmd_thrust = 0
    self.commander.send_setpoint(0, 0, 0, 0)

  def update_position(self, x,y,z,sx,sy,sz):
    self.pos = (x,y,z)
    print self.calib_thrust, x, y, z
    if(self.sequence == "start"):
      self.sequence = "track"
      print "Start tracking"
      # Pure P:
      #self.pid_thrust = PID(z,self.target[2], -250, 0, 0, neutral = self.cmd_thrust) 
      self.calib_thrust = self.cmd_thrust
      self.pid_thrust = PID(z,self.target[2], -145, -60, 0, neutral = self.cmd_thrust, debug=True) 
      self.pid_pitch = PID(y,y, 0.02, 0.00, 0, neutral = self.calib_pitch) 
      self.pid_roll = PID(x,x, -0.02, -0.00, 0, neutral=self.calib_roll, debug=False) 
      self.lasttime = time() 
    if self.sequence == "track":
      dt = time() - self.lasttime
      self.cmd_thrust = max(min(self.pid_thrust.update(z,dt),50000), 0)
      #self.cmd_pitch = self.calib_pitch
      #self.cmd_roll = self.calib_roll
      self.cmd_pitch = self.pid_pitch.update(y,dt)
      self.cmd_roll = self.pid_roll.update(x,dt)
      
      #print self.sequence, self.cmd_roll, self.cmd_pitch, self.cmd_yawrate, int(self.cmd_thrust)
    self.lasttime = time() 

  def update(self):
    if(self.sequence == "start"):
      ct = time()
      dt = ct - self.lasttime
      self.lasttime = ct
      self.cmd_thrust += self.start_ramp * dt
    elif self.sequence == "track":
      if time()-self.lasttime > self.expecttime:
	print "Lost tracking for too long"
	self.stop()
      
    #print self.sequence, self.cmd_roll, self.cmd_pitch, self.cmd_yawrate, int(self.cmd_thrust)
    self.commander.send_setpoint(self.cmd_roll, self.cmd_pitch, self.cmd_yawrate, int(self.cmd_thrust))
    
    