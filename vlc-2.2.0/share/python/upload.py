
import urllib2 ,requests,time, threading,thread
import sys, httplib, re, urllib2,os

global loading
loading = [['0' for col in range(1)] for row in range(30)]
#loading = [[0 for col in range(0)] for row in range(30)]
lock = thread.allocate_lock()

global index
index=0
global process
process = [0 for col in range(30)]
global exitflag
exitflag=[0 for col in range(30)]
global datamutex
datamutex = [threading.Lock() for col in range(30)]

global mutex
mutex = threading.Lock()




def parse_url(url):
	s1 =  url.split("//")
	s2 = s1[1].split("/")
	s3 = s1[1].split("/",1)
	return s3

def nfschina_upload_file(theindex,userid, filename, filesize, url, block_size=1024*1024*2, finish=0):
	global loading
	SENDTPL = \
'''<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope xmlns:ns0="http://schemas.xmlsoap.org/soap/envelope/" xmlns:ns1="nfschina.com" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/">
<SOAP-ENV:Header/>
<ns0:Body>
	<ns1:nfschina_upload_file>
		<ns1:userid>%d</ns1:userid>
		<ns1:filename>%s</ns1:filename>
		<ns1:filesize>%d</ns1:filesize>
		<ns1:block_size>%d</ns1:block_size>
		<ns1:finish>%d</ns1:finish>
	</ns1:nfschina_upload_file>
</ns0:Body>
</SOAP-ENV:Envelope>'''
	SoapMessage = SENDTPL % (userid, filename, filesize, block_size, finish )

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
	results =  webservice.getfile().read()

	print str(results)

	p1=re.compile(r'nfschina_upload_fileResult>(.*?)nfschina_upload_fileResult')
	p2=re.compile(r'<s0:string>(.*?)</s0:string>')
	p3=re.compile(r'<tns:string>(.*?)</tns:string>')
	match = p1.findall(results)
	loading[theindex] = p2.findall(match[0])
	if len(loading[theindex]) == 0 :
		loading[theindex] = p3.findall(match[0])
	print loading[theindex]


def post_file(theindex,file_path,filename,userid,block_size=2*1024*1024,url='http://192.168.7.97/haha/service/upload_file'):
	global loading,lock, process,datamutex
#	print loading
	sfile = open(file_path,"rb")
	offset = 0
#	lock.acquire()
	for load in loading[theindex]:
		if isStop(theindex) == 1:
			return
		lock.acquire()
		if loading[theindex][offset] == '0':
			loading[theindex][offset] = '2'
		else:
			offset=offset+1
			lock.release()
			continue
		lock.release()
		lock.acquire()
		datamutex[theindex].acquire()
		#process[theindex] = (offset*100)/len(loading[theindex])
		process[theindex] = float(offset)/float(len(loading[theindex])-1)*100
		datamutex[theindex].release()
		lock.release()
		print str(offset)
		sfile.seek(block_size*offset)
		tfb = sfile.read(block_size)
		files = {"file":tfb}
		payload = {'userid': userid, 'filename':filename, 'offset':offset,'block_size':block_size}
		result = requests.post(url,payload,files=files)

		lock.acquire()
#		print str(result.text)
		if result.text == str(1):
			if loading[theindex][offset] == '2':
				loading[theindex][offset] = '1'
				offset=offset+1
		lock.release()
	print "thread end"





def get_process(index):
	global process,datamutex
	datamutex[index].acquire()
	processflag = process[index]
	datamutex[index].release()
#	print str(process)
	return processflag

def isStop(index):
	global exitflag
	global datamutex

	stopflag = 0
	datamutex[index].acquire()
	stopflag = exitflag[index]
	datamutex[index].release()
	return stopflag




def upload_post(theindex, userid, filepath, filename, url):
	global loading
	filesize = os.path.getsize(filepath)
	block_size = 2*1024*1024
#	userid, filename, filesize, url, block_size=1024*1024*2, finish=0
	nfschina_upload_file(theindex,userid, filename, filesize, url,block_size, 0)
#	print loading
	t1 = threading.Thread(target=post_file,args=(theindex,filepath,filename,userid,))
	t1.start()
	t2 = threading.Thread(target=post_file,args=(theindex,filepath,filename,userid,))
	t2.start()
	
	while t1.isAlive() or t2.isAlive():
		if isStop(theindex) == 1:
			return
	#	time.sleep(0.1)
		continue
	print "111111"
#	print loading
	nfschina_upload_file(theindex,userid, filename, filesize, url=url, finish=1)


def start_upload(userid, filename, filepath, url):
	global index
	global mutex
	global datamutex
	global exitflag
	global process

	theindex = 0
	mutex.acquire()
	theindex = index
	index = index + 1
	mutex.release()

	datamutex[index].acquire()
	process[theindex] = 0
	loading[theindex] = ['0']
	exitflag[theindex] = 0
	datamutex[index].release()

	s1 = threading.Thread(target=upload_post,args=(theindex, userid, filepath, filename, url,))
	s1.start()
	return theindex	


def stop_upload(index):
	global exitflag
	global datamutex

	datamutex[index].acquire()
	exitflag[index]=1
	datamutex[index].release()
	print "index=%d threads is exited"%(index)

if __name__ == '__main__':
#	s1 = threading.Thread(target=start_upload,args=(9,"haha.mp4","/home/nfschina/forDjangoClient/haha.mp4","http://127.0.0.1:8000/haha/service/wsdl"))
	start_upload(1,"haha.mp4","/home/lili/haha.mp4","http://192.168.7.97/haha/service/wsdl")
	start_upload(1,"hehe.mp4","/home/lili/hehe.mp4","http://192.168.7.97/haha/service/wsdl")

	print "sleep...."
	while 1:
		print "0 -> %d" % (get_process(0))
		print "1 -> %d" % (get_process(1))
		time.sleep(1)
#	stop_download(0)
#	print  get_process(0)
	
