#!/usr/bin/python
from Tkinter import *
from PIL import Image, ImageTk

import subprocess
import locale
import os
import time
import thread
from collections import deque

import paramiko
import memcache

# NUMS times WPT should be 600 for the scale to work out
NUMS = 200    # number of values to show in strip
WPT = 3       # width of each data point in plot
PMRG = 60     # margin beside chart
PWIDTH = 722  # left margin is 30, right margin is 30
SHGT = 380    # strip chart pane height

CWIDTH = 250    # width of stats column
DHGT = 380    # stats dump pane height
SLHGT = 15    # line height for stats
INTERVAL = 1000

CTWIDTH = 276
CLWIDTH = 344
CMWIDTH = 108
CHGT = 230    # height of load control frames
CLICOL = 3    # number of columns for client icons
CLIROW = 8    # number of rows for client icons

LSAMP = 10    # number of samples in moving average
LINTVL = 100    # number of seconds between latency samples

NSAMP = 20    # number of samples to take before writing an average record
WARM = 5     # number of samples to skip before starting AUTO test
COOL = 5     # number of samples to skip before restarting AUTO test

MULTIKEY = [ "000","001","002","003","004","005","006","007","008","009",\
             "010","011","012","013","014","015","016","017","018","019",\
             "020","021","022","023","024","025","026","027","028","029",\
             "030","031","032","033","034","035","036","037","038","039",\
             "040","041","042","043","044","045","046","047","048","049",\
             "050","051","052","053","054","055","056","057","058","059",\
             "060","061","062","063"]
MULTIOBJ = [ "r000","r001","r002","r003","r004","r005","r006","r007","r008","r009",\
             "r010","r011","r012","r013","r014","r015","r016","r017","r018","r019",\
             "r020","r021","r022","r023","r024","r025","r026","r027","r028","r029",\
             "r030","r031","r032","r033","r034","r035","r036","r037","r038","r039",\
             "r040","r041","r042","r043","r044","r045","r046","r047","r048","r049",\
             "r050","r051","r052","r053","r054","r055","r056","r057","r058","r059",\
             "r060","r061","r062","r063"]


# defaults for loadgen
MINthr = 1
MAXthr = 16
CLThr = 16     # threads per client
MINcon = 1
MAXcon = 64
CLCon = 8   # connections per thread
MINgpr = 1
MAXgpr = 48
CLgpr = 32    # gets per network packet
MINdel = 1
MAXdel = 100000
CLdelay = 50  # delay between requests
MINkey = 8
MAXkey = 64
CLkey = 8
MINval = 8
MAXval = 128
CLval = 32

FONT = "Liberation Mono"
CHFNTSIZ = "12"
BGCLR = "dark slate gray"
CHCLR = "white"
RELF = "ridge"

class mcLat:
# object to collect a moving average of latency
  def __init__(self,master,mc):
    global LATMULTI
    self.mymaster = master
# load data for latency measurements
    self.mc = mc
    if LATMULTI:
      for i in range(len(MULTIKEY)):
        mc.set(MULTIKEY[i],MULTIOBJ[i])
    else:
        mc.set(MULTIKEY[0],MULTIOBJ[0])

# prime latency moving average
    stime = time.time()
    if LATMULTI:
      obj = self.mc.get_multi(MULTIKEY)
    else:
      obj = self.mc.get(MULTIKEY[0])
    elmic = int(1000000*(time.time() - stime))

    self.latsamp = deque()
    for i in range(0,LSAMP):
      self.latsamp.append(elmic)

    self.mymaster.after(LINTVL,self.sample)
    
  def sample(self):
    global LATMULTI
    global LOADGEN
    global gprEntry
# time single query
    elmic = 0
    if LOADGEN:
      gprm1 = int(gprEntry.get())
      key = MULTIKEY[0:gprm1]
    else:
      key = MULTIKEY

    stime = time.time()
    if LATMULTI:
      obj = self.mc.get_multi(key)
    else:
      obj = self.mc.get(MULTIKEY[0])
    elmic = int(1000000*(time.time() - stime))
#    print "latency sample = ",elmic

#    self.il = self.il + 1
#    if self.il >= LSAMP:
#      self.il = 0
#    self.latsamp[self.il] = elmic
    toss = self.latsamp.popleft()
    self.latsamp.append(elmic)

    self.mymaster.after(LINTVL,self.sample)
    
  def get(self):
    lsum = 0
    for i in range(len(self.latsamp)):
      lsum = lsum + self.latsamp[i]
    return lsum/LSAMP

class mcStats:
  def __init__(self,master,SYSTEM,LATENCY,MISSES,STATS,SHOWAVG,RECFILE,TRACEFILE,AUTOFILE):
    global SVRVER
    global AUTO
    global getque
    global getpsque
    global setque
    global setpsque
    global gmissque
    global gmisspsque
    global latque
    global il
    global lasttime
    global samples
    global settot
    global gettot
    global gmisstot
    global lattot
    global warmcnt
    global WARMING
    global coolcnt
    global COOLING
    global autoparms

    getque = deque()
    getpsque = deque()
    gmissque = deque()
    gmisspsque = deque()
    setque = deque()
    setpsque = deque()
    latque = deque()
    self.resetAvg()
    if RECORD:
      rfile=open(RECFILE,"a")
      rec = "clientsys,total_conn,protocol,threads,conn,gets_per_frame,delay,keysize,valsize,numsamp,avgsetps,avggetps"
      if MISSES:
        rec = rec + ",avgmissps"
      if LATENCY:
        rec = rec + ",avglatency"
      rfile.write(rec + "\n")
      rfile.close()

    if TRACE:
      tfile=open(TRACEFILE,"a")
      rec = "setps,getps"
      if MISSES:
        rec = rec + ",missps"
      if LATENCY:
        rec = rec + ",latency"
      tfile.write(rec + "\n")
      tfile.close()

    self.master=master
    self.chart = Canvas(master,width=PWIDTH,height=SHGT,bg=BGCLR,bd=3,relief=RELF)
    self.chart.pack()
    if STATS:
      self.grid = Canvas(master,width=PWIDTH,height=DHGT,bd=3,bg='white',relief=RELF)
      self.grid.pack()

