#!/usr/bin/env python3
#Minimalizacia konecneho automatu - autor: Adam Bezak xbezak01
#MKA:xbezak01

import argparse
import sys
import pprint
import re
import traceback
from operator import itemgetter

# 0 - OK
# 1 - parametre
# 2 - chyba nacitania vstupu
# 3 - chyba pri otvoreni zapisu do suburu
# 4 - chybny format vstupneho suboru
# 60 - chybn format konecneho automatu
# 61 - semanticka chyba definice konecneho automatu
# 62 - konecny automat nieje dobre speci-fiikovan

# 
# Trieda reprezentujuca vynimku
# 
class ErrorWithCode(Exception) :

	## Konstruktor
	# @param self pointer na objekt
	# @param code error code
	def __init__(self, code):
		self.code = code

	def __str__(self):
		return repr(self.code)

# 
# Trieda reprezentujuca konecny automat
# 
class StateMachine(object) :

	## Konstruktor
    	#  @param self pointer na objekt
	def __init__(self) :
		self.next_state = 'start' #pociatocny stav konecneho automatu
		self.states = set() # mnozina stavov
		self.alphabet = set() # mnozina abecedy
		self.rules = {} # 'stav': [{'symbol': hodnota}]
		self.start_state = ' ' #startovaci stav
		self.end_state =  set() #mnozina koncovych stavov
		self.rules_basic = {}
		self.rules_list = [] # pravidla v liste tvaru stav'stav1
		self.not_finishing = [] # zoznam neukoncujich stavov
		self.rules_list_minimize = [] #pravidla v liste tvaru stav'a'stav1

	## Spracuje vstupny konecny automat
	# @param input_text vstupny konecny automat
	# @return 
	def run(self, input_text) :
		input_text_cut = ""
		while True :
			if (self.next_state == 'start') : # startovaci stav
				input_text = self.cut_start_end_whitespace(input_text)
				if (input_text[0] == '{') :
					self.next_state = 'load_states'
				else :
					raise ErrorWithCode(60)

			if (self.next_state == 'load_states') :
				input_states = input_text[input_text.find("{")+1:input_text.find("}")] # odrezem prvu { }
				if input_states == " " :
					raise ErrorWithCode(61)
				input_text_cut = input_text[input_text.find('}') + 1 : ] #zvysok
				self.states = self.get_states(input_states)
				self.next_state = 'alphabet'

			if (self.next_state == 'alphabet') :
				input_text_cut = self.cut_start_end_whitespace(input_text_cut) # osetrenie zaciatku a konca od medzer
				if (input_text_cut[0] != ',') : #  { } sa musia oddelovat ciarkov
					raise ErrorWithCode(60)
				else :
					input_text_cut = input_text_cut[1:] # odstranim ciarku a medzere znovu
					input_text_cut = self.cut_start_end_whitespace(input_text_cut)
					if (input_text_cut[0] != '{') : # zacina dalsi blok 
						raise ErrorWithCode(60)
				input_alphabet = input_text_cut[input_text_cut.find("{")+1:input_text_cut.find("}")] # odrezem prvu { }
				position = [pos for pos, char in enumerate(input_text_cut) if char == '}'] # najdem vsetky vyskyty }
				for x in position :
					if (input_text_cut[x + 1] != "'") :
						input_alphabet = input_text_cut[input_text_cut.find("{")+1:x] # musim odrezat len tu spravnu tj. NIE '}'
						input_text_cut = input_text_cut[x + 1 : ]
						break
				self.get_alphabet(input_alphabet) # ziskam abecedu
				self.next_state = 'rules'

			if (self.next_state == 'rules') :
				input_text_cut = self.cut_start_end_whitespace(input_text_cut) # osetrenie zaciatku
				if (input_text_cut[0] != ',') :
					raise ErrorWithCode(60)
				else :
					input_text_cut = input_text_cut[1:]
					input_text_cut = self.cut_start_end_whitespace(input_text_cut)
					if (input_text_cut[0] != '{') :
						raise ErrorWithCode(60)
				input_rules = input_text_cut[input_text_cut.find("{")+1:input_text_cut.find("}")] # odrezem prvu { }
				position = [pos for pos, char in enumerate(input_text_cut) if char == '}']
				for x in position :
					if (input_text_cut[x + 1] != "'") :
						input_rules = input_text_cut[input_text_cut.find("{")+1:x]
						input_text_cut = input_text_cut[x + 1 : ]
						break
				self.get_rules(input_rules)
				self.next_state = 'state_start'

			if (self.next_state == 'state_start') :
				input_text_cut = self.cut_start_end_whitespace(input_text_cut) # osetrenie zaciatku
				if (input_text_cut[0] != ',') :
					raise ErrorWithCode(60)
				else :
					input_text_cut = input_text_cut[1:]
					input_text_cut = self.cut_start_end_whitespace(input_text_cut)
					start = input_text_cut[:input_text_cut.find(',')]
					start = self.cut_start_end_whitespace(start)
					if (start in self.states) :
						self.start_state = start
						self.next_state = 'end_states'
					else :
						raise ErrorWithCode(61)

			if (self.next_state == 'end_states') :
				input_end_states = input_text_cut[input_text_cut.find('{') + 1: input_text_cut.find('}')]
				if input_end_states == " ":
					raise ErrorWithCode(62)
				self.end_state = self.get_states(input_end_states) # mozem vyuzit znovu get_states 
				if  (not (self.end_state.issubset(self.states))) :
					raise ErrorWithCode(61)
				return

	## Pomocny vypis startovacieho stavu
	# @return startovaci stav
	def print_start(self) :
		return self.start_state

	## Lexikalna, semanticka a syntakticka kontrola pravidiel konecneho automatu
	# @parma input_rules vstupne pravidla
	# return
	def get_rules(self, input_rules) :
		input_rules = input_rules.split(",")
		tmp_str = "" # stav'stav1
		tmp_str_minimize = "" # stav'a'stav1

		for item in input_rules :
			if (item.find("'")) :
				item = ''.join(item.split())
			dict_key = item[:item.find("'")] # 'state': [{'symbol0': value} ..]
			#pos = lambda x, y : x.find(y, x.find(y) + 1) # najde druhu '
			#pos = pos(item, "'")
			dict_key_value = item[item.find("'") + 1: item.rfind("'")]
			dict_value = item[item.find('>') + 1:]
			#print (self.alphabet)
			if (dict_key_value == '') : # nemoze mat epsilom prechod
				raise ErrorWithCode(62)
			elif (len(dict_key_value) > 1 and dict_key_value != "''") or (dict_value == '') or (dict_key == '') :
				raise ErrorWithCode(60)
			if (dict_key not in self.states) or (dict_key_value not in self.alphabet) or (dict_value not in self.states) :
				raise ErrorWithCode(61)

			bool_true = 'false'

			if (dict_key in self.rules) :
				for k, v in self.rules.items() : # ak uz tam je tak pass
					if (k == dict_key) :
						for x in v:
							for i, y in x.items() :
								if (dict_key_value == i and dict_value == y) :
									bool_true = 'true'
									break
								if (dict_key_value == i and dict_key == k) : # nedeterminizmus
									raise ErrorWithCode(62)
				if bool_true == 'false' :
					self.rules[dict_key].append({dict_key_value : dict_value})
			else :
				self.rules[dict_key] = [{dict_key_value : dict_value}]

			self.rules_basic.setdefault(dict_key, []).append(dict_value) # pre lepsiu orientaciu v pravidlach zapis {od kade:kam}
			tmp_str = dict_key + "'" + dict_value
			# print ("key",dict_key)
			# print ("value",dict_key_value)
			# print ("val",dict_value)
			tmp_str_minimize = dict_key + "'" + dict_key_value + "'" + dict_value
			self.rules_list.append(tmp_str)
			if tmp_str_minimize not in self.rules_list_minimize :
				self.rules_list_minimize.append(tmp_str_minimize)
		return

	## Pomocna metoda na vypis pravidiel
	# @return
	def print_rules(self) :
		for k,v in self.rules.items() :
			print ('Pravidlo: ' , k)
			print (v)

	## Lexikalna, syntakticka kontrola vstupnej abecedy
	# @param input_alphabet vstupna abeceda konecneho automatu
	# @return
	def get_alphabet(self, input_alphabet) : 
		list_alphabet = ''.join(input_alphabet.split()) # odstranim medzere
		if not list_alphabet :
			raise ErrorWithCode(61)

		list_alphabet = list_alphabet.split("','") # delimeter je ',' pretoze ako abeceda moze byt aj ciarka
		list_alphabet[0] = list_alphabet[0][1:] # odstranim ' '
		list_alphabet[-1] = list_alphabet[-1][:-1]

		for item in list_alphabet :
			if (item == '' or item == "'") :
				raise ErrorWithCode(60)
			#print (item)
			#item = item.replace("''", "'")
			if (len(item) > 1 and item != "''") :
				raise ErrorWithCode(60)
			self.alphabet.add(item)
		return

	## Pomocna metoda na vypis abecedy
	# @return abeceda
	def print_alphabet(self) :
		return self.alphabet

	## Lexikalna a syntakticka kontrola vstupnych pravidiel
	# @param input_states vstupne pravidla
	# @return mnozina pravidiel
	def get_states(self, input_states) :
		tmp_states = set() # mnozina stavov
		if not input_states :
			raise ErrorWithCode(61)
		list_states = input_states.split(',') # delimeter
		if not list_states : # bez stavov
			raise ErrorWithCode(61)
		for item in list_states :
			item = self.cut_start_end_whitespace(item)
			if item:
				if (item[0] != '_' and ('a' <= item[0] <= 'z' or 'A' <= item[0] <= 'Z') and not item[0].isdigit()) : # zaciatok
					if (item[len(item) - 1] != '_' and ' ' not in item) : # stred a koniec
						for c in item[1:]:
							if (c.isalnum() or c == '_') :
								continue
							else :
								raise ErrorWithCode(60)
						tmp_states.add(item)
					else :
						raise ErrorWithCode(60)
				else :
					raise ErrorWithCode(60)
			else :
				raise ErrorWithCode(60)
		return tmp_states

	## Metoda odstrani pociatocne a koncove biele znaky v stringu
	# @param text odstranenie v texte
	# @return text bez uvodnych a koncovych medzier
	def cut_start_end_whitespace(self, text) :
		text = text.lstrip() # odstranim na zaciatku medzery
		text = text.rstrip() # koncove medzery
		return text

	## Pomocna metoda na vypis stavov
	# @return stavy
	def print_state(self) :
		return self.states

	## Pomocna metoda na vypis koncovych stavov
	# @return stavy
	def print_end_state(self) :
		return self.end_state

	## Normalizovany vypis
	# @param output_file vystupny subor (popr. stdout)
	# @return
	def print_normalize(self, output_file) :

		output_file.write('(\n{')
		output_file.write(', '.join(sorted(self.states)))
		output_file.write('},\n{')
		output_file.write('\'' + '\', \''.join(sorted(self.alphabet)) + '\'')
		output_file.write('},\n{\n')
		arr = []
		for x in self.rules:	# kluc
			for y in self.rules[x]:
				for z in y:		 # kluc/hodnota
					arr.append([x, z, y[z]])  # hodnota
		output_file.write(',\n'.join([ x[0] + ' \'' + x[1] + '\' -> ' + x[2] for x in sorted(arr, key=itemgetter(0, 1, 2))]))
		output_file.write('\n},\n'+self.start_state+',\n{')
		output_file.write(', '.join(sorted(self.end_state)))
		output_file.write('}\n)')

	## Kontrola dobre specifikovaneho konecneho automatu
	# @return
	def is_DSKA(self) : # kontrola nedostupnych stavov, a nedosiahnutelnych stavov (trap)
		tmp_set = set()
		tmp_states = set()
		tmp_dosiahnute = set()
		tmp_dict = {}

		for x in self.rules:
			for y in self.rules[x]:
				for z in y:
					tmp_set.add(y[z])
					if (x == self.start_state) :
						tmp_states.add(y[z])
						tmp_dosiahnute.add(y[z])

		for k,v in self.rules.items() :
			if (len(v) != len(self.alphabet)) :
				raise ErrorWithCode(62)

		to_visit = []
		to_visit.append(self.start_state) # zacnem startovacim stavom
		visited = []

		while len(to_visit) != 0 : # ziadne nedostupne stavy ( zo startu sa tam nemozem dostat 
			actual = to_visit.pop()
			visited.append(actual)
			for x in self.rules_basic :
				if x == actual :
					for y in self.rules_basic[x] :
						if y not in visited :
							to_visit.append(y)

		if (set(visited) != self.states) :
			raise ErrorWithCode(62)

		counter  = 0

		for stav in self.states : #### kontrola neukoncujucich stavov max 0-1
			tmp_list = []
			tmp_list.append(stav)
			for y in tmp_list :
				for x in self.rules_list :
					left = x[:x.find("'")]
					right = x[x.find("'") +1 :]
					if (left == y) :
						if (right not in tmp_list) :
							tmp_list.append(right)

			if not (set(tmp_list).intersection(self.end_state)) : # prienik pravej strany s mnozinou koncovych stavov
				counter += 1
				self.not_finishing = tmp_list

		if (counter > 1) : # jeden stav je OK
			raise ErrorWithCode(62)

	## vypis neukoncujuceho stavu
	#@return neukoncujuci stav
	def print_non_finishing(self) :
		if not self.not_finishing :
			self.not_finishing.append(str(0))

		return self.not_finishing[0]

	##Minimalizacia konecneho automatu
	# @return
	def FSM_minimize(self) :
		all_states = [list(self.end_state), list(self.states.difference(self.end_state))] # [[koncove stavy], [ostatne stavy]]s
		br = False
		while True:
			br = False
			for jeden in all_states:
				for a in self.alphabet :
					tmp_left = []
					tmp_right = []
					tmp_stav = []
					for item in jeden :
						for x in self.rules_list_minimize :
							if (item  == x[:x.find("'")]) :
								pos = lambda x, y : x.find(y, x.find(y) + 1) # najde druhu '
								pos = pos(x, "'")
								findd = x[x.find("'") + 1 : pos]
								if (a == findd) :	 # vyberem len to pravidlo s danym znakom z abeccedy						
									tmp_right.append(x[pos + 1 :]) # kam idem
									tmp_stav.append(x[ : x.find("'") :] + "'" + x[pos + 1 :]) # prava strana
									tmp_left.append(x[ : x.find("'")])	 # lava strana
					for item in all_states :
						intersekcia = set(tmp_right).intersection(set(item))	# prienik pravych stran s mnozinou
						if (len(intersekcia) > 0) and (len(intersekcia) < len(tmp_right)) : # ak ma nastat stieenie musi byt prienik vacsi ako 0 a dlzka mensia ako dlzka pravych stran
							tmp_pod = set()
							zostatok = set(tmp_right) - set(intersekcia)
							intersekcia2 = set(zostatok).intersection(set(item))
							if (len(intersekcia2) == 0) :
								for i in intersekcia : # k pravej strane musim najst aj zodpovedajucu lavu stranu
									for y in tmp_stav :
										prava_strana = y[y.find("'")  + 1: ]
										if prava_strana == i :
											lava_strana = y[ : y.find("'")]
											tmp_pod.add(lava_strana)

								for item in all_states :
									if not item :
										all_states.remove(item)
								rozdiel =set(tmp_left) - tmp_pod
								if rozdiel :
									all_states.append(list(tmp_pod))
									all_states.append(list(set(tmp_left) - tmp_pod))
									all_states.remove(tmp_left)
								else :
									break
								br = True
								break
					if br :
						break
				if br :
					break
			if br :
				continue
			break
		new_rules = {}

		tmp_new_rules = []
		# stiepenie stavov
		for jeden in all_states :
			for a in self.alphabet :
				tmp_left = []
				tmp_right = []
				tmp_new_ruls = []
				for item in jeden : 
					for x in self.rules_list_minimize :
						if (item  == x[:x.find("'")]) :
							#pos = lambda x, y : x.find(y, x.find(y) + 1) # najde druhu '
							#pos = pos(x, "'")
							findd = x[x.find("'") + 1 : x.rfind("'")]

							if (a == findd) :	 # vyberem len to pravidlo s danym znakom z abeccedy
								right = x[x.rfind("'") + 1:]
								tmp_stav.append(x[ : x.find("'") :] + "'" + x[pos + 1 :]) # prava strana
								tmp_left.append(x[ : x.find("'")])	 # lava strana
								for item2 in all_states:
									if right in item2:
										tmp_right = item2
				right = ""
				left = ""
				right = ('_'.join(sorted(tmp_right)))
				left = ('_'.join(sorted(tmp_left)))
				bool_true = False
				if (left in new_rules) :
					for k, v in new_rules.items() :
						if (k == left) :
							for x in v :
								for i, y in x.items() :
									if (a == i and right == y) :
										bool_true = True
										break
					if bool_true == False :
						new_rules[left].append({a : right}) #pripojim k existujucim stavom
				else :
					new_rules[left] = [{a : right}] # pridam nove stavy

		Qm = set()
		new_start_state = ""
		new_end_state = set()
		for item in all_states :
			stav = '_'.join(sorted(item))
			if self.start_state in  item :
				new_start_state = stav
			for i in self.end_state :
				if i in item :
					new_end_state.add(stav)
			Qm.add(stav)
		# nove komponenty
		self.states = Qm
		self.start_state = new_start_state
		self.rules = new_rules
		self.end_state = new_end_state

	##Metoda na porovnavanie dvoch listov
	# @return true ak su zhodne, false ak nie
	def comp(self, list1, list2) :
		for val in list1:
			if val in list2 :
				return True
		return False
