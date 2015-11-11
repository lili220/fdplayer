# -*- coding=utf-8 -*-

import urllib2 
import sys, httplib 
import re
import requests

def parse_url(url):
	s1 =  url.split("//")
	s2 = s1[1].split("/")
	s3 = s1[1].split("/",1)
	return s3

def nfschina_register(username,password,url): 
	SENDTPL = \
'''<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope xmlns:ns0="http://schemas.xmlsoap.org/soap/envelope/" xmlns:ns1="nfschina.com" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/">
   <SOAP-ENV:Header/>
   <ns0:Body>
	  <ns1:nfschina_register>
		 <ns1:username>%s</ns1:username>
		 <ns1:password>%s</ns1:password>
	  </ns1:nfschina_register>
   </ns0:Body>
</SOAP-ENV:Envelope>'''
	SoapMessage = SENDTPL % (username, password)
	purl = parse_url(url)
	ip = purl[0]
	postpath = "/" + purl[1]
	soapaction = url
	webservice = httplib.HTTP(ip) 
	webservice.putrequest("POST", postpath)
	webservice.putheader("Host", ip) 
	webservice.putheader("User-Agent", "Python Post") 
	webservice.putheader("Content-type", "text/xml; charset=\"UTF-8\"") 
	webservice.putheader("Content-length", "%d" % len(SoapMessage)) 
	webservice.putheader("SOAPAction", soapaction) 
	webservice.endheaders() 
	webservice.send(SoapMessage) 
	# get the response 
	statuscode, statusmessage, header = webservice.getreply() 
	msg = webservice.getfile().read()
	p1=re.compile(r'nfschina_registerResult>(.*?)nfschina_registerResult>')
	p2=re.compile(r'(.*?)</')
	match = p1.findall(msg)
	match1 = p2.findall(match[0])
	print match
	print int(match1[0])
	return int(match1[0])

def nfschina_login(username,password,url): 
	SENDTPL = \
'''<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope xmlns:ns0="http://schemas.xmlsoap.org/soap/envelope/" xmlns:ns1="nfschina.com" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/">
   <SOAP-ENV:Header/>
   <ns0:Body>
	  <ns1:nfschina_login>
		 <ns1:username>%s</ns1:username>
		 <ns1:password>%s</ns1:password>
	  </ns1:nfschina_login>
   </ns0:Body>
</SOAP-ENV:Envelope>'''
	SoapMessage = SENDTPL % (username, password)
	purl = parse_url(url)
	ip = purl[0]
	postpath = "/" + purl[1]
	soapaction = url
	webservice = httplib.HTTP(ip) 
	webservice.putrequest("POST", postpath)
	webservice.putheader("Host", ip) 
	webservice.putheader("User-Agent", "Python Post") 
	webservice.putheader("Content-type", "text/xml; charset=\"UTF-8\"") 
	webservice.putheader("Content-length", "%d" % len(SoapMessage)) 
	webservice.putheader("SOAPAction", soapaction) 
	webservice.endheaders() 
	webservice.send(SoapMessage) 
	# get the response 
	statuscode, statusmessage, header = webservice.getreply() 
	msg = webservice.getfile().read()
	p1=re.compile(r'nfschina_loginResult>(.*?)nfschina_loginResult>')
	p2=re.compile(r'(.*?)</')
	match = p1.findall(msg)
	match1 = p2.findall(match[0])
	print match
	print int(match1[0])
	return int(match1[0])

def nfschina_listmyfile(userid, url): 
	SENDTPL = \