# connect to memcached server
    print "connecting to ",SYSTEM
    self.mc = memcache.Client([SYSTEM],debug=1)
    print self.mc

# initialize strip chart data queues
    lasttime = -1.0
# prime with current counts
    data = self.mc.get_stats()
    if(len(data) <= 0):
      print "couldn't get stats"
      exit(1)
    stats = data[0][1]
    numget = int(stats['cmd_get'])
    numset = int(stats['cmd_set'])
    nummiss = int(stats['get_misses'])
    for x in range(NUMS):
      getque.append(numget)
      getpsque.append(0)
      gmissque.append(nummiss)
      gmisspsque.append(0)
      setque.append(numset)
      setpsque.append(0)
      if LATENCY:
        latque.append(0)

    SVRVER = stats['version']
    
# if LATENCY create latency object
    if LATENCY:
      self.lat = mcLat(master,self.mc)

    WARMING = True  # signifies warmup period of test
    COOLING = False  # signifies cooling period of test
    warmcnt = 0
    coolcnt = 0
    self.refresh()

  def refresh(self):
    global RECORD
    global AUTO
    global COOL
    global COOLING
    global coolcnt
    global WARM
    global WARMING
    global warmcnt
    global autoparms
    global active
    global totconn
    global g_thr
    global g_conn
    global g_gpr
    global g_delay
    global g_keysize
    global g_valsize
    global g_protocol

# get stats from server
    data = self.mc.get_stats()
    if len(data) > 0:
      stats = data[0][1]
      numget = int(stats['cmd_get'])
      numset = int(stats['cmd_set'])
      nummiss = int(stats['get_misses'])
      if LATENCY:
        latency = self.lat.get()
      else:
        latency = 0
      self.drawchart(numget,numset,nummiss,latency)
      if STATS:
        self.dumpstats(data)
      if AUTO:
        if WARMING:
          print "warmcnt cycle ",warmcnt
          self.resetAvg()
          warmcnt = warmcnt + 1
          if warmcnt > WARM:
            WARMING = False
        elif COOLING:
          print "cooldown cycle ",coolcnt
          self.resetAvg()
          coolcnt = coolcnt + 1
          if coolcnt > COOL:
            COOLING = False
            if len(autoparms) < 1:
              print "auto test complete"
              AUTO = False
              controller.stopAndExit()
            else:
              ptext = autoparms.popleft().rstrip()
              params = ptext.split(",")
              print "starting test with params ",params
              iclients = int(params[0])
              if iclients > nclients:
                print "requested ",iclients," clients, ",nclients," available"
                iclients = nclients
              ithreads = int(params[1])
              iconn = int(params[2])
              igetperreq = int(params[3])
              idelay = int(params[4])
              ikeysize = int(params[5])
              imode = params[6]
              controller.setParams(ithreads,iconn,igetperreq,idelay,ikeysize,imode)
              controller.startN(iclients)
              warmcnt = 0
              coolcnt = 0
              WARMING = True
        elif samples == NSAMP:
          print "finished a sample set, writing data"
          rfile=open(RECFILE,"a")
          rec = "%d" % active
          rec = rec + ",%d" % totconn
          rec = rec + ",%s" % g_protocol
          rec = rec + ",%d" % g_thr
          rec = rec + ",%d" % g_conn
          rec = rec + ",%d" % g_gpr
          rec = rec + ",%d" % g_delay
          rec = rec + ",%d" % g_keysize
          rec = rec + ",%d" % g_valsize
          rec = rec + ",%d" % samples
          rec = rec + ",%d" % setavg
          rec = rec + ",%d" % getavg
          if MISSES:
            rec = rec + ",%d" % gmissavg
          if LATENCY:
            rec = rec + ",%d" % latavg
          rfile.write(rec + "\n")
          rfile.close()
          controller.stopAll()
          self.resetAvg()
          COOLING = True
      elif RECORD:
        if samples == NSAMP:
          print "finished a sample set, writing data"
          rfile=open(RECFILE,"a")
          rec = "%d" % active
          rec = rec + ",%d" % totconn
          rec = rec + ",%s" % g_protocol
          rec = rec + ",%d" % g_thr
          rec = rec + ",%d" % g_conn
          rec = rec + ",%d" % g_gpr
          rec = rec + ",%d" % g_delay
          rec = rec + ",%d" % g_keysize
          rec = rec + ",%d" % g_valsize
          rec = rec + ",%d" % samples
          rec = rec + ",%d" % setavg
          rec = rec + ",%d" % getavg
          if MISSES:
            rec = rec + ",%d" % gmissavg
          if LATENCY:
            rec = rec + ",%d" % latavg
          rfile.write(rec + "\n")
          rfile.close()



# reschedule sampler
    self.master.after(INTERVAL,self.refresh)

  def drawchart(self,numget,numset,nummiss,latency):
    global gmissque
    global gmisspsque
    global getque
    global setque
    global lasttime
    global samples
    global settot
    global setavg
    global gettot
    global getavg
    global gmisstot
    global lattot
    global active
    global totconn
    

    curtime = time.time()

    self.chart.delete("MCCHART")
    CLFT = PMRG
    CRGT = PWIDTH-PMRG
    CTOP = 5+SHGT/10
    CBOT = SHGT-5
    YRNG = CBOT-CTOP
    NMAJ = 7
    YMAJ = YRNG/NMAJ
    YMNR = YMAJ/2
    YSC = 35000000
    LSC = 7000

