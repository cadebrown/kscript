""" talk-server.ks - Example server showing how to create a telnet-like communication program


SEE: talk-client.ks

@author: Cade Brown <cade@kscript.org>
"""

import os
import net
import getarg

p = getarg.Parser('talk-server', '0.0.1', 'Socket-based server for a talk client', ['Cade Brown <cade@kscript.org>'])

p.opt('port', ['-p', '--port'], 'Port to bind to', int, 8080)
p.opt('addr', ['-a', '--addr'], 'Address to bind to', str, 'localhost')

args = p.parse()

s = net.SocketIO()

s.bind((args.addr, args.port))

# Accept up to 16 connections
s.listen(16)

# Globals
g = {
    'ct': 0
}

func handle(sock, name) {
    tid = g['ct'] += 1
    print ('START', name + '[' + str(tid) +']')
    for l in sock {
        print (name + '[' + str(tid) + ']' + ':', l.decode())
    }
    print ('END', name)
}

while true {
    v = s.accept()
    os.thread(handle, v).start()
}

s.close()
