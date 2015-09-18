__author__ = 'wp'

import urllib2
import sys, httplib
import re


def nfschina_msg(userid,max_items):
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
    webservice = httplib.HTTP("192.168.7.97")
    webservice.putrequest("POST", "/haha/service/wsdl")
    webservice.putheader("Host", "192.168.7.97")
    webservice.putheader("User-Agent", "Python Post")
    webservice.putheader("Content-type", "text/xml; charset=\"UTF-8\"")
    webservice.putheader("Content-length", "%d" % len(SoapMessage))
    webservice.putheader("SOAPAction", "\"http://192.168.7.97/haha/service/wsdl\"")
    webservice.endheaders()
    webservice.send(SoapMessage)
    # get the response
    statuscode, statusmessage, header = webservice.getreply()
    # print "Response: ", statuscode, statusmessage
    # print "headers: ", header
    # return webservice.getfile().read()
    # print webservice.getfile().read()
    msg = webservice.getfile().read()
    p1=re.compile(r'(?<=<tns:string>)(.*?)(?=</tns:string>)')
    a = p1.findall(msg)
    tmp = []
    for i in range(0, len(a)):
        # b = a[i].split()[]
        tmp.append(str( a[i].split()[2]))
    return tmp



# if __name__ == "__main__":
# 	print(nfschina_login(34,2))[1]