# 
# Spracovanie argumentov
# @return args argumenty
#
def get_args() :
	parser = argparse.ArgumentParser()
	parser.add_argument("--input", help="zadaný vstupní textový soubor filename v UTF-8 s popisem dobře speci-fikovaného konečného automatu.", type=str, required=False, default="sys.stdin")
	parser.add_argument("--output" , help="textový výstupní soubor filename (opět v UTF-8) s popisem výsledného ekvivalentního9 konečného automatu v předepsaném formátu výstupu.", type=str, required=False, default="sys.stdout")
	parser.add_argument("-f", "--find-non-finishing", dest="find", action='store_true', help="hledá neukončující stav")
	parser.add_argument("-m", "--minimize ", dest="minimize", action='store_true', help="provede minimalizaci")
	parser.add_argument("-i", "--case-insensitive", dest="case", action='store_true', help="nebude brán ohled na velikost znaků")
	args = parser.parse_args()

	return args

# 
# Otvorenie suboru
# @param type typ otvorenia suboru
# @return dany file
#
def open_file(input, type) :
	if (type == 'input') :
		try:
			input_file = open(input, 'r')
		except:
			print ("Error code: 2", file=sys.stderr)
			sys.exit(2)
		return input_file
	if (type == 'output') :
		try:
			output_file = open(input, 'w')
		except:
			print ("Error code: 3", file=sys.stderr)
			sys.exit(3)
		return output_file

