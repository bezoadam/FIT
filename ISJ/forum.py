# Adam Bezak 1BIA - forum.py

from bs4 import BeautifulSoup # parser na vybranie dat z HTML formatu
import requests # stahovanie dat stranok
import re # regularne vyrazy
import urlparse # praca s URL
import sys # argumenty

post_counter = 0

def getsubforum(url):

	r = requests.get(url)
	data = r.text
	soup = BeautifulSoup(data)
	list_link = []

	for link in soup.find_all('a',href=True): # najdem vsetky linky pomocou bs4
	    link['href'] = urlparse.urljoin(url, link['href']) 
	    links = link['href'] # ulozim ich do listu

	    for i in links.splitlines(): #iteracia

	    	if 'http://csko.cz/forum/forumdisplay.php?' in i: # ak sa zhoduje nazov na subforum appendnem do noveho listu v ktorom su len potrebne linky na subfora
	    		list_link.append(i)

	return list_link

def gettopic(url):

	global post_counter
	r = requests.get(url)
	data = r.text
	soup = BeautifulSoup(data)
	sub_forum_link = []
	sub_forums_links = []

	for link in soup.find_all('a', href=True):

		link['href'] = urlparse.urljoin(url, link['href'])
		links = link['href']

		for i in links.splitlines():

			if 'http://csko.cz/forum/showthread.php?' in i:

				re = requests.get(i)
				data = re.text
				soup = BeautifulSoup(data)
				if post_counter % 2 == 0:
					gettext(i)
				post_counter = post_counter + 1
				print 'number of downloaded posts:', post_counter
				sub_forums_links.append(i)

	return sub_forums_links

def gettext(url):

	url2 = url.encode('utf-8')
	text_file.write("++++++++++++++POST LINK++++++++++++++\n")
	text_file.write(url2 + '\n')
	r = requests.get(url)
	data = r.text
	soup = BeautifulSoup(data)
	member_count = 0
	thread_title = soup.findAll(attrs={'class':'threadtitle'}) # najde vsetky nazvy topicov

	for row in thread_title:
		text = ''.join(row.findAll(text=True))
		data = text.strip() # stripovanie textu od zbytocnych tagov
		text_file.write("*************THREAD NAME*************\n")
		data = data.encode('utf-8') # prekodovanie na utf-8, pre lepsi zapis do suboru
		text_file.write(str(data) + '\n')

	post_owner = soup.findAll(attrs={'class':'username offline popupctrl'})
	post_text = soup.findAll(attrs={'class':'postcontent restore '})
	post_date = soup.findAll(attrs={'class':'posthead'})

	for row1 in post_owner:

		text = ''.join(row1.findAll(text=True))
		data1 = text.strip()
		text_file.write("\t*************USER NAME*************\n")
		data1 = data1.encode('utf-8')
		text_file.write('\t' + str(data1) + '\n')

		text = ''.join(post_text[member_count].findAll(text=True))
		data1 = text.strip()
		text_file.write("\t*************TEXT*************\n")
		data1 = data1.encode('utf-8')
		data1 = "".join([s for s in data1.splitlines(True) if s.strip("\r\n")])
		data1 = data1.replace('\n', '\n\t')
		text_file.write('\t' + str(data1) + '\n')

		text = ''.join(post_date[member_count].findAll(text=True))
		data1 = text.strip()
		text_file.write("\t*************TIME AND DATE + ID*************\n")
		data1 = data1.encode('utf-8')
		data1 = "".join([s for s in data1.splitlines(True) if s.strip("\r\n")])
		data1 = data1.replace('\n', '\n\t')
		text_file.write('\t' + str(data1) + '\n')
		text_file.write('\n')	
		member_count = member_count + 1

#kontrola vstupneho argumentu
p = len(sys.argv)
if p == 2:
	s = sys.argv[1]
	if (int(s) > 0) and (int(s) < 1500):
		maximum_posts = sys.argv[1]
	else:
		print 'WRONG ARGUMENTS'
		exit(1)
else: 
	print 'WRONG ARGUMENTS'
	exit(1)

text_file = open("output.txt", "w")

url = ('http://csko.cz/forum/')
sub_forum = getsubforum(url)
sub_forum.pop(0)
for topic_url in sub_forum: 

	if (not(re.search("forumdisplay.php\?104-|forumdisplay.php\?3-",topic_url)) and (re.search("forumdisplay.php\?",topic_url))): # musim vynechat tieto linky z dovodu ze sa duplikuju

		if post_counter < int(maximum_posts): 

			print '######### DOWNLOADING #########'
			print topic_url
			urls = gettopic(topic_url)
		else:

			print '######### FINISHED #########'
			break
text_file.close()