#-*- coding=utf-8 -*-

import urllib2 , re
import sys, httplib 

def parse_url(url):
	s1 =  url.split("//")
	s2 = s1[1].split("/")
	s3 = s1[1].split("/",1)
	return s3


def nfschina_getresource(userid, stype, filename,url): 
	SENDTPL = \
'''<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope xmlns:ns0="http://schemas.xmlsoap.org/soap/envelope/" xmlns:ns1="nfschina.com" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/">
<SOAP-ENV:Header/>
<ns0:Body>
	<ns1:nfschina_getresource>
		<ns1:userid>%d</ns1:userid>
		<ns1:stype>%d</ns1:stype>
		<ns1:filename>%s</ns1:filename>
	</ns1:nfschina_getresource>
</ns0:Body>
</SOAP-ENV:Envelope>'''
	SoapMessage = SENDTPL % (userid,stype, filename)
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
	webservice.putheader("SOAPAction", url)

	webservice.endheaders() 
	webservice.send(SoapMessage) 
	# get the response 
	statuscode, statusmessage, header = webservice.getreply() 
	#print "Response: ", statuscode, statusmessage 
	#print "headers: ", header 
	results = webservice.getfile().read()
	print str(results)
	p1=re.compile(r'nfschina_getresourceResult>(.*?)nfschina_getresourceResult')
	p2=re.compile(r'(.*?)</')
	#p2=re.compile(r'<s0:string>(.*?)</s0:string>')
	p3=re.compile(r'<tns:string>(.*?)</tns:string>')
	match = p1.findall(results)
	print match
	server_ip = p2.findall(match[0])
	print server_ip
	if len(server_ip) == 0:
		server_ip = p3.findall(match[0])
	return server_ip
	


if __name__ == "__main__":
	ret = nfschina_getresource(9,1,"哈哈.mp4",'http://192.168.7.97/haha/service/wsdl')
	print ret
#	ip = nfschina_getresource(9,3,"guigu.mp4",'http://192.168.6.161/haha/service/wsdl')
#	print ip
