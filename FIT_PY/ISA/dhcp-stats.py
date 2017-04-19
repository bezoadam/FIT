#!/usr/bin/env python
'''
Adam Bezak - xbezak01
05 - ACK
07 - RELEASE
'''
 
import socket
from struct import *
import pcapy
from pcapy import findalldevs
import sys
import string
import curses
import timeit

global dic
global ncurses
global dic_used
global start

dic_used = {}
dic = {}

# class represented custom exception
class ErrorWithCode(Exception):

    ## Constructor
    # @param self pointer na objekt
    # @param code error code
    def __init__(self, code):
        self.code = code

    def __str__(self):
        return repr(self.code)

#class for terminal ncurses module
class ncurses():

    ## Constructor
    # @param self pointer na objekt
    # @param stdscr ncurses screen
    def __init__(self):
        self.stdscr = curses.initscr()

    def curses_main(self):

        global dic

        self.stdscr.clear()
        curses.noecho()
        #curses.cbreak()
        #curses.curs_set(0)
        self.stdscr.keypad(1)

        #create custom table
        self.stdscr.addstr(0,0, "IP Prefix")
        self.stdscr.addstr(0,20, "Max hosts")
        self.stdscr.addstr(0,35, "Allocated adresses")
        self.stdscr.addstr(0,60, "Utilization")

        for index, i in enumerate(dic['IP Address']):

            self.stdscr.addstr(index + 1,0, i)
            self.stdscr.addstr(index + 1,20, dic['Max host'][index])
            self.stdscr.addstr(index + 1,35, dic['Allocated'][index])
            self.stdscr.addstr(index + 1,60, dic['Utilization'][index] + '%')
        
        curses.endwin()

    def stdscr_getch(self):
        self.stdscr.refresh()

    def curses_end(self):
        curses.endwin()
    
def parse_data(ip_data_bin, op, dec_time):
    """
    Parse data from packet.
    Parameters:
      ip_data_bin - example:['c0', 'a8', '63', '14']
      op - type of DHCP, 05 for ACK, 07 for RELEASE
      dec_time - lease time
    """
    global dic
    global ncurses
    global dic_used
    global start
    if ip_data_bin:
        #ip from packet in bin
        ip_data_bin_str = ''
        #ip from packet in dec
        #ip_data_dec_str example: 192.168.1.10
        ip_data_dec_str = ''

        #convert
        for z in ip_data_bin:
            ip_data_bin_str = ip_data_bin_str + toBin(z,16)
            if not ip_data_dec_str :
                ip_data_dec_str = str(getTime(z))
            else:
                ip_data_dec_str = ip_data_dec_str + "." + str(getTime(z))
    
        #if something usefull is in DHCP packet (IP ADDRESS)
        if ip_data_bin_str != '00000000000000000000000000000000':
            tmp_list = []
            #j example: ip
            for index, (j) in enumerate(dic['IP Address']):
                #check if ip is valid (if already used on DHCP server)
                bl = None
                for x in dic_used[j]:
                    if ip_data_dec_str in x.keys() and op == '05':
                        bl = True
                    if op == '07' :
                        if not any(ip_data_dec_str in d for d in dic_used[j]):
                            bl = True
                if bl :
                    continue
                tmp_str = ''
                #split ip adress based dot (.) example: 192    168     1     0
                for y in j[:j.rfind('/')].split('.'):
                    tmp_str = tmp_str + toBin(y,10)
                #tmp_str - every octet in binary form
                prefix = j[j.find('/') + 1:]
                #compare two ip address of network in binary format
                if ip_data_bin_str[:int(prefix)] == tmp_str[:int(prefix)]:
                    #append or remove ip from DHCP server stats
                    for dic_ip in dic_used:
                        if j == dic_ip:
                            if op == '05':
                                dic_used[j].append({ip_data_dec_str : [dec_time, timeit.default_timer() - start]})
                            elif op == '07':
                                for ind, ii in enumerate(dic_used[j]):
                                    if ip_data_dec_str in ii:
                                        dic_used[j].pop(ind)
                            dic['Allocated'][index] = str(len(dic_used[j]))                 
                tmp_list.append(tmp_str)


            #PRINT STAT TABLE

            utilization = []
            for x, y in zip(dic['Allocated'], dic['Max host']):
                utilization.append((int(x)*100)/float(y))
            dic['Utilization'] = utilization
            dic['Utilization'] = [ '%.2f' % elem for elem in utilization ]
            [str(elem) for elem in dic['Utilization']]

            ncurses.curses_main()
            ncurses.stdscr_getch()