# draw chart
    self.chart.create_line(CLFT,CBOT,CRGT,CBOT,fill=CHCLR,tags="MCCHART")
#    self.chart.create_line(CLFT,CTOP,CRGT,CTOP,fill=CHCLR,tags="MCCHART")
    self.chart.create_line(CLFT,CBOT,CLFT,CTOP,fill=CHCLR,tags="MCCHART")
    self.chart.create_line(CRGT,CBOT,CRGT,CTOP,fill=CHCLR,tags="MCCHART")
    ly = CBOT - YMAJ
    val = 0
    lval = 0
    while ly > CTOP:
      self.chart.create_line(CLFT,ly,CRGT,ly,fill=CHCLR,dash=(3,3),tags="MCCHART")
      val = val + (YSC/NMAJ)/1000000
      self.chart.create_text(CLFT,ly,anchor=E,text=str(val) + 'M',fill=CHCLR,tags="MCCHART")
      if LATENCY:
        lval = lval + (LSC/NMAJ)/1000
        self.chart.create_text(CRGT,ly,anchor=W,text=str(lval) + 'ms',fill=CHCLR,tags="MCCHART")
      ly = ly - YMAJ
    self.chart.create_text(PWIDTH/2,CTOP,anchor=S,text="memcached: " + SVRVER + "  server: " + HOSTNAME,fill='white',font=(FONT,"20","bold italic"),tags="MCCHART")

    if lasttime > 0.0:
      deltime = curtime - lasttime

      getpscur = int((numget-getque[-1])/deltime)
      getque.append(numget)
      getpsque.append(getpscur)
      setpscur = int((numset-setque[-1])/deltime)
      setque.append(numset)
      setpsque.append(setpscur)
      gmisspscur = int((nummiss-gmissque[-1])/deltime)
      gmissque.append(nummiss)
      gmisspsque.append(gmisspscur)
      if LATENCY:
        latque.append(latency)

      if TRACE:
        tfile=open(TRACEFILE,"a")
        rec = "%d" % setpscur
        rec = rec + ",%d" % getpscur
        if MISSES:
          rec = rec + ",%d" % gmisspscur
        if LATENCY:
          rec = rec + ",%d" % latency
        tfile.write(rec + "\n")
        tfile.close()

      csetps = locale.format("%d",setpscur,grouping=True)
      cgetps = locale.format("%d",getpscur,grouping=True)
      cgmissps = locale.format("%d",gmisspscur,grouping=True)
      if LATENCY:
        clat = locale.format("%d",latque[-1],grouping=True)
        
      ypos = CTOP+2
      self.chart.create_text(CLFT+2,ypos,anchor=NW,text='sets per second',fill='green',font=(FONT,CHFNTSIZ),tags="MCCHART")
      self.chart.create_text(CLFT+300,ypos,anchor=NE,text=csetps,fill='green',font=(FONT,CHFNTSIZ,"bold"),tags="MCCHART")
      ypos = ypos + 16
      self.chart.create_text(CLFT+2,ypos,anchor=NW,text='gets per second',fill='cyan',font=(FONT,CHFNTSIZ),tags="MCCHART")
      self.chart.create_text(CLFT+300,ypos,anchor=NE,text=cgetps,fill='cyan',font=(FONT,CHFNTSIZ,"bold"),tags="MCCHART")
      if MISSES:
        ypos = ypos + 16
        self.chart.create_text(CLFT+2,ypos,anchor=NW,text='misses per second',fill='red',font=(FONT,CHFNTSIZ),tags="MCCHART")
        self.chart.create_text(CLFT+300,ypos,anchor=NE,text=cgmissps,fill='red',font=(FONT,CHFNTSIZ,"bold"),tags="MCCHART")
      if LATENCY:
        ypos = ypos + 16
        self.chart.create_text(CLFT+2,ypos,anchor=NW,text='latency (mic)',fill='yellow',font=(FONT,CHFNTSIZ),tags="MCCHART")
        self.chart.create_text(CLFT+300,ypos,anchor=NE,text=clat,fill='yellow',font=(FONT,CHFNTSIZ,"bold"),tags="MCCHART")
 
      if SHOWAVG:
        self.updateAvg(setpscur,getpscur,gmisspscur,latency)
        cavgsamp = locale.format("%d",samples,grouping=True)
        csetps = locale.format("%d",setavg,grouping=True)
        cgetps = locale.format("%d",getavg,grouping=True)
        if MISSES:
          cgmissps = locale.format("%d",gmissavg,grouping=True)
        if LATENCY:
          clat = locale.format("%d",latavg,grouping=True)
        
        ypos = CTOP+2
        self.chart.create_text(CLFT+352,ypos,anchor=NW,text='avg',fill='green',font=(FONT,CHFNTSIZ),tags="MCCHART")
        self.chart.create_text(CLFT+500,ypos,anchor=NE,text=csetps,fill='green',font=(FONT,CHFNTSIZ,"bold"),tags="MCCHART")
        ypos = ypos + 16
        self.chart.create_text(CLFT+352,ypos,anchor=NW,text='avg',fill='cyan',font=(FONT,CHFNTSIZ),tags="MCCHART")
        self.chart.create_text(CLFT+500,ypos,anchor=NE,text=cgetps,fill='cyan',font=(FONT,CHFNTSIZ,"bold"),tags="MCCHART")
        if MISSES:
          ypos = ypos + 16
          self.chart.create_text(CLFT+352,ypos,anchor=NW,text='avg',fill='red',font=(FONT,CHFNTSIZ),tags="MCCHART")
          self.chart.create_text(CLFT+500,ypos,anchor=NE,text=cgmissps,fill='red',font=(FONT,CHFNTSIZ,"bold"),tags="MCCHART")
        if LATENCY:
          ypos = ypos + 16
          self.chart.create_text(CLFT+352,ypos,anchor=NW,text='avg',fill='yellow',font=(FONT,CHFNTSIZ),tags="MCCHART")
          self.chart.create_text(CLFT+500,ypos,anchor=NE,text=clat,fill='yellow',font=(FONT,CHFNTSIZ,"bold"),tags="MCCHART")
        ypos = ypos + 16
        self.chart.create_text(CLFT+352,ypos,anchor=NW,text='samples',fill='white',font=(FONT,CHFNTSIZ),tags="MCCHART")
        self.chart.create_text(CLFT+500,ypos,anchor=NE,text=cavgsamp,fill='white',font=(FONT,CHFNTSIZ),tags="MCCHART")
      delx = WPT
      x1 = CLFT
      for x in range(NUMS):
        x2 = x1 + WPT
        y1 = CBOT - (YRNG*getpsque[x])/YSC
        y2 = CBOT - (YRNG*getpsque[x+1])/YSC
        self.chart.create_line(x1,y1,x2,y2,fill="cyan",width=2.0,tags="MCCHART")
        y1 = CBOT - (YRNG*setpsque[x])/YSC
        y2 = CBOT - (YRNG*setpsque[x+1])/YSC
        self.chart.create_line(x1,y1,x2,y2,fill="green",width=2.0,tags="MCCHART")
        if MISSES:
          y1 = CBOT - (YRNG*gmisspsque[x])/YSC
          y2 = CBOT - (YRNG*gmisspsque[x+1])/YSC
          self.chart.create_line(x1,y1,x2,y2,fill="red",width=2.0,tags="MCCHART")
        if LATENCY:
          y1 = CBOT - (YRNG*latque[x])/LSC
          y2 = CBOT - (YRNG*latque[x+1])/LSC
          self.chart.create_line(x1,y1,x2,y2,fill="yellow",width=2.0,tags="MCCHART")
        x1 = x2

      getque.popleft()
      getpsque.popleft()
      setque.popleft()
      setpsque.popleft()
      if MISSES:
        gmissque.popleft()
        gmisspsque.popleft()
      if LATENCY:
        latque.popleft()

    lasttime = curtime

  def resetAvg(self):
    global samples
    global settot
    global gettot
    global gmisstot
    global lattot
    samples = 0
    settot = 0
    gettot = 0
    gmisstot = 0
    lattot = 0

  def updateAvg(self,setpscur,getpscur,gmisspscur,latency):
    global samples
    global settot
    global gettot
    global gmisstot
    global lattot
    global setavg
    global getavg
    global gmissavg
    global latavg

    samples = samples + 1
    settot = settot + setpscur
    setavg = settot/samples
    gettot = gettot + getpscur
    getavg = gettot/samples
    gmisstot = gmisstot + gmisspscur
    gmissavg = gmisstot/samples
    lattot = lattot + latency
    latavg = lattot/samples

  def dumpstats(self,data):
