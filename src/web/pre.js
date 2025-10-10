/* web/pre.js - Preamble, intialization, and kscript <-> JavaScript wrappers
 *
 * @author: Cade Brown <cade@kscript.org>
 */
var Module = {
    'canvas': document.getElementById('canvas'),
    // List of callbacks to write to 'stdout' and 'stderr'
    '_write_stdout': [],
    '_write_stderr': [],

    // Overloading the 'stdout' and 'stderr' write callback
    'print': function(text) {
        for (var i = 0; i < Module._write_stdout.length; ++i) {
            Module._write_stdout[i](text);
        } 
    },
    'printErr': function(text) {
        for (var i = 0; i < Module._write_stderr.length; ++i) {
            Module._write_stderr[i](text);
        }
    },

    /* Checks for errors, and catches and prints them 
     *
     */
    'check': function() {
        Module._catch_ignore_print();
    },

    /* Increment reference count */
    'incref': function(obj) {
        Module._incref(obj)
    },

    /* Decrement reference count */
    'decref': function(obj) {
        Module._decref(obj)
    },

    /* Returns a kscript 'str' object with the given text
     */
    'make_str': function(text) {
        text = text.toString()

        // Fill a buffer with UTF8 bytes of the string
        var buf = Module.ccall('ks_malloc', 'number', ['number'], [text.length + 1])
        Module.stringToUTF8(text, buf, text.length + 1)

        // Wrap the result as a string
        var res = Module._str_new(text.length, buf)
        Module.ccall('ks_free', null, ['number'], [buf])
        return res
    },

    /* Returns a kscript 'int' object with the given value
     */
    'make_int': function(val) {
        var res = Module._int_new(val.valueOf())
        return res
    },

    /* Make a list
     */
    'make_list': function() {
        var res = Module._list_new(0, null);
        return res
    },

    /* Make a dictionary
     */
    'make_dict': function(obj={}) {
        var res = Module._dict_new(null);

        return res
    },

    /* Returns a kscript 'float' object with the given value
     */
    'make_int': function(val) {
        var res = Module._float_new(val.valueOf())
        return res
    },

    /* Returns a JavaScript string describing 'obj'
     *
     */
    'get_str': function(obj) {
        var buf = Module._str_c(obj)
        Module.check()

        var res = Module.UTF8ToString(buf)
        Module.ccall('ks_free', null, ['number'], [buf])

        return res
    },

    /* Returns a JavaScript string describing 'obj'
     *
     */
    'get_repr': function(obj) {
        var buf = Module._repr_c(obj)
        Module.check()

        var res = Module.UTF8ToString(buf)
        Module.ccall('ks_free', null, ['number'], [buf])

        return res
    },

    /* Returns a JavaScript string describing 'obj' for the interpreter */
    'get_display': function(obj) {
        var buf = Module._display_c(obj)
        Module.check()

        var res = Module.UTF8ToString(buf)
        Module.ccall('ks_free', null, ['number'], [buf])
        return res
    },

    /* Evaluate a string expression
     *
     */
    'eval': async function(src, fname="<>", locals=0) {
        var o_src = Module.make_str(src)
        var o_fname = Module.make_str(fname)

        // forcibly wait here - eval is async
        var res = await Module._eval(o_src, o_fname, locals)
        Module.check()

        Module.decref(o_src)
        Module.decref(o_fname)

        return res
    },


    /* Return a hash of the current IO objects, to know if anything was written
     */
    'iohash': function() {
        return Module._iohash()
    },

    /* Compute interactive completions for the given text
     */
    'inter_completions': function(cmd, locals) {
        let out_list = Module.make_list()
        var cmdbuf = Module.ccall('ks_malloc', 'number', ['number'], [cmd.length * 4 + 1])
        Module.stringToUTF8(cmd, cmdbuf, cmd.length + 1)
        cmdbuf[cmd.length] = 

        Module.ccall('ks_inter_completions', null, ['number', 'number', 'number'], [out_list, cmdbuf, locals])
        Module.check()
        Module.ccall('ks_free', null, ['number'], [cmdbuf])
        let res = []
        let n = Module._list_len(out_list)
        for (let i = 0; i < n; ++i) {
            let item = Module._list_get(out_list, i)
            Module.check()
            res.push(Module.get_str(item))
            Module.decref(item)
        }
        Module.decref(out_list)
        return res
    },

    /* Initializes the kscript library
     * NOTE: This must be called before anything else
     */
    'init': function() {
        // First, add some callbacks to print to the console on 'stdout'/'stderr' lines
        Module._write_stdout.push(console.log)
        Module._write_stderr.push(console.warn)

        //Module._ks_malloc = Module.cwrap('ks_malloc', 'number', ['number'])
        //Module._ks_free = Module.cwrap('ks_free', null, ['number'])

        // Get C-wrappers
        Module._incref = Module.cwrap('_ksem_incref_', null, ['number'])
        Module._decref = Module.cwrap('_ksem_decref_', null, ['number'])
        Module._list_len = Module.cwrap('_ksem_list_len_c_', 'number', ['number'])
        Module._list_get = Module.cwrap('_ksem_list_get_c_', 'number', ['number', 'number'])
        
        Module._refs = Module.cwrap('_ksem_refs_c_', 'number', ['number'])
        Module._type = Module.cwrap('_ksem_type_c_', 'number', ['number'])

        Module._iohash = Module.cwrap('_ksem_iohash_c_', 'number', [])

        Module._display_c = Module.cwrap('_ksem_display_c_', 'number', ['number'])
        Module._str_c = Module.cwrap('_ksem_str_c_', 'number', ['number'])
        Module._repr_c = Module.cwrap('_ksem_repr_c_', 'number', ['number'])
        
        Module._eval = Module.cwrap('kso_eval', 'number', ['number', 'number', 'number'], { async: true });
        
        Module._str_new = Module.cwrap('ks_str_new', 'number', ['number', 'number']);
        Module._int_new = Module.cwrap('ks_int_new', 'number', ['number']);
        Module._float_new = Module.cwrap('ks_float_new', 'number', ['number']);
        Module._list_new = Module.cwrap('ks_list_new', 'number', ['number', 'number']);
        Module._dict_new = Module.cwrap('ks_dict_new', 'number', ['number']);

        Module._catch_ignore_print = Module.cwrap('kso_catch_ignore_print', 'number', [])
        
        // Actually initialize the module
        var res = Module.ccall('ks_init',
            'number', [],
            [ ]
        )

        // Get version string
        Module.verstr = Module.UTF8ToString(Module.ccall('ks_get_verstr', 'number', [], []))

        return res
    },

};
