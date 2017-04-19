# Adam Bezak 1BIA - twitter.py

import os.path #  overenie existencie priecinkov a suborov a ich pripadne vytvorenie
import tweepy #  python kniznica pre pracu s Twitter API
import json # kniznica na pracu s datami vo formate JSON
import requests # kniznica na pracu s HTTP (konkretne na stahovanie web stranok)
import shutil # operacie so subormi, konkretne kopirovanie stiahnutej webstranky do lokalneho priecinka

twitter_profile_name = 'vossenwheels' 
sites_download_dir = 'sites'
tweets_filename = 'tweets.json'
# TWITTER potrebne veci pre vytvorenie vlastnej twitter applikacie tzv. API KLUCE
consumer_key = 'Yr7ku5JrcIaslnGD6COD6fxD8'
consumer_secret = 'dEjGypE8KAQ2wsTDA67Y20dRmtexpY3xwbBYKN18GiYusJgx7Q'
access_token = '196230163-hjgKmMQ1KizFd6MLuurV9EpmoA92BscdxRXrZ49z'
access_token_secret = 'guLizAtz0WtVGixnsZlBxfK17FQ8ocqPXYABa6GlunVhn'

# pouzitie twitter klucov
auth = tweepy.OAuthHandler(consumer_key, consumer_secret)
auth.set_access_token(access_token, access_token_secret)

# inicializacia tweepy objektu
api = tweepy.API(auth)

'''
if os.path.exists(tweets_filename):

	with open(tweets_filename, 'r') as tweets_file:
		content = tweets_file.read()
		json_tweets = json.loads(content)
		last_id = json_tweets[0].get('id_str', '')

else:

	last_id = ''
	json_tweets = []

print last_id
'''
json_tweets = [] 
public_tweets = api.user_timeline(twitter_profile_name) # volanie na tweepy objekt, ziadost o uzivatelovu nastenku


if len(public_tweets): # ak dany ucet ma nejake tweety na stiahnutie

	with open(tweets_filename, 'w') as f: # otvorenie suboru tweets.json

		if not os.path.exists(sites_download_dir): # ak dany priecinok neexistuje tak ho vytvorim
			os.makedirs(sites_download_dir)

		for tweet in public_tweets: 

			print '\n######### TWEET START #########'

			tweet_json = tweet._json # do premennej tweet_json si ulozim JSON objekt z aktualneho tweet objektu
			json_tweets.append(tweet_json) # do listu json_tweets pridam JSON objekt aktualneho tweetu na ktorom som 

			for url in tweet.entities.get("urls", []): # iteracia vsetkych URL ktore su v aktualnom tweete

				short_url = url.get('url') # do premennej short_url si ulozim skratenu verziu tej URL (napr http://t.co/5mylYBUOMN)
				expanded_url = url.get('expanded_url') #  do premennej expanded_url si ulozim plnu (neskratenu linku)
				url_id = short_url.split('t.co/', 1)[1] # rozdelim url podla separatora t.co, ulozim svoje id

				write_path = sites_download_dir + '/' + url_id + '.html' # do write_path si ulozim cestu pod akou ulozim aktualnu stranku, teda spojim sites_download dir, lomka, url_id a na koniec .html napr. napr sites/5mylYBUOMN.html

				print 'Downloading ' + expanded_url 
				response = requests.get(expanded_url, stream = True) #  stiahne danu stranku
				with open(write_path, 'wb') as out_file:
					response.raw.decode_content = True # nastaveni dekodovanie obsahu na true, tzn ze ak ma server napr gzip kompresiu tak python odpoved dekoduje
					shutil.copyfileobj(response.raw, out_file) # zkopiruje stiahnuty subor do download priecinku
				del response

			print '######### TWEET END ##########'

		json_tweets = json.dumps(json_tweets, indent=4) # list JSON objektov zoserializuje / zostringifikuje, tzn ze spravi z tych objektov jeden dlhy JSON string, indent = 4 znamena ze ma odsadzovat o 4 medzery, aby bol ten subor tweets.json citatelny pre ludi
		f.write(json_tweets) # zapise tento zoserializovany JSON do suboru
else:

	print 'No tweets to download'