# pane size is a minimum of 200
    NP = PWIDTH/CWIDTH    # how many panes can I fit in the panel width?
    PANE = PWIDTH/(NP)  # expand pane size so the columns are all the same size
    xpos = 0
    ypos = 15

    self.grid.delete("MCINFO")
    sysid = data[0][0]
    self.grid.create_text(xpos+5,ypos,anchor=W,text='address:port',fill='blue',font=(FONT,CHFNTSIZ),tags="MCINFO")
    self.grid.create_text(xpos+PANE-5,ypos,anchor=E,text=sysid,fill='blue',font=(FONT,CHFNTSIZ),tags="MCINFO")
    ypos = ypos + SLHGT

    stats = data[0][1]
    for name in sorted(stats.iterkeys()):
      self.grid.create_text(xpos+5,ypos,anchor=W,text=name,fill='black',font=(FONT,CHFNTSIZ),tags="MCINFO")
      self.grid.create_text(xpos+PANE-5,ypos,anchor=E,text=stats[name],fill='red',font=(FONT,CHFNTSIZ),tags="MCINFO")
      xpos = xpos+PANE
      if(xpos >= PWIDTH):
        xpos = 0
        ypos = ypos + SLHGT
    
class mcCtl:
  def __init__(self,master,svrip,CLIENTS):
    global AUTO
    global autoparms
    global clthr
    global clconn
    global cldelay
    global clgpr
    global clientctl
    global nclients
    global iclient
    global active
    global totconn
    global nthEntry
    global conEntry
    global gprEntry
    global delEntry
    global keyEntry
    global binButton
    global ascButton
    global protoVar
    global nclData
    global tcoData
    global numSlide
    
    mysvr = svrip
    clthr = CLThr
    clconn = CLCon
    clgpr = CLgpr
    cldelay = CLdelay
    clkey = CLkey
    active = 0
    totconn = 0

    cfile=open(CLIENTS,"r")
    text = cfile.readlines()
    cfile.close()

# how many clients do we have?
    nrec = len(text)
    if nrec <= 1:
      print "must have init client and at least one loadgen client"
      return
    else:
      print "found ",nrec," client records"
    nclients = nrec - 1

# create frame for control
    ctlFrame=LabelFrame(master,text="load generator parameters",background=BGCLR,fg="white",width=CTWIDTH,height=CHGT,bd=3,relief=RELF)
    ctlFrame.grid_propagate(0)
    ctlFrame.pack(side=LEFT)