def recv_pkts(hdr, data):
    """
    Callback for received packets
    Parameters:
      hdr - header
      data - data to parse
    """    
    global start
    global ncurses
    # remove from used ips after lease time expired
    for ind, i in enumerate(dic_used):
        if dic_used[i]:
            for x in dic_used[i]:
                flatt = [val for sublist in x.values() for val in sublist]
                #print flatt[1] - start
                if (timeit.default_timer() - start) > (flatt[0] + flatt[1]):
                    dic_used[i].remove(x)
                    #print new stats

                    ind2 = dic['IP Address'].index(dic_used.keys()[ind])
                    dic['Allocated'][ind2] = str(len(dic_used[i]))
                    utilization = []
                    for x, y in zip(dic['Allocated'], dic['Max host']):
                        utilization.append((int(x)*100)/float(y))
                    dic['Utilization'] = utilization
                    dic['Utilization'] = [ '%.2f' % elem for elem in utilization ]
                    [str(elem) for elem in dic['Utilization']]
                    #for row in zip(*([key] + value for key, value in sorted(dic.items()))):
                        #print '\t\t'.join(row)

                    ncurses.curses_main()
                    ncurses.stdscr_getch()

    parse_packet(data)

def check_ip(ip):
    """
    Check for validty of ip adresses
    Parameters:
        ip - IP address
    """
    prefix = int(ip[ip.find('/') + 1 :])
    if prefix < 0 or prefix > 32:
        raise ErrorWithCode(1)
    ip = ip[:ip.find('/')]

    ip_bin = ''.join([bin(int(x)+256)[3:] for x in ip.split('.')])

    ip_set = set(ip_bin[prefix:])
    if len(ip_set) > 1 or '1' in ip_set:
        raise ErrorWithCode(1)
    #throws exception if ip is not valid
    socket.inet_aton(ip)

def printInterface():
    """
    Grab a list of active interfaces
    """
    ifs = findalldevs()

    if 0 == len(ifs):
        raise ErrorWithCode(2)

    for count, i in enumerate(ifs):
        print '%i - %s' % (count, i)

def main(argv):
    """
    Main function.
    Parameters:
      argv - arguments from command line
    """
    global dic
    global ncurses
    global dic_used
    global start


    if len(sys.argv) < 2 :
        raise ErrorWithCode(1)

    if sys.argv[1] == "-f":
        printInterface()
        return
    elif sys.argv[1] == "-i":
        if len(sys.argv) < 4:
            raise ErrorWithCode(1)
        dev = sys.argv[2]
        ifs = findalldevs()
        if dev not in ifs :
            raise ErrorWithCode(3)
    else:
        raise ErrorWithCode(1)

    ip_list = sys.argv[3:]

    for ip in ip_list:
        check_ip(ip)

    start = timeit.default_timer()
    #dev = getInterface()
    #print ip
    prefix_list = []
    for i in ip_list:
        if int(i[i.find('/') + 1:]) == 31:
            max_host = 2
        elif int(i[i.find('/') + 1:]) == 32:
            max_host = 1
        else :
            max_host = (2 ** (32 - int(i[i.find('/') + 1:])) - 2)
        prefix_list.append(str(max_host))

    #print prefix_list
    prefix_list.sort(key=int, reverse=True)
    ip_list.sort(key=lambda x: int(x[i.find('/') + 1:]))
    for i in reversed(ip_list):
        dic_used.update({i : []})
    Allocated = []
    Utilization = []
    for l in range(len(ip_list)) :
        Allocated.append(str(0))
        Utilization.append(str(0))
    dic = {'IP Address' : ip_list, 'Max host' : prefix_list, "Allocated" : Allocated, "Utilization" : Utilization}

    #print first stats
    ncurses = ncurses()
    ncurses.curses_main()
    ncurses.stdscr_getch()    

    '''
    open device
    # Arguments here are:
    #   device
    #   snaplen (maximum number of bytes to capture _per_packet_)
    #   promiscious mode (1 for true)
    #   timeout (in milliseconds)
    '''
    cap = pcapy.open_live(dev , 65536 , 1 , 100)

    packet_limit = -1 # infinite
    cap.loop(packet_limit, recv_pkts) # capture packets

