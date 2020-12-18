
import net
import getarg

p = getarg.Parser('talk-server', '0.0.1', 'Socket-based server for a talk client', ['Cade Brown <cade@kscript.org>'])

p.opt('port', ['-p', '--port'], 'Port to bind to', int, 8080)
p.opt('addr', ['-a', '--addr'], 'Address to bind to', str, '')

args = p.parse()

s = net.SocketIO()

s.bind((args.addr, args.port))

s.listen()

while true {
    v = s.accept()
    print (v)
    
    # read lines
    for line in v[0], print (line.decode())

}

