import urllib2 
import sys, httplib 
import re
def nfschina_login(username,password): 
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
    webservice = httplib.HTTP("192.168.7.97") 
    webservice.putrequest("POST", "/haha/service/?wsdl")
    webservice.putheader("Host", "192.168.7.97") 
    webservice.putheader("User-Agent", "Python Post") 
    webservice.putheader("Content-type", "text/xml; charset=\"UTF-8\"") 
    webservice.putheader("Content-length", "%d" % len(SoapMessage)) 
    webservice.putheader("SOAPAction", "\"http://192.168.7.97/haha/service/?wsdl\"") 
    webservice.endheaders() 
    webservice.send(SoapMessage) 
    # get the response 
    statuscode, statusmessage, header = webservice.getreply() 
   # print "Response: ", statuscode, statusmessage 
    #print "headers: ", header 
    #print webservice.getfile().read() 
    msg = webservice.getfile().read()
    p1=re.compile(r'nfschina_loginResult>(.*?)nfschina_loginResult>')
    p2=re.compile(r'(.*?)</')
    match = p1.findall(msg)
    match1 = p2.findall(match[0])
    print match
    print int(match1[0])
    return int(match1[0])


#if __name__ == "__main__":
#	nfschina_register("lili","123456")
