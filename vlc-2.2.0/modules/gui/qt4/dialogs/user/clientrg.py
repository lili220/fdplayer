import urllib2 
import sys, httplib 
import re
import requests
def nfschina_register(username,password): 
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
    p1=re.compile(r'nfschina_registerResult>(.*?)nfschina_registerResult>')
    p2=re.compile(r'(.*?)</')
    match = p1.findall(msg)
    match1 = p2.findall(match[0])
    print match
    print int(match1[0])
    return int(match1[0])

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

def nfschina_listmyfile(userid): 
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
    print "Response: ", statuscode, statusmessage 
    print "headers: ", header 
    results = webservice.getfile().read() 
    print results
    #p = re.compile(r'(?<=<tns:string>)(.*?)(?=</tns:string>)')
    p = re.compile(r'(?<=<tns:string>)(.*?)(?=</tns:string>)')
    '''<tns:string>guigu.mp4</tns:string> '''
    match = p.findall(results)
    return match

def nfschina_upload(userid,filename,filepath): 
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
    print "Response: ", statuscode, statusmessage 
    print "headers: ", header 
    print webservice.getfile().read()

    ret = 0
    url = 'http://192.168.7.97/haha/service/uploadfile'
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
            #print result.text
            if int(result.text) == int(1):
                print "success upload file"
                ret = 1
                break
	file1.close
    print ret
    return ret

def nfschina_delete(userid, filename): 
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
    print "Response: ", statuscode, statusmessage 
    print "headers: ", header 
    print webservice.getfile().read() 


#if __name__ == "__main__":
#	nfschina_register("lili","123456")
