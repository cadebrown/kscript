""" eval-url.ks - HTTP server which evalutes the URL as input and returns the result (unsafe!)


NOTE: This is unsafe! It's just for testing. You shouldn't 'eval()' arbitrary input on your server

@author: Cade Brown <cade@kscript.org>
"""


import net
import getarg

# Parse argument
p = getarg.Parser('eval-url', '0.0.1', 'HTTP server which evaluates the URL (unsafe!)', ['Cade Brown <cade@kscript.org>'])

p.opt('addr', ['-a', '--addr'], 'Address to bind to', str, 'localhost')
p.opt('port', ['-p', '--port'], 'Port to bind to', int, 8080)

args = p.parse()

# Custom server type 
type Server extends net.http.Server {

    # overload 'handle' function to transform a request into the response
    func handle(self, addr, sock, req) {
        code = net.http.uridecode(req.uri[1:])
        try {
            # Try to evaluate the code directly
            res = eval(code)
            ret "%s" % (res,)
        } catch as e {
            # Something went wrong
            ret "%t: %s" % (e, e)
        }
    }
}

# Create and serve on a server
Server((args.addr, args.port)).serve()