'''<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope xmlns:ns0="http://schemas.xmlsoap.org/soap/envelope/" xmlns:ns1="nfschina.com" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/">
   <SOAP-ENV:Header/>
   <ns0:Body>
	  <ns1:nfschina_listmyfile>
		 <ns1:userid>%d</ns1:userid>
	  </ns1:nfschina_listmyfile>
   </ns0:Body>
</SOAP-ENV:Envelope>'''
	SoapMessage = SENDTPL % (userid)
	purl = parse_url(url)
	ip = purl[0]
	postpath = "/" + purl[1]
	soapaction = url
	webservice = httplib.HTTP(ip) 
	webservice.putrequest("POST", postpath)
	webservice.putheader("Host", ip) 
	webservice.putheader("User-Agent", "Python Post") 
	webservice.putheader("Content-type", "text/xml; charset=\"UTF-8\"") 
	webservice.putheader("Content-length", "%d" % len(SoapMessage)) 
	webservice.putheader("SOAPAction", soapaction)
	webservice.endheaders() 
	webservice.send(SoapMessage) 
	# get the response 
	statuscode, statusmessage, header = webservice.getreply() 
	print "Response: ", statuscode, statusmessage 
	print "headers: ", header 
	results = webservice.getfile().read() 
	print results
	p = re.compile(r'(?<=<tns:string>)(.*?)(?=</tns:string>)')
	'''<tns:string>guigu.mp4</tns:string> '''
	match = p.findall(results)
	if match:
		return match
	p = re.compile(r'(?<=<s0:string>)(.*?)(?=</s0:string>)')
	'''<tns:string>guigu.mp4</tns:string> '''
	match = p.findall(results)
	if match:
		return match

def nfschina_upload(userid,filename,filepath,url,httpurl): 
	SENDTPL = \
'''<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope xmlns:ns0="http://schemas.xmlsoap.org/soap/envelope/" xmlns:ns1="nfschina.com" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/">
   <SOAP-ENV:Header/>
   <ns0:Body>
	  <ns1:nfschina_upload>
		 <ns1:userid>%d</ns1:userid>
		 <ns1:filename>%s</ns1:filename>
	  </ns1:nfschina_upload>
   </ns0:Body>
</SOAP-ENV:Envelope>'''
	SoapMessage = SENDTPL % (userid, filename)
	purl = parse_url(url)
	ip = purl[0]
	postpath = "/" + purl[1]
	soapaction = url
	webservice = httplib.HTTP(ip) 
	webservice.putrequest("POST", postpath)
	webservice.putheader("Host", ip) 
	webservice.putheader("User-Agent", "Python Post") 
	webservice.putheader("Content-type", "text/xml; charset=\"UTF-8\"") 
	webservice.putheader("Content-length", "%d" % len(SoapMessage)) 
	webservice.putheader("SOAPAction", soapaction) 
	webservice.endheaders() 
	webservice.send(SoapMessage) 
	# get the response 
	statuscode, statusmessage, header = webservice.getreply() 
	print "Response: ", statuscode, statusmessage 
	print "headers: ", header 
	ret =  webservice.getfile().read()
	print ret

	p1=re.compile(r'nfschina_uploadResult>(.*?)nfschina_uploadResult>')
	p2=re.compile(r'(.*?)</')
	ret1 = p1.findall(ret)
	print ret1
	ret2 = p2.findall(ret1[0])
	print ret2[0]

	if int(ret2[0]) == int(-3):
		print "hhhhhhhhhhhhh" # cant uploada
		return -3

	results = 0
#	url = 'http://192.168.7.97/haha/service/uploadfile'
	url = httpurl
	file1 = open(filepath,'r')
	offset=1
	off_set=0
	while 1:
		rf = file1.read(20*1024*1024)
		if rf != '':
			if int(offset-1) == int(off_set):
				files = {'file': rf}
				payload = {'userid': userid, 'filename':filename, 'offset':offset}
				result = requests.post(url,payload,files=files)
				off_set=int(result.text)
				if str(result).find("200") and int(offset) <= int(off_set):
					offset+=1
					print result.url,result.text
			else:
				offset+=1
				print "haha"
		else:
			files = {'file': rf}
			offset=-2
			payload = {'userid': userid, 'filename':filename, 'offset':offset}
			result = requests.post(url,payload,files=files)
			print result.text
			if int(result.text) == int(1):
				print "success upload file"
				results = 1
				break
	file1.close
	return results

