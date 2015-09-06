#/bin/python2.7
import requests
from suds.client import Client

global nfschina
nfschina = Client('http://192.168.7.96:8000/nfschina/service/?wsdl')


def nfschina_register(username, password):
	global nfschina
	err = nfschina.service.nfschina_register(username,password)
	return err
	
def nfschina_login(username, password):
	global nfschina
	err = nfschina.service.nfschina_login(username, password)
	return err

def nfschina_keeponline(userid, is_share):
	global nfschina
	err = nfschina.service.nfschina_keeponline(userid,is_share)
	return err

def nfschina_upload(userid, filename,filepath):
	global nfschina
	url = 'http://192.168.7.96:8000/nfschina/service/uploadfile'
	files = {'file': open(filepath, 'rb')}
	payload = {'userid': userid, 'filename':filename}
	err = nfschina.service.nfschina_upload(userid, filename)
	if err == 1:
		err = requests.post(url,payload,files=files)
	return err
	
def nfschina_listmyfile(userid):
	global nfschina
	err = nfschina.service.nfschina_listmyfile(userid)
        if len(err) == 0:
            return []
        else:
            return err[0]

def nfschina_delete(userid, filename):
	global nfschina
	err = nfschina.service.nfschina_delete(userid,filename)
	return err

def nfschina_download(userid, filename):
	global nfschina
	err = nfschina.service.nfschina_download(userid, filename)
        if len(err) == 0:
            return []
        else:
            return err[0]

def nfschina_logout(userid):
        global nfschina
        err = nfschina.service.nfschina_logout(userid)
        return err

import sys, getopt
from socket import *

def SocketClient(localIp,localPort,userid,is_share,ServerUrl):
    try:
        s=socket(AF_INET,SOCK_STREAM,0)
        s.setsockopt(SOL_SOCKET,SO_REUSEADDR, 1)
        s.bind((localIp, localPort))

        Colon = ServerUrl.find(':')
        IP = ServerUrl[0:Colon]
        Port = ServerUrl[Colon+1:]

        s.connect((IP,int(Port)))
        sdata='GET /Test/userid=%d&is_share=%d HTTP/1.1\r\nHost: %s\r\nnfs-cmd: keepalive\r\n\r\n'%(userid,is_share,ServerUrl)

        print "Request:\r\n%s\r\n"%sdata
        s.send(sdata)
        sresult=s.recv(1024)

        print "Response:\r\n%s\r\n" %sresult
        s.close()
    except Exception,ex:
        print ex
        s.close()

