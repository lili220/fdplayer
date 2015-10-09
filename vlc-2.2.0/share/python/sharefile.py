__author__ = 'wp'
# coding="utf-8"

import urllib2
import re

def getfile(ip,myid,targetid):
	url='http://' + ip + '/browse/' + myid + '/' + targetid + '/'
	html=urllib2.urlopen(url).read()
	find=re.compile(u'(?<=<filename>)(.*?)(?=</filename>)')
	filename = find.findall(html)
	return filename
    # except URLError, e: 
    # 	return e.reason 

    


# if __name__ == "__main__":
# 	#print(getfile('192.168.7.88:8090','1001','1102'))
#     c = getfile('192.168.7.88:8090','1001','1102')
#     # str = "".join(c)
#     for f in c:
#         print f
    # print str