# create frame for client status
    cliFrame=LabelFrame(master,text="load generation clients",background=BGCLR,fg="white",width=CLWIDTH,height=CHGT,bd=3,relief=RELF)
    cliFrame.grid_propagate(0)
    cliFrame.pack(side=LEFT)

# create master control frame
    clmFrame=LabelFrame(master,text="master controls",background=BGCLR,fg="white",width=CMWIDTH,height=CHGT,bd=3,relief=RELF)
    clmFrame.pack_propagate(0)
    clmFrame.pack(side=LEFT)

# get params for init client
    rec = text[0].rstrip()
    sys = rec.split(",")
    iclient=mcInit(sys[0],sys[1],sys[2],sys[3],sys[4])

# create grid of client objects
    clientctl = []
    ncl = 0
    for i in range(1,nrec):
      rec = text[i].rstrip()
      sys = rec.split(",")
      irow = ncl/CLICOL
      icol = ncl % CLICOL
      clientctl.append(mcClient(cliFrame,sys[0],sys[1],sys[2],sys[3],sys[4],ncl,irow,icol))
      ncl = ncl + 1
      cliFrame.pack()

# add start/stop buttons to clmFrame
    startButton = Button(clmFrame,text="Start All",bg='light green',fg='white',command=self.startAll,font=(FONT,10,"bold"),width=8,bd=2,relief=RAISED)
    startButton.pack(side=TOP,pady=2)
    stopButton = Button(clmFrame,text="Stop All",bg='indian red',fg='white',command=self.stopAll,font=(FONT,10,"bold"),width=8,bd=2,relief=RAISED)
    stopButton.pack(side=TOP,pady=2)
    initButton = Button(clmFrame,text="Load DB",bg='medium purple',fg='white',command=self.initDB,font=(FONT,10,"bold"),width=8,bd=2,relief=RAISED)
    initButton.pack(side=TOP,pady=2)
    if SHOWAVG:
      resetavgButton = Button(clmFrame,text="Reset Avg",bg='gray',fg='white',command=self.resetavg,font=(FONT,10,"bold"),width=8,bd=2,relief=RAISED)
      resetavgButton.pack(side=TOP,pady=2)
    resetButton = Button(clmFrame,text="Reset",bg='steel blue',fg='white',command=self.reset,font=(FONT,10,"bold"),width=8,bd=2,relief=RAISED)
    resetButton.pack(side=TOP,pady=2)
    exitButton = Button(clmFrame,text="Exit",bg='sienna3',fg='white',command=self.stopAndExit,font=(FONT,10,"bold"),width=8,bd=2,relief=RAISED)
    exitButton.pack(side=TOP,pady=2)

# add entry widgets for load parameters
    ir = 0

    ir = ir + 1
    keyLabel = Label(ctlFrame,text="Key Size",font=(FONT,10,"bold"),bg=BGCLR,fg="white")
    keyLabel.grid(row=ir,column=0,sticky=W,padx=2,pady=0)
    keyCk = (ctlFrame.register(self.keyCheck),'%d','%i','%P','%s','%S','%v','%V','%W')
    keyEntry = Entry(ctlFrame,font=(FONT,10),width=6,justify=RIGHT,validate='focusout',validatecommand=keyCk)
    keyEntry.grid(row=ir,column=1,sticky=E,padx=2,pady=0)
    keyEntry.insert(0,"%d" % CLkey) 
    
    ir = ir + 1
    nthLabel = Label(ctlFrame,text="Threads per LG",font=(FONT,10,"bold"),bg=BGCLR,fg="white")
    nthLabel.grid(row=ir,column=0,sticky=W,padx=2,pady=0)
    nthCk = (ctlFrame.register(self.nthCheck),'%d','%i','%P','%s','%S','%v','%V','%W')
    nthEntry = Entry(ctlFrame,font=(FONT,10),width=6,justify=RIGHT,validate='focusout',validatecommand=nthCk)
    nthEntry.grid(row=ir,column=1,sticky=E,padx=2,pady=0)
    nthEntry.insert(0,"%d" % clthr)
    
    ir = ir + 1
    conLabel = Label(ctlFrame,text="Connections per thread",font=(FONT,10,"bold"),bg=BGCLR,fg="white")
    conLabel.grid(row=ir,column=0,sticky=W,padx=2,pady=0)
    conCk = (ctlFrame.register(self.conCheck),'%d','%i','%P','%s','%S','%v','%V','%W')
    conEntry = Entry(ctlFrame,font=(FONT,10),width=6,justify=RIGHT,validate='focusout',validatecommand=conCk)
    conEntry.grid(row=ir,column=1,sticky=E,padx=2,pady=0)
    conEntry.insert(0,"%d" % clconn)
    
    ir = ir + 1
    gprLabel = Label(ctlFrame,text="Gets per request",font=(FONT,10,"bold"),bg=BGCLR,fg="white")
    gprLabel.grid(row=ir,column=0,sticky=W,padx=2,pady=0)
    gprCk = (ctlFrame.register(self.gprCheck),'%d','%i','%P','%s','%S','%v','%V','%W')
    gprEntry = Entry(ctlFrame,font=(FONT,10),width=6,justify=RIGHT,validate='focusout',validatecommand=gprCk)
    gprEntry.grid(row=ir,column=1,sticky=E,padx=2,pady=0)
    gprEntry.insert(0,"%d" % clgpr)
    
    ir = ir + 1
    delLabel = Label(ctlFrame,text="Microsec between requests",font=(FONT,10,"bold"),bg=BGCLR,fg="white")
    delLabel.grid(row=ir,column=0,sticky=W,padx=2,pady=0)
    delCk = (ctlFrame.register(self.delCheck),'%d','%i','%P','%s','%S','%v','%V','%W')
    delEntry = Entry(ctlFrame,font=(FONT,10),width=6,justify=RIGHT,validate='focusout',validatecommand=delCk)
    delEntry.grid(row=ir,column=1,sticky=E,padx=2,pady=0)
    delEntry.insert(0,"%d" % cldelay)

    ir = ir + 1
    protoVar = StringVar()
    protoLabel = Label(ctlFrame,text="Protocol",font=(FONT,10,"bold"),bg=BGCLR,fg="white")
    protoLabel.grid(row=ir,column=0,sticky=W,padx=2,pady=0)
    binButton = Radiobutton(ctlFrame,text="binary",variable=protoVar,value="binary",command=self.protoCheck)
    binButton.grid(row=ir,column=0,sticky=E,padx=2,pady=0)
    ascButton = Radiobutton(ctlFrame,text="ascii",variable=protoVar,value="ascii",command=self.protoCheck)
    ascButton.grid(row=ir,column=1,sticky=E,padx=2,pady=0)
    binButton.invoke()
    
    ir = ir + 1
    nclLabel = Label(ctlFrame,text="Active load generators",font=(FONT,10,"bold"),bg=BGCLR,fg="white")
    nclLabel.grid(row=ir,column=0,sticky=W,padx=2,pady=0)
    nclData = Label(ctlFrame,font=(FONT,10),anchor=E,width=6,fg="steel blue",bg='light gray',justify=RIGHT,bd=2,relief=RIDGE)
    nclData.grid(row=ir,column=1,sticky=E,padx=2,pady=0)
    nclData.config(text="%d" % active,justify=RIGHT)
    
    ir = ir + 1
    tcoLabel = Label(ctlFrame,text="Total connections",font=(FONT,10,"bold"),bg=BGCLR,fg="white")
    tcoLabel.grid(row=ir,column=0,sticky=W,padx=2,pady=0)
    tcoData = Label(ctlFrame,font=(FONT,10),anchor=E,width=6,fg="steel blue",bg='light gray',justify=RIGHT,bd=2,relief=RIDGE)
    tcoData.grid(row=ir,column=1,sticky=E,padx=2,pady=0)
    tcoData.config(text="%d" % totconn,justify=RIGHT)
    
    master.update_idletasks()

