__author__ = 'wp'

import urllib2
import sys, httplib
import re
import requests

def parse_url(url):
    s1 =  url.split("//")
    s2 = s1[1].split("/")
    s3 = s1[1].split("/",1)
    return s3
	
def nfschina_msg(userid,max_items,url):
    SENDTPL = \
'''<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope xmlns:ns0="http://schemas.xmlsoap.org/soap/envelope/" xmlns:ns1="nfschina.com" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/">
   <SOAP-ENV:Header/>
   <ns0:Body>
      <ns1:nfschina_getonline>
         <ns1:userid>%d</ns1:userid>
         <ns1:max_items>%d</ns1:max_items>
      </ns1:nfschina_getonline>
   </ns0:Body>
</SOAP-ENV:Envelope>'''
    SoapMessage = SENDTPL % (userid, max_items)
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
    # print "headers: ", header
    # return webservice.getfile().read()
    #print webservice.getfile().read()
    msg = webservice.getfile().read()
    '''<s0:string>192.168.7.88 8090 2337 1102 1102 192.168.7.88 33611</s0:string>'''
    p1=re.compile(r'(?<=<tns:string>)(.*?)(?=</tns:string>)')
    p2=re.compile(r'(?<=<s0:string>)(.*?)(?=</s0:string>)')
    a = p1.findall(msg)
    if len(a) == 0:
        a = p2.findall(msg)
    print a
    tmp = []
    for i in range(0, len(a)):
        tmp.append(str( a[i].split()[0])+" "+str( a[i].split()[1])+" "+str( a[i].split()[4])+" "+str( a[i].split()[5])+" "+str( a[i].split()[6]))
    print tmp
    return tmp



if __name__ == "__main__":
    print(nfschina_msg(2,2,"http://192.168.7.97/haha/service/wsdl"))