def eth_addr(a) :
    """
    Convert a string of 6 characters of ethernet address into a dash separated hex string
    Parameters:
        a - adress
    """    
    b = "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x" % (ord(a[0]) , ord(a[1]) , ord(a[2]), ord(a[3]), ord(a[4]) , ord(a[5]))
    return b

def toBin(tmp, numb):
    """
    Convert a value to binary
    Parameters:
        tmp - value
        numb - number
    """       
    return bin(int(tmp, numb))[2:].zfill(8)  

def getTime(hex_time):
    """
    Converts to int
    Parameters:
        hex_time - number to convert
    """
    return int(hex_time, 16)

def dumphex(s):
    """
    Decode data to hex format.
    Parameters:
        s - string to decode
    """       
    bytes = map(lambda x: '%.2x' % x, map(ord, s))
    data_str = ''
    for i in xrange(0,len(bytes)/16):
        data_str = data_str + string.join(bytes[i*16:(i+1)*16],' ')

    data_opt = data_str[700:]
    for t,c in enumerate(data_opt):
        if c == '3':
            if data_opt[t + 1] == '5':
                packet_type = data_opt[t + 6:t + 8]
                break
    #print packet_type
    if packet_type == '05':
        for t,c in enumerate(data_opt):
            if c == '3':
                if data_opt[t + 1] == '3':
                    if data_opt[t + 3:t + 5] == '04':
                        hex_time = data_opt[t + 6: t + 18]
                        dec_time = getTime(hex_time.replace(" ",""))
                        break
        parse_data(data_str[47:58].split(),packet_type, dec_time)
    elif packet_type == '07' :
        parse_data(data_str[35:47].split(),packet_type, 0)

def parse_packet(packet):
    """
    Function to parse packet
    Parameters:
        packet - data
    """        
    #parse ethernet header
    eth_length = 14

    eth_header = packet[:eth_length]
    eth = unpack('!6s6sH' , eth_header)
    eth_protocol = socket.ntohs(eth[2])
 
    #Parse IP packets, IP Protocol number = 8
    if eth_protocol == 8 :
        #Parse IP header
        #take first 20 characters for the ip header
        ip_header = packet[eth_length:20+eth_length]
         
        #now unpack them
        iph = unpack('!BBHHHBBH4s4s' , ip_header)
        version_ihl = iph[0]
        ihl = version_ihl & 0xF
 
        iph_length = ihl * 4

        protocol = iph[6]

        #UDP
        if protocol == 17 :
            u = iph_length + eth_length
            udph_length = 8
            udp_header = packet[u:u+8]
 
            #now unpack them :)
            udph = unpack('!HHHH' , udp_header)
             
            dest_port = udph[1]
            
            if dest_port == 68 or dest_port == 67:             
                h_size = eth_length + iph_length + udph_length
                #get data from the packet
                data = packet[h_size:]
            
                dumphex(data)
                
                return
 
if __name__ == "__main__":

    while True:
        try:
            main(sys.argv)
        except ErrorWithCode as e:
            if e.code == 1:
                print >> sys.stderr, 'Wrong arguments'
            elif e.code == 2:
                print >> sys.stderr, 'There is no interface to sniff'
            elif e.code == 3:
                print >> sys.stderr, 'Not valid interaface to sniff, use -i'
            else :
                print >> sys.stderr, 'Something went wrong'   
            sys.exit(e.code)
        except socket.error:
            print >> sys.stderr, 'Invalid IP adress'
            sys.exit(3)
        except KeyboardInterrupt:
            ncurses.curses_end()
            sys.exit(0)
        except ValueError:
            print >> sys.stderr, 'Something went wrong' 
            sys.exit(99)
        else:
            sys.exit(0)