# note:  voodoo below is to pass Entry data to validate command
#  %d = Type of action (1=insert, 0=delete, -1 for others)
#  %i = index of char string to be inserted/deleted, or -1
#  %P = value of the entry if the edit is allowed
#  %s = value of the entry prior to editing
#  %S = the text string being inserted or deleted, if any
#  %v = the type of validation currently set
#  %V = the type of validation that triggered the callback
#       (key, focusin, focusout, forced)
#  %W = the tk name of the widget

  def keyCheck(self,d,i,P,s,S,v,V,W):
    key = int(P)
    if key < MINthr:
      print "key too small"
      keyEntry.delete(0,12)
      keyEntry.insert(0,MINkey)
      return False
    if key > MAXkey:
      print "key too large"
      keyEntry.delete(0,12)
      keyEntry.insert(0,MAXkey)
      return False
    print "key size now %d" % key
    return True
    
  def nthCheck(self,d,i,P,s,S,v,V,W):
    nth = int(P)
    if nth < MINthr:
      print "too few threads"
      nthEntry.delete(0,12)
      nthEntry.insert(0,MINthr)
      return False
    if nth > MAXthr:
      print "too many threads"
      nthEntry.delete(0,12)
      nthEntry.insert(0,MAXthr)
      return False
    print "num threads now %d" % nth
    return True
    
  def conCheck(self,d,i,P,s,S,v,V,W):
    con = int(P)
    if con < MINcon:
      print "too few connections"
      conEntry.delete(0,12)
      conEntry.insert(0,MINcon)
      return False
    if con > MAXcon:
      print "too many connections"
      conEntry.delete(0,12)
      conEntry.insert(0,MAXcon)
      return False
    print "num connections per thread now %d" % con
    return True
    
  def gprCheck(self,d,i,P,s,S,v,V,W):
    gpr = int(P)
    if gpr < MINgpr:
      print "too few gets per request"
      gprEntry.delete(0,12)
      gprEntry.insert(0,MINgpr)
      return False
    if gpr > MAXgpr:
      print "too many gets per request"
      gprEntry.delete(0,12)
      gprEntry.insert(0,MAXgpr)
      return False
    print "num gets per request now %d" % gpr
    return True
    
  def delCheck(self,d,i,P,s,S,v,V,W):
    delay = int(P)
    if delay < MINdel:
      print "delay too small"
      delEntry.delete(0,12)
      delEntry.insert(0,MINdel)
      return False
    if delay > MAXdel:
      print "delay too large"
      delEntry.delete(0,12)
      delEntry.insert(0,MAXdel)
      return False
    print "delay between requests now %d microsecond" % delay
    return True
  
  def protoCheck(self):
    print "protocol is now %s" % protoVar.get()
    return True
    
  def startAll(self):
    print "starting all clients"
    for i in range(0,len(clientctl)):
      clientctl[i].pressed()

  def initDB(self):