def nfschina_delete(userid, filename, url): 
	SENDTPL = \
'''<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope xmlns:ns0="http://schemas.xmlsoap.org/soap/envelope/" xmlns:ns1="nfschina.com" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/">
   <SOAP-ENV:Header/>
   <ns0:Body>
	  <ns1:nfschina_delete>
		 <ns1:userid>%d</ns1:userid>
		 <ns1:filename>%s</ns1:filename>
	  </ns1:nfschina_delete>
   </ns0:Body>
</SOAP-ENV:Envelope>'''
	SoapMessage = SENDTPL % (userid, filename)
	purl = parse_url(url)
	ip = purl[0]
	postpath = "/" + purl[1]
	soapaction = url
	webservice = httplib.HTTP(ip) 
	webservice.putrequest("POST", postpath)
	webservice.putheader("Host", ip) 
	webservice.putheader("User-Agent", "Python Post") 
	webservice.putheader("Content-type", "text/xml; charset=\"UTF-8\"") 
	webservice.putheader("Content-length", "%d" % len(SoapMessage)) 
	webservice.putheader("SOAPAction", soapaction) 
	webservice.endheaders() 
	webservice.send(SoapMessage) 
	# get the response 
	statuscode, statusmessage, header = webservice.getreply() 
	print "Response: ", statuscode, statusmessage 
	print "headers: ", header 
	print webservice.getfile().read() 

def nfschina_download(userid, filename, url): 
	SENDTPL = \
'''<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope xmlns:ns0="http://schemas.xmlsoap.org/soap/envelope/" xmlns:ns1="nfschina.com" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/">
   <SOAP-ENV:Header/>
   <ns0:Body>
	  <ns1:nfschina_download>
		 <ns1:userid>%d</ns1:userid>
		 <ns1:filename>%s</ns1:filename>
	  </ns1:nfschina_download>
   </ns0:Body>
</SOAP-ENV:Envelope>'''
	SoapMessage = SENDTPL % (userid, filename)
	purl = parse_url(url)
	ip = purl[0]
	postpath = "/" + purl[1]
	soapaction = url
	webservice = httplib.HTTP(ip)
	webservice.putrequest("POST", postpath)
	webservice.putheader("Host", ip) 
	webservice.putheader("User-Agent", "Python Post") 
	webservice.putheader("Content-type", "text/xml; charset=\"UTF-8\"") 
	webservice.putheader("Content-length", "%d" % len(SoapMessage)) 
	webservice.putheader("SOAPAction", soapaction) 
	webservice.endheaders() 
	webservice.send(SoapMessage) 
	# get the response 
	statuscode, statusmessage, header = webservice.getreply() 
	print "Response: ", statuscode, statusmessage 
	print "headers: ", header 
	#print webservice.getfile().read() 
	results = webservice.getfile().read()
	print str(results)
	p = re.compile(r'(?<=<tns:string>)(.*?)(?=</tns:string>)')
	match = p.findall(results)
	if match:
		print match
		return match
	p = re.compile(r'(?<=<s0:string>)(.*?)(?=</s0:string>)')
	match = p.findall(results)
	if match :
		print match
		return match
	#return match


if __name__ == "__main__":
#	nfschina_register("tjiajiandong","123456","http://192.168.7.97/haha/service/?wsdl")
#	nfschina_login("tjiajiandong","123456","http://192.168.7.97/haha/service/?wsdl")
#	nfschina_listmyfile(2,"http://192.168.7.97/haha/service/?wsdl")
#	nfschina_delete(2,"duolaameng.mkv","http://192.168.7.97/haha/service/?wsdl")
	nfschina_download(9,"哈哈.mp4", "http://192.168.7.97/haha/service/wsdl")
#	nfschina_upload(2,"test.mp4","/home/nfschina/test.mp4","http://192.168.7.97/haha/service/?wsdl",'http://192.168.7.97/haha/service/uploadfile')
