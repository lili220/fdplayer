#!/usr/bin/python
# -*- coding: utf-8 -*-
# filename: paxel.py

'''It is a multi-thread downloading tool

    It was developed follow axel.
        Author: volans
        E-mail: volansw [at] gmail.com
'''

import sys
import os
import time
import urllib
from threading import Thread
import threading
from multiprocessing import  Value, Process

local_proxies = {'http': 'http://131.139.58.200:8080'}


class AxelPython(Thread, urllib.FancyURLopener):
    '''Multi-thread downloading class.

        run() is a vitural method of Thread.
    '''
    def __init__(self, threadname, url, filename, theindex, ranges=0, proxies={}):
        Thread.__init__(self, name=threadname)
        urllib.FancyURLopener.__init__(self, proxies)
        self.name = threadname
        self.url = url
        self.filename = filename
        self.ranges = ranges
        self.downloaded = 0
        self.theindex = theindex

    def run(self):
        '''vertual function in Thread'''
        try:
            self.downloaded = os.path.getsize( self.filename )
        except OSError:
            #print 'never downloaded'
            self.downloaded = 0

        # rebuild start poind
        self.startpoint = self.ranges[0] + self.downloaded
        
        # This part is completed
        print "self.startpoint=%d, self.ranges[1]=%d" %(self.startpoint, self.ranges[1])
        if self.startpoint >= self.ranges[1]:
            print 'Part %s has been downloaded over.' % self.filename
            return
        
        self.oneTimeSize = 16384 #16kByte/time
        print 'task %s will download from %d to %d' % (self.name, self.startpoint, self.ranges[1])

        self.addheader("Range", "bytes=%d-%d" % (self.startpoint, self.ranges[1]))
            
        self.urlhandle = self.open( self.url )
        data = self.urlhandle.read( self.oneTimeSize )
        while data:
            #print "---------------------"
            if isStop(self.theindex)==1:
                break
            filehandle = open( self.filename, 'ab+' )
            filehandle.write( data )
            filehandle.close()

            self.downloaded += len( data )
            #print "%s-->downloaded %d" % (self.name, self.downloaded)
            #progress = u'\r...'

            data = self.urlhandle.read( self.oneTimeSize )
            #print "download=%d"%(self.downloaded)
        print "thread %d is finished"%(self.downloaded)
        
def GetUrlFileSize(url, proxies={}):
    urlHandler = urllib.urlopen( url, proxies=proxies )
    headers = urlHandler.info().headers
    #print "DEBUG: url:%s"%url 
    length = 0
    for header in headers:
        if header.find('Length') != -1:
            length = header.split(':')[-1].strip()
            length = int(length)
    #print "GetUrlFileSize file size =%d" %(length)
    return length

def SpliteBlocks(totalsize, blocknumber):
    blocksize = totalsize/blocknumber
    ranges = []
    for i in range(0, blocknumber-1):
        ranges.append((i*blocksize, i*blocksize +blocksize - 1))
    ranges.append(( blocksize*(blocknumber-1), totalsize -1 ))

    return ranges
def islive(tasks):
    for task in tasks:
        if task.isAlive():
            return True
    return False

def paxel(url, output, theindex, blocks=6, proxies=local_proxies):
    ''' paxel
    '''
    global loading
    global datamutex
    print "loading=[%d] theindex=[%d]"%(loading[theindex],theindex)
    size = GetUrlFileSize( url, proxies )
    print str("size is ") + str(size)
    ranges = SpliteBlocks( size, blocks )
    print str(ranges)

    threadname = [ "thread_%d" % i for i in range(0, blocks) ]
    filename = [ str(output)+str("tmpfile_") + str(i) for i in range(0, blocks) ]
    print str("filename is ") + str(filename) +str("   ") + str(threadname)
    #filename = [ str(output)+"tmpfile_%d" % i for i in range(0, blocks) ]
  
    tasks = []
    for i in range(0,blocks):
        task = AxelPython( threadname[i], url, filename[i], theindex, ranges[i] )
        task.setDaemon( True )
        task.start()
        tasks.append( task )
        
    time.sleep( 2 )
    while islive(tasks):
        if isStop(theindex)==1:
            return
        downloaded = sum( [task.downloaded for task in tasks] )
        process = downloaded/float(size)*100
        datamutex[theindex].acquire()
        loading[theindex] = process - 1
        datamutex[theindex].release()
        show = u'\rFilesize:%d Downloaded:%d Completed:%.2f%%' % (size, downloaded, process)
        sys.stdout.write(show)
        sys.stdout.flush()
        time.sleep( 1 )
    
    filehandle = open( output, 'wb+' )
    for i in filename:
        #print str(filename)
        if isStop(theindex)==1:
            return
        f = open( i, 'rb' )
        filehandle.write( f.read() )
        f.close()
        try:
            os.remove(i)
            pass
        except:
            pass

    filehandle.close()
    datamutex[theindex].acquire()
    loading[theindex] = 100
    datamutex[theindex].release()
    print "load is finished"

global index
index=0
global loading
loading=[0 for col in range(30)]
global exitflag
exitflag=[0 for col in range(30)]
global datamutex
datamutex = [threading.Lock() for col in range(30)]

global mutex
mutex = threading.Lock()


def isStop(index):
    global exitflag
    global datamutex
    
    stopflag = 0
    datamutex[index].acquire()
    stopflag = exitflag[index]
    datamutex[index].release()
    return stopflag


def stop_download(index):
    global exitflag
    global datamutex

    datamutex[index].acquire()
    exitflag[index]=1
    datamutex[index].release()
    print "index=%d threads is exited"%(index)


def printf(index):
    global loading
    global datamutex
    datamutex[index].acquire()
    process=loading[index]
    datamutex[index].release()    
    print "index=%d loading is %d"%(index,process)
    return process

def start_download(url,output,nthread=8):
    global loading
    global index
    global mutex
    global datamutex
    global exitflag

    theindex = 0
    mutex.acquire()
    theindex = index
    index = index + 1
    mutex.release()

    datamutex[index].acquire()
    loading[theindex] = 0
    exitflag[theindex] = 0
    datamutex[index].release()  

    #p = threading.Thread(target=paxel,args=(url,output,theindex,1,{},))
    p = threading.Thread(target=paxel,args=(url,output,theindex,nthread,{},))
    p.start()
    print "start_download theindex=[%d] finished"%(theindex)
    return theindex

if __name__ == '__main__':
   #url = "http://192.168.7.97/download/5/baofengyu1.mp4"
   #url = "http://192.168.7.88:8090/transfer/3/1/test.mpg"
   #output = 'test.mpg'
   #url = "http://192.168.7.88:8090/transfer/3/1/中文.wmv"
   url = "http://192.168.7.88:8090/transfer/2/3/暴风雨.mp4"
   size = GetUrlFileSize( url, {} )

   #output = '中文.wmv'
   #index = start_download(url,output,1)
   #i = 0
   #while 1:
   #   process = printf(index)
   #   time.sleep(1)
   #   if process == 100:
   #       break