# create thread to asynchronously run mc-hammr init
    print "initializing DB"
    thread.start_new_thread(iclient.initCmd,(int(keyEntry.get()),CLval,))

  def stopAll(self):
    print "stopping all clients"
    for i in range(0,len(clientctl)):
      clientctl[i].stopCmd()

  def stopAndExit(self):
    print "stopping all clients"
    for i in range(0,len(clientctl)):
      clientctl[i].stopCmd()
    print "exiting"
    exit(0)

  def resetavg(self):
    global samples
    global settot
    global gettot
    global gmisstot
    global lattot
    samples = 0
    settot = 0
    gettot = 0
    gmisstot = 0
    lattot = 0
    
  def setParams(self,ith,icon,igpr,idelay,ikey,imode):
    nthEntry.delete(0,12)
    nthEntry.insert(0,"%d" % ith)
    conEntry.delete(0,12)
    conEntry.insert(0,"%d" % icon)
    gprEntry.delete(0,12)
    gprEntry.insert(0,"%d" % igpr)
    delEntry.delete(0,12)
    delEntry.insert(0,"%d" % idelay)
    keyEntry.delete(0,12)
    keyEntry.insert(0,"%d" % ikey)
    if(imode == "ascii"):
      ascButton.invoke()

  def startN(self,icl):
    print "starting ",icl," clients"
    for i in range(0,icl):
      print "starting client ",icl
      clientctl[i].pressed()

  def reset(self):
    global active
    global totconn
    global nthEntry
    global conEntry
    global gprEntry
    global delEntry
    global keyEntry
    global nclData

# reset values for parameters to defaults
    nthEntry.delete(0,12)
    nthEntry.insert(0,"%d" % CLThr)
    conEntry.delete(0,12)
    conEntry.insert(0,"%d" % CLCon)
    gprEntry.delete(0,12)
    gprEntry.insert(0,"%d" % CLgpr)
    delEntry.delete(0,12)
    delEntry.insert(0,"%d" % CLdelay)
    keyEntry.delete(0,12)
    keyEntry.insert(0,"%d" % CLkey)

# stop all running clients
    for i in range(0,len(clientctl)):
      print "stopping mc-hammr on client %d" % i
      clientctl[i].stopCmd()
# pkill all surviving mc-hammr instances
    for i in range(0,len(clientctl)):
      print "pkill mc-hammr on client %d" % i
      clientctl[i].pkill()
   
class mcInit:     
  def __init__(self,remote,username,password,svrip,svrport):
    self.state = "ready"
    self.mysys = remote
    self.myuser = username
    self.mypass = password
    self.mysvrip = svrip
    self.mysvrport = svrport

    print "setting up ssh connection for init thread"
    self.ssh = paramiko.SSHClient()

  def initCmd(self,keysize,valsize):
    global active
    global totconn
    global nthEntry
    global conEntry
    global gprEntry
    global delEntry
    global keyEntry
    global nclData
    print "in initCmd"

# get current values for parameters
    self.thr = 1
    self.conn = 1
    self.delay = 10
    self.keysize = keysize
    self.keypre = "%d:" % self.keysize
    self.valsize = valsize
    if self.state == "running":
      print "init already running on ",self.mysys
      return

# set up ssh connection to client
#    print "logging in to ",self.mysys," as ",self.myuser," password ",self.mypass
    self.ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())

    self.ssh.connect(self.mysys,username=self.myuser,password=self.mypass)
# run_init hostip port nthreads connections delay keysize keyprefix valsize
    cmd = "./client_scripts/run_init %s %s %d %d %d %d %s %d" % (self.mysvrip,self.mysvrport,self.thr, self.conn, self.delay, self.keysize,self.keypre,self.valsize)
    print "running %s on %s " % (cmd,self.mysys)
    self.ssh_stdin,self.ssh_stdout, self.ssh_stderr = self.ssh.exec_command(cmd)

    status = self.ssh_stdout.channel.recv_exit_status()
    print "run_init completed with status ","%d" % status
#    print self.ssh_stdout
#    print self.ssh_stderr
    self.ssh.close()

class mcClient:

  def __init__(self,master,remote,username,password,svrip,svrport,id,irow,icol):
    self.state = "ready"
    self.mysys = remote
    self.myuser = username
    self.mypass = password
    self.mysvrip = svrip
    self.mysvrport = svrport
    self.myid = id
    self.state = "initial"

    self.ssh = paramiko.SSHClient()
    self.icon = Button(master,text=self.mysys,command=self.pressed,bg='khaki3',fg="black",font=(FONT,10),height=1,width=8,bd=2)
    self.icon.grid(row=irow,column=icol,padx=4,pady=4)
    self.state = "ready"


  def pressed(self):
    if self.state == "running":
      self.stopCmd()
    else:
      self.runCmd()

   
  def runCmd(self):
    global active
    global totconn
    global g_thr
    global g_conn
    global g_gpr
    global g_delay
    global g_keysize
    global g_valsize
    global g_protocol
    global nthEntry
    global conEntry
    global gprEntry
    global delEntry
    global keyEntry
    global nclData

# get current values for parameters
    self.thr = int(nthEntry.get())
    self.conn = int(conEntry.get())
    self.gpr = int(gprEntry.get())
    self.delay = int(delEntry.get())
    self.keysize = int(keyEntry.get())
    self.keypre = "%d:" % self.keysize
    self.valsize = CLval
    self.protocol = protoVar.get()

    if self.state == "running":
      print "client ",self.myid,"already running!"
      return
    g_thr = self.thr
    g_conn = self.conn
    g_gpr = self.gpr
    g_delay = self.delay
    g_keysize = self.keysize
    g_valsize = self.valsize
    g_protocol = self.protocol

# set up ssh connection to client
#    print "logging in to ",self.mysys," as ",self.myuser," password ",self.mypass
    self.ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    self.ssh.connect(self.mysys,username=self.myuser,password=self.mypass)