# 
# Nacita vstup zo vstupneho suboru
# @param input_file vstupny subor
# @return string vstupneho automatu
#
def get_text(input_file) :
	try:
		input_file_text = input_file.read()
	except:
		print ("Error code: 2", file=sys.stderr)
		sys.exit(2)
	return input_file_text

#
# Odstrani prebytocne komentare
# @param input_file_text zadany vstupny konecny automat
# @return vstupny konecny automat bez komentarov
#
def remove_comments(input_file_text) :
	index = 0
	authomat = "" #input_file_text bez komentarov
	while True :
		if index >= len(input_file_text) :
			break

		if ((input_file_text[index] == '#') and (input_file_text[index - 1] != "'")) : # znak # moze byt pouzity aj ako validny v tvare '#'
			while True :
				if ((input_file_text[index] == '\n') or (input_file_text[index] == '\r')): #ak nacitam koniec riadku tak break a posun na novy znak (riadok)
					index += 1
					break
				index += 1
				if index >= len(input_file_text) :
					break
			continue

		if (input_file_text[index] == '\n') : # nove riadky nepotrebujem
			index += 1
			continue
		authomat += str(input_file_text[index]) # dam to dokopy do stringu
		index += 1

	index = 0
	authomat = authomat.lstrip() # odstranim na zaciatku medzery
	authomat = authomat.rstrip() # koncove medzery
	if authomat.startswith('(') and authomat.endswith(')') : # skontrolujem zaciatok '(' a koniec ')'
		authomat = authomat[1:-1]
		return authomat
	else :
		raise ErrorWithCode(60)

