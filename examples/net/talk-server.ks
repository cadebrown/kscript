
import os
import net
import getarg

p = getarg.Parser('talk-server', '0.0.1', 'Socket-based server for a talk client', ['Cade Brown <cade@kscript.org>'])

p.opt('port', ['-p', '--port'], 'Port to bind to', int, 8080)
p.opt('addr', ['-a', '--addr'], 'Address to bind to', str, 'localhost')

args = p.parse()

s = net.SocketIO()

s.bind((args.addr, args.port))

s.listen()

func handle(sock, name) {
    print ('START', name)
    for l in sock {
        print (name + ':', l.decode())
    }
    print ('END', name)
}

while true {
    v = s.accept()
    os.thread(handle, v).start()
}

s.close()