# load_gen hostip port nthreads conn mget delay keysize keyprefix valsize
    cmd = "./client_scripts/run_loadgen %s %s %d %d %d %d %d %s %d %s" % (self.mysvrip,self.mysvrport,self.thr, self.conn, self.gpr, self.delay, self.keysize,self.keypre,self.valsize,self.protocol)
    print "running %s on %s " % (cmd,self.mysys)
    self.ssh_stdin,self.ssh_stdout, self.ssh_stderr = self.ssh.exec_command(cmd)
    self.icon.configure(background='DarkSeaGreen2',relief=SUNKEN)
    self.state = "running"
    active = active + 1
    nclData.config(text="%d" % active)
    totconn = totconn + (self.conn * self.thr)
    tcoData.config(text="%d" % totconn)

  def stopCmd(self):
    global active
    global totconn
    global nclData
    if self.state != "running":
      return
    print "closing ssh connection to ",self.mysys
    self.ssh_stdin.write("stop\n")
    self.ssh_stdin.write("stop\n")
#    print self.ssh_stdout
#    print self.ssh_stderr
    self.ssh.close()
    self.icon.configure(background='khaki3',relief=RAISED)
    self.state = "ready"
    active = active - 1
    nclData.config(text="%d" % active)
    totconn = totconn - (self.conn * self.thr)
    tcoData.config(text="%d" % totconn)

  def pkill(self):
    self.stopCmd()
    self.ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    self.ssh.connect(self.mysys,username=self.myuser,password=self.mypass)
    cmd = "pkill mc-hammr"
    print "running %s on %s " % (cmd,self.mysys)
    self.ssh_stdin,self.ssh_stdout, self.ssh_stderr = self.ssh.exec_command(cmd)
#    print self.ssh_stdout
#    print self.ssh_stderr
    self.ssh.close()

def printusage():
  print "usage: python mcmon.py [--stats] [--latency] [--misses] [--clients=<file>]  [--showavg] [--record=<file>] [--auto=<file>] [--help] system:port"
  print "  --stats: display dump of memcached stats"
  print "  --misses: plot get misses"
  print "  --latency: plot latency"
  print "  --clients=<filename>: client description file (note 1st entry is init)"
  print "  --showavg: show running averages"
  print "  --record=<filename>: append averages to <filename> when sample count == 50"
  print "  --auto=<filename>: automatically run tests described in <filename>"
  print "      file should contain records with comma separated integer parameters:"
  print "      clients,threads,connections per thread,gets per req,delay,keysize"
  print "  --help: print this message"
#
# main driver
#
global SYSTEM
global HOSTNAME
global PORT
global LATMULTI
global LOADGEN
global RECORD
global TRACE
global AUTO
global autoparms
global warmcnt
global nclients
STATS = False
LATENCY = False
LATMULTI = False
MISSES = False
LOADGEN = False
SHOWAVG = False
RECORD = False
RECFILE=""
TRACE = False
TRACEFILE=""
AUTO = False
AUTOFILE = "AUTOPARAMS"
CLIENTS = 'CLIENTS'
SYSTEM = "localhost:11211"

starttime = os.times()
root = Tk()
# some housekeeping
locale.setlocale(locale.LC_ALL, 'en_US')

# parse args
for a in sys.argv[1:]:
  if (a == "--stats"):
    STATS = True
  elif (a == "--loadgen"):
    LOADGEN = True
  elif (a == "--latency"):
    LATENCY = True
  elif (a == "--latmulti"):
    LATMULTI = True
    LATENCY = True
  elif (a == "--misses"):
    MISSES = True
  elif (a == "--showavg"):
    SHOWAVG = True
  elif (a[0:9] == "--record="):
    SHOWAVG = True
    RECORD = True
    RECFILE=a.split('=')[1]
    print "recording averages in %s" % RECFILE
  elif (a[0:8] == "--trace="):
    TRACE = True
    TRACEFILE=a.split('=')[1]
    print "recording data in %s" % TRACEFILE
  elif (a[0:7] == "--auto="):
    SHOWAVG = True
    RECORD = True
    AUTO = True
    AUTOFILE=a.split('=')[1]
    print "executing test parameters from %s" % AUTOFILE
  elif (a[0:10] == "--clients="):
    CLIENTS=a.split('=')[1]
    print "client file is %s" % CLIENTS
  elif (a == "--help"):
    printusage()
    exit(0)
  elif (a[0] == "-"):
    print "invalid option %s" % a
    printusage()
    exit(1)
  else:
    SYSTEM = a 

HOSTNAME = SYSTEM.split(':')[0]
PORT = SYSTEM.split(':')[1]

# create header 
root.title("memcached monitor")

# create frame
mcFrame=Frame(root,bg='black',width=PWIDTH)
mcFrame.pack()
stats = mcStats(mcFrame,SYSTEM,LATENCY,MISSES,STATS,SHOWAVG,RECFILE,TRACEFILE,AUTOFILE)
root.update_idletasks()

if LOADGEN:
  ctFrame=Frame(root,bg='black',width=PWIDTH)
  ctFrame.pack()
  controller = mcCtl(ctFrame,SYSTEM,CLIENTS)
root.update_idletasks()

# initialize auto parameters
if AUTO:
  afile=open(AUTOFILE,"r")
  autoparms = deque(afile.readlines())
  afile.close()
  ptext = autoparms.popleft().rstrip()
  params = ptext.split(",")
  iclients = int(params[0])
  if iclients > nclients:
    print "requested ",iclients," clients, ",nclients," available"
    iclients = nclients
  ithreads = int(params[1])
  iconn = int(params[2])
  igetperreq = int(params[3])
  idelay= int(params[4])
  ikeysize= int(params[5])
  imode = params[6]
  controller.setParams(ithreads,iconn,igetperreq,idelay,ikeysize,imode)
  controller.startN(iclients)
  

root.update_idletasks()
root.mainloop()
