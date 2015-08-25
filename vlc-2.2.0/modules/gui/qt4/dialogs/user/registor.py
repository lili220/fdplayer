#/bin/python2.7
import requests
from suds.client import Client

global nfschina
nfschina = Client('http://192.168.7.96:8000/nfschina/service/?wsdl')


def nfschina_register(username, password):
	global nfschina
	err = nfschina.service.nfschina_register(username,password)
	return err
	
def nfschina_login(username, password, is_share, ip, port):
	global nfschina
	err = nfschina.service.nfschina_login(username, password, is_share, ip, port)
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
	return err

def nfschina_delete(userid, filename):
	global nfschina
	err = nfschina.service.nfschina_delete(userid,filename)
	return err

def nfschina_download(userid, filename):
	global nfschina
	err = nfschina.service.nfschina_download(userid, filename)
	return err



















