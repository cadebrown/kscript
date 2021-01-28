""" talk-client.ks - Example client which connects to a server


SEE: talk-server.ks

@author: Cade Brown <cade@kscript.org>
"""


import net
import getarg

p = getarg.Parser('talk-client', '0.0.1', 'Socket-based client for a talk server', ['Cade Brown <cade@kscript.org>'])

p.opt('port', ['-p', '--port'], 'Port to bind to', int, 8080)
p.opt('addr', ['-a', '--addr'], 'Address to bind to', str, 'localhost')

args = p.parse()

s = net.SocketIO()

s.connect((args.addr, args.port))

# Write 'stdin' to the socket
for line in __stdin { 
    s.write(line + '\n')
}