# 
# Validacia zadaneho koneneho automatu
# @param authomat Vstupny konecny automat
# @return objekt reprezentujuci konecny automat
#
def parse_authomat(authomat) :

	FSM = StateMachine()

	FSM_OBJ = FSM.run(authomat)

	return FSM

if __name__ == '__main__' :
	try :
		args = get_args() # argparse
	except SystemExit as e:
		if int(str(e)) == 0 :
			sys.exit(0)
		else :
			print ("Error code: 1", file=sys.stderr)
			sys.exit(1)		

	# kontrola duplicity argumentov, kedze argparse podporuje az od verzie >3.5
	try :
		if (len(sys.argv[1:]) != len(set(sys.argv[1:]))) :
			raise ErrorWithCode(1)
		elif (('-f') in sys.argv[1:])  and (('--find-non-finishing') in sys.argv[1:]):
			raise ErrorWithCode(1)
		elif (('-i') in sys.argv[1:])  and (('--case-insensitive') in sys.argv[1:]):
			raise ErrorWithCode(1)
		elif (('-m') in sys.argv[1:])  and (('--minimize') in sys.argv[1:]):
			raise ErrorWithCode(1)
		elif args.minimize and args.find :
			raise ErrorWithCode(1)

		match_input = [s for s in sys.argv[1:] if "--input" in s]
		match_output = [s for s in sys.argv[1:] if "--output" in s]

		if (len(match_input) > 1) or (len(match_output) > 1) :
			raise ErrorWithCode(1)

	except ErrorWithCode as e :
		print ("Error code:", e.code, file=sys.stderr)
		sys.exit(e.code)

	if (args.input != 'sys.stdin') :
		input_file = open_file(args.input, 'input') #otvorenie suboru
		input_file_text = get_text(input_file)
	else :
		input_file_text = sys.stdin.read() #ak nebol zadany vstupny subor tak beriem stdin

	if args.case :
		input_file_text = input_file_text.lower() # v pripade zadania --case-insensitive alebo -i

	try :
		input_file_text = remove_comments(input_file_text) # odstranenie komentarov
	except ErrorWithCode as e :
		for frame in traceback.extract_tb(sys.exc_info()[2]):
			fname,lineno,fn,text = frame
			print ("Error in %s on line %d" % (fname, lineno), file=sys.stderr)
		print ("Error code:", e.code, file=sys.stderr)
		sys.exit(e.code)

	try :
		FSM = parse_authomat(input_file_text) # spracovanie zadaneho konecneho automatu
		#print ('\n')
		#print ("Stavy: ", FSM.print_state())
		#print ("Abeceda: ", FSM.print_alphabet())
		#FSM.print_rules()
		#print ("Startovaci stav: ", FSM.print_start())
		#print ("Koncove stavy: ", FSM.print_end_state())
		FSM.is_DSKA() #validacia Dobre Specifikovaneho Konecneho Automatu
		#print ('\n')

	except ErrorWithCode as e :
		for frame in traceback.extract_tb(sys.exc_info()[2]):
			fname,lineno,fn,text = frame
			print ("Error in %s on line %d" % (fname, lineno), file=sys.stderr)
		print ("Error code:", e.code, file=sys.stderr)
		sys.exit(e.code)

	if args.minimize : # minimalizacia DSKA
		FSM.FSM_minimize()

	if (args.output != 'sys.stdout') : # zapis do suboru
		output_file = open_file(args.output, 'output')
	else :
		output_file = sys.stdout

	if args.find :
		output_file.write(FSM.print_non_finishing())
	else :
		FSM.print_normalize(output_file)

	if 'input_file' in locals(): # zatvorenie suborov
		input_file.close()
		
	if args.output :
		output_file.close